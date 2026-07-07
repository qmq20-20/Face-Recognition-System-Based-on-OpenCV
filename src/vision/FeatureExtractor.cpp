#include "vision/FeatureExtractor.h"

#include <opencv2/imgproc.hpp>

std::vector<float> FeatureExtractor::extract(const cv::Mat &faceImage) const
{
    std::vector<float> feature;

    if (faceImage.empty())
    {
        return feature;
    }

    cv::Mat gray;

    // 第一步：转灰度图。
    // 人脸识别第一版只使用亮度信息，不使用颜色信息。
    if (faceImage.channels() == 3)
    {
        cv::cvtColor(faceImage, gray, cv::COLOR_BGR2GRAY);
    }
    else
    {
        gray = faceImage.clone();
    }

    // 第二步：统一尺寸。
    // 不同人脸框大小不同，必须缩放成固定大小，才能得到固定长度特征。
    cv::Mat resized;
    cv::resize(gray, resized, cv::Size(FACE_WIDTH, FACE_HEIGHT));

    // 第三步：直方图均衡化。
    // 作用是增强对比度，让光照差异对特征影响稍微小一点。
    cv::Mat equalized;
    cv::equalizeHist(resized, equalized);

    // 第四步：归一化并展开成一维向量。
    // 像素值原本是 0~255，转换成 0.0~1.0，方便后续计算相似度。
    feature.reserve(FACE_WIDTH * FACE_HEIGHT);

    for (int row = 0; row < equalized.rows; ++row)
    {
        for (int col = 0; col < equalized.cols; ++col)
        {
            float value = static_cast<float>(equalized.at<uchar>(row, col)) / 255.0f;
            feature.push_back(value);
        }
    }

    return feature;
}

int FeatureExtractor::featureLength() const
{
    return FACE_WIDTH * FACE_HEIGHT;
}
