#ifndef IMAGEUTILS_H
#define IMAGEUTILS_H

#include <QImage>
#include <QString>
#include <opencv2/core.hpp>

namespace ImageUtils
{
cv::Mat readImageFromFile(const QString &filePath);
QImage matToQImage(const cv::Mat &mat);
cv::Mat cropFace(const cv::Mat &image, const cv::Rect &faceRect);
}

#endif
