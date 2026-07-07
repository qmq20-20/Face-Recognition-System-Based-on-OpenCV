#include "utils/ImageUtils.h"

#include <QByteArray>
#include <QFile>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <vector>

cv::Mat ImageUtils::readImageFromFile(const QString &filePath)
{
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly))
    {
        return cv::Mat();
    }

    QByteArray imageData = file.readAll();

    std::vector<uchar> buffer(
        reinterpret_cast<const uchar *>(imageData.constData()),
        reinterpret_cast<const uchar *>(imageData.constData()) + imageData.size());

    return cv::imdecode(buffer, cv::IMREAD_COLOR);
}

QImage ImageUtils::matToQImage(const cv::Mat &mat)
{
    if (mat.empty())
    {
        return QImage();
    }

    cv::Mat rgbImage;

    if (mat.channels() == 3)
    {
        cv::cvtColor(mat, rgbImage, cv::COLOR_BGR2RGB);
        return QImage(
                   rgbImage.data,
                   rgbImage.cols,
                   rgbImage.rows,
                   static_cast<int>(rgbImage.step),
                   QImage::Format_RGB888)
            .copy();
    }

    if (mat.channels() == 1)
    {
        return QImage(
                   mat.data,
                   mat.cols,
                   mat.rows,
                   static_cast<int>(mat.step),
                   QImage::Format_Grayscale8)
            .copy();
    }

    return QImage();
}

cv::Mat ImageUtils::cropFace(const cv::Mat &image, const cv::Rect &faceRect)
{
    if (image.empty())
    {
        return cv::Mat();
    }

    cv::Rect imageRect(0, 0, image.cols, image.rows);
    cv::Rect safeRect = faceRect & imageRect;

    if (safeRect.width <= 0 || safeRect.height <= 0)
    {
        return cv::Mat();
    }

    return image(safeRect).clone();
}
