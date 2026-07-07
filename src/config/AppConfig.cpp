#include "config/AppConfig.h"

#include <QCoreApplication>

namespace
{
constexpr double DEFAULT_SIMILARITY_THRESHOLD = 0.80;
constexpr int CAMERA_FRAME_INTERVAL_MS = 30;
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

int AppConfig::cameraFrameIntervalMs()
{
    return CAMERA_FRAME_INTERVAL_MS;
}

int AppConfig::cameraLogFrameInterval()
{
    return CAMERA_LOG_FRAME_INTERVAL;
}
