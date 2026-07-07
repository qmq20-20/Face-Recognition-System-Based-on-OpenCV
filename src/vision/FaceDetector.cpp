#include "vision/FaceDetector.h"

#include <QFileInfo>
#include <opencv2/imgproc.hpp>

bool FaceDetector::loadModel(const QString &modelPath)
{
    if (!QFileInfo::exists(modelPath))
    {
        return false;
    }

    return faceCascade.load(modelPath.toStdString());
}

bool FaceDetector::isLoaded() const
{
    return !faceCascade.empty();
}

std::vector<cv::Rect> FaceDetector::detect(const cv::Mat &image)
{
    std::vector<cv::Rect> faces;

    if (image.empty() || faceCascade.empty())
    {
        return faces;
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

    faceCascade.detectMultiScale(
        gray,
        faces,
        1.05,
        5,
        0,
        cv::Size(30, 30));

    return faces;
}
