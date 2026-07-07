#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QString>

class AppConfig
{
public:
    static QString resourceDirectory();
    static QString faceCascadeModelPath();
    static QString dataDirectory();
    static QString databasePath();

    static double defaultSimilarityThreshold();
    static int cameraFrameIntervalMs();
    static int cameraLogFrameInterval();
};

#endif
