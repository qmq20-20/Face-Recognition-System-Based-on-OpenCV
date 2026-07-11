#ifndef CAMERARECOGNITIONRESULT_H
#define CAMERARECOGNITIONRESULT_H

#include "service/RecognitionService.h"

#include <QString>
#include <QtGlobal>
#include <opencv2/core.hpp>

#include <vector>

struct CameraRecognitionResult
{
    cv::Mat frame;
    std::vector<cv::Rect> faces;
    QList<RecognitionResult> recognitionResults;
    QString detectorName;
    QString detectorFallbackReason;
    QString processingError;
    quint64 cameraSessionId = 0;
    bool hasKnownFeatures = false;
    bool shouldWriteLog = false;
};

#endif
