#include "config/AppConfig.h"

#include <QCoreApplication>

namespace
{
constexpr double DEFAULT_SIMILARITY_THRESHOLD = 0.80;
constexpr int YOLO_INPUT_SIZE = 640;
constexpr float YOLO_CONFIDENCE_THRESHOLD = 0.35f;
constexpr float YOLO_NMS_THRESHOLD = 0.45f;
constexpr int YOLO_MAX_FACES = 100;
constexpr int CAMERA_FRAME_INTERVAL_MS = 33;
constexpr int CAMERA_LOG_FRAME_INTERVAL = 30;
}

QString AppConfig::resourceDirectory()
{
    return QCoreApplication::applicationDirPath() + "/resources";
}

QString AppConfig::faceCascadeModelPath()
{
    return resourceDirectory() + "/haarcascade_frontalface_default.xml";
}

QString AppConfig::yoloFaceDetectorModelPath()
{
    return resourceDirectory() + "/models/face_detector_yolo.onnx";
}

QString AppConfig::dataDirectory()
{
    return QCoreApplication::applicationDirPath() + "/data";
}

QString AppConfig::databasePath()
{
    return dataDirectory() + "/face_recognition.db";
}

double AppConfig::defaultSimilarityThreshold()
{
    return DEFAULT_SIMILARITY_THRESHOLD;
}

int AppConfig::yoloInputSize()
{
    return YOLO_INPUT_SIZE;
}

float AppConfig::yoloConfidenceThreshold()
{
    return YOLO_CONFIDENCE_THRESHOLD;
}

float AppConfig::yoloNmsThreshold()
{
    return YOLO_NMS_THRESHOLD;
}

int AppConfig::yoloMaxFaces()
{
    return YOLO_MAX_FACES;
}

int AppConfig::cameraFrameIntervalMs()
{
    return CAMERA_FRAME_INTERVAL_MS;
}

int AppConfig::cameraLogFrameInterval()
{
    return CAMERA_LOG_FRAME_INTERVAL;
}
