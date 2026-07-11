#include "vision/FaceDetector.h"

#include "config/AppConfig.h"

#include <QFileInfo>
#include <algorithm>
#include <cmath>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>

bool FaceDetector::loadModel(const QString &modelPath)
{
#if CV_VERSION_MAJOR < 5
    if (!QFileInfo::exists(modelPath))
    {
        return false;
    }

    return faceCascade.load(modelPath.toStdString());
#else
    Q_UNUSED(modelPath);
    return false;
#endif
}

bool FaceDetector::isLoaded() const
{
#if CV_VERSION_MAJOR < 5
    return yoloLoaded || !faceCascade.empty();
#else
    return yoloLoaded;
#endif
}

bool FaceDetector::loadYoloModel(const QString &modelPath)
{
    yoloLoaded = false;
    yoloNet = cv::dnn::Net();
    yoloLoadErrorMessage.clear();
    yoloInferenceErrorMessage.clear();

    if (!QFileInfo::exists(modelPath))
    {
        yoloLoadErrorMessage = "模型文件不存在。";
        return false;
    }

    try
    {
        yoloNet = cv::dnn::readNetFromONNX(modelPath.toStdString());
        yoloNet.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        yoloNet.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        yoloLoaded = !yoloNet.empty();

        if (!yoloLoaded)
        {
            yoloLoadErrorMessage = "OpenCV DNN 未能创建有效的 ONNX 网络。";
        }
    }
    catch (const cv::Exception &error)
    {
        yoloNet = cv::dnn::Net();
        yoloLoadErrorMessage = QString::fromStdString(error.what());
    }

    return yoloLoaded;
}

QString FaceDetector::activeModelName() const
{
    if (yoloLoaded)
    {
        return "YOLO ONNX";
    }

#if CV_VERSION_MAJOR < 5
    if (!faceCascade.empty())
    {
        return "Haar Cascade";
    }
#endif

    return "未加载";
}

QString FaceDetector::lastDetectionModelName() const
{
    return lastDetectionModel.isEmpty() ? activeModelName() : lastDetectionModel;
}

QString FaceDetector::yoloLoadError() const
{
    return yoloLoadErrorMessage;
}

QString FaceDetector::yoloInferenceError() const
{
    return yoloInferenceErrorMessage;
}

std::vector<FaceDetection> FaceDetector::detectWithScores(const cv::Mat &image)
{
    if (image.empty())
    {
        return {};
    }

    if (yoloLoaded)
    {
        yoloInferenceErrorMessage.clear();

        try
        {
            std::vector<FaceDetection> detections = detectYolo(image);
            lastDetectionModel = "YOLO ONNX";
            return detections;
        }
        catch (const cv::Exception &error)
        {
            yoloInferenceErrorMessage = QString::fromStdString(error.what());
#if CV_VERSION_MAJOR < 5
            // ONNX 推理失败时仍可使用已加载的 Haar 模型继续工作。
            lastDetectionModel = "Haar Cascade（YOLO 推理失败，已回退）";
#else
            lastDetectionModel = "YOLO ONNX 推理失败";
#endif
        }
    }

#if CV_VERSION_MAJOR < 5
    if (!yoloLoaded)
    {
        lastDetectionModel = "Haar Cascade";
    }
#else
    if (!yoloLoaded)
    {
        lastDetectionModel = "未加载";
    }
#endif

    return detectHaar(image);
}

std::vector<cv::Rect> FaceDetector::detect(const cv::Mat &image)
{
    const std::vector<FaceDetection> detections = detectWithScores(image);
    std::vector<cv::Rect> faces;
    faces.reserve(detections.size());

    for (const FaceDetection &detection : detections)
    {
        faces.push_back(detection.box);
    }

    return faces;
}

std::vector<FaceDetection> FaceDetector::detectYolo(const cv::Mat &image)
{
    const int inputSize = AppConfig::yoloInputSize();
    const float scale = std::min(
        static_cast<float>(inputSize) / static_cast<float>(image.cols),
        static_cast<float>(inputSize) / static_cast<float>(image.rows));
    const int resizedWidth = static_cast<int>(std::round(image.cols * scale));
    const int resizedHeight = static_cast<int>(std::round(image.rows * scale));
    const int paddingX = (inputSize - resizedWidth) / 2;
    const int paddingY = (inputSize - resizedHeight) / 2;

    cv::Mat resized;
    cv::resize(image, resized, cv::Size(resizedWidth, resizedHeight));

    cv::Mat letterboxed;
    cv::copyMakeBorder(
        resized,
        letterboxed,
        paddingY,
        inputSize - resizedHeight - paddingY,
        paddingX,
        inputSize - resizedWidth - paddingX,
        cv::BORDER_CONSTANT,
        cv::Scalar(114, 114, 114));

    cv::Mat blob = cv::dnn::blobFromImage(
        letterboxed,
        1.0 / 255.0,
        cv::Size(inputSize, inputSize),
        cv::Scalar(),
        true,
        false);
    yoloNet.setInput(blob);

    cv::Mat output = yoloNet.forward();

    // Ultralytics YOLO11 单类别检测模型输出形状为 [1, 5, 8400]：xywh + face score。
    if (output.dims != 3 || output.size[0] != 1 || output.size[1] < 5)
    {
        CV_Error(cv::Error::StsError, "Unexpected YOLO ONNX output shape.");
    }

    const int channelCount = output.size[1];
    const int candidateCount = output.size[2];
    cv::Mat candidates(channelCount, candidateCount, CV_32F, output.ptr<float>());
    cv::Mat transposedCandidates;
    cv::transpose(candidates, transposedCandidates);

    std::vector<cv::Rect> boxes;
    std::vector<float> scores;

    for (int row = 0; row < transposedCandidates.rows; ++row)
    {
        const float *data = transposedCandidates.ptr<float>(row);
        const float score = data[4];

        if (score < AppConfig::yoloConfidenceThreshold())
        {
            continue;
        }

        const float left = data[0] - data[2] * 0.5f;
        const float top = data[1] - data[3] * 0.5f;
        const int x = static_cast<int>(std::round((left - paddingX) / scale));
        const int y = static_cast<int>(std::round((top - paddingY) / scale));
        const int width = static_cast<int>(std::round(data[2] / scale));
        const int height = static_cast<int>(std::round(data[3] / scale));
        cv::Rect box(x, y, width, height);
        box &= cv::Rect(0, 0, image.cols, image.rows);

        if (box.area() > 0)
        {
            boxes.push_back(box);
            scores.push_back(score);
        }
    }

    std::vector<int> indices;
    cv::dnn::NMSBoxes(
        boxes,
        scores,
        AppConfig::yoloConfidenceThreshold(),
        AppConfig::yoloNmsThreshold(),
        indices);

    std::sort(indices.begin(), indices.end(), [&scores](int leftIndex, int rightIndex) {
        return scores[leftIndex] > scores[rightIndex];
    });

    const int faceCount = std::min(
        static_cast<int>(indices.size()),
        AppConfig::yoloMaxFaces());
    std::vector<FaceDetection> detections;
    detections.reserve(faceCount);

    for (int i = 0; i < faceCount; ++i)
    {
        const int index = indices[i];
        detections.push_back({boxes[index], scores[index]});
    }

    return detections;
}

std::vector<FaceDetection> FaceDetector::detectHaar(const cv::Mat &image)
{
    std::vector<FaceDetection> detections;

#if CV_VERSION_MAJOR < 5
    if (image.empty() || faceCascade.empty())
    {
        return detections;
    }

    cv::Mat gray;

    // Haar 检测一般使用灰度图，速度更快，也更稳定。
    if (image.channels() == 3)
    {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    }
    else
    {
        gray = image.clone();
    }

    // 提高图像对比度，有助于检测人脸。
    cv::equalizeHist(gray, gray);

    std::vector<cv::Rect> faces;
    faceCascade.detectMultiScale(
        gray,
        faces,
        1.05,
        5,
        0,
        cv::Size(30, 30));

    detections.reserve(faces.size());

    for (const cv::Rect &face : faces)
    {
        detections.push_back({face, 1.0f});
    }
#else
    Q_UNUSED(image);
#endif

    return detections;
}
