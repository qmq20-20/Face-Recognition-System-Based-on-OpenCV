#ifndef FACEDETECTOR_H
#define FACEDETECTOR_H

// FaceDetector.h
// 人脸检测模块声明文件。
// 负责加载 OpenCV Haar 人脸检测模型，并检测图片中的人脸位置。

#include <QString>
#include <opencv2/core.hpp>
#include <opencv2/objdetect.hpp>

#include <vector>

class FaceDetector
{
public:
    // 加载 Haar 模型文件。
    bool loadModel(const QString &modelPath);

    // 判断模型是否已经成功加载。
    bool isLoaded() const;

    // 检测图片中的人脸，返回人脸矩形框列表。
    std::vector<cv::Rect> detect(const cv::Mat &image);

private:
    cv::CascadeClassifier faceCascade;
};

#endif