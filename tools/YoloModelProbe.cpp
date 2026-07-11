#include "config/AppConfig.h"
#include "vision/FaceDetector.h"

#include <QCoreApplication>
#include <QTextStream>

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);
    const QStringList arguments = application.arguments();
    const QString yoloPath = arguments.size() > 1
        ? arguments.at(1)
        : AppConfig::yoloFaceDetectorModelPath();

    FaceDetector detector;
    detector.loadModel(AppConfig::faceCascadeModelPath());
    const bool yoloLoaded = detector.loadYoloModel(yoloPath);

    cv::Mat blankFrame(480, 640, CV_8UC3, cv::Scalar(0, 0, 0));
    detector.detect(blankFrame);

    QTextStream output(stdout);
    output << "YOLO path: " << yoloPath << "\n";
    output << "YOLO loaded: " << (yoloLoaded ? "true" : "false") << "\n";
    output << "Detection backend: " << detector.lastDetectionModelName() << "\n";
    output << "YOLO load error: " << detector.yoloLoadError() << "\n";
    output << "YOLO inference error: " << detector.yoloInferenceError() << "\n";

    return detector.lastDetectionModelName() == "YOLO ONNX" ? 0 : 1;
}
