#ifndef FACEDETECTOR_H
#define FACEDETECTOR_H

#include <QString>
#include <opencv2/core.hpp>
#include <opencv2/core/version.hpp>
#include <opencv2/dnn.hpp>

#if CV_VERSION_MAJOR < 5
#include <opencv2/objdetect.hpp>
#endif

#include "vision/FaceDetection.h"

#include <vector>

class FaceDetector
{
public:
    // 加载 Haar 级联模型，作为 YOLO 不可用时的备用检测器。
    bool loadModel(const QString &modelPath);

    // 加载单类别 face 的 YOLO ONNX 模型。
    bool loadYoloModel(const QString &modelPath);

    // 判断至少有一个检测模型已经成功加载。
    bool isLoaded() const;

    // 返回当前优先检测器的名称：YOLO ONNX 或 Haar Cascade。
    QString activeModelName() const;

    // 返回最近一次检测实际使用的检测器名称。
    QString lastDetectionModelName() const;

    // 返回 YOLO 模型加载失败的原因，便于在界面中诊断回退情况。
    QString yoloLoadError() const;

    // 返回最近一次 YOLO 推理失败的原因，便于显示自动回退的真实原因。
    QString yoloInferenceError() const;

    // 检测图片中的人脸，包含人脸框和检测置信度。
    std::vector<FaceDetection> detectWithScores(const cv::Mat &image);

    // 保持原有调用接口，返回人脸矩形框列表。
    std::vector<cv::Rect> detect(const cv::Mat &image);

private:
    std::vector<FaceDetection> detectYolo(const cv::Mat &image);
    std::vector<FaceDetection> detectHaar(const cv::Mat &image);

#if CV_VERSION_MAJOR < 5
    cv::CascadeClassifier faceCascade;
#endif
    cv::dnn::Net yoloNet;
    bool yoloLoaded = false;
    QString lastDetectionModel;
    QString yoloLoadErrorMessage;
    QString yoloInferenceErrorMessage;
};

#endif
