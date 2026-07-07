#ifndef FEATUREEXTRACTOR_H
#define FEATUREEXTRACTOR_H

// FeatureExtractor.h
// 人脸特征提取模块声明文件。
// 负责把裁剪后的人脸图片转换成固定长度的浮点数特征向量。
// 第一版使用简单图像特征：灰度化、缩放、均衡化、归一化、展开成向量。

#include <opencv2/core.hpp>

#include <vector>

class FeatureExtractor
{
public:
    // 提取人脸特征。
    // 参数 faceImage 是已经裁剪出来的人脸图像。
    // 返回值是固定长度的浮点数向量。
    std::vector<float> extract(const cv::Mat &faceImage) const;

    // 返回特征向量长度。
    // 当前使用 64x64 灰度图，所以长度是 4096。
    int featureLength() const;

private:
    static const int FACE_WIDTH = 64;
    static const int FACE_HEIGHT = 64;
};

#endif