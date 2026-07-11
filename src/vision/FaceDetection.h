#ifndef FACEDETECTION_H
#define FACEDETECTION_H

#include <opencv2/core.hpp>

struct FaceDetection
{
    cv::Rect box;
    float score = 0.0f;
};

#endif
