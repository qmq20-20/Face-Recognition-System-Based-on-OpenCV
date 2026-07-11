#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QString>

class AppConfig
{
public:
    static QString resourceDirectory();
    static QString faceCascadeModelPath();
    static QString yoloFaceDetectorModelPath();
    static QString dataDirectory();
    static QString databasePath();

    static double defaultSimilarityThreshold();
    static int yoloInputSize();
    static float yoloConfidenceThreshold();
    static float yoloNmsThreshold();
    static int yoloMaxFaces();
    static int cameraFrameIntervalMs();
    static int cameraLogFrameInterval();
};

#endif
