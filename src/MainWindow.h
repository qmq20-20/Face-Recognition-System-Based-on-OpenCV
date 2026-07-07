#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// MainWindow.h
// 主窗口类声明文件。
// 负责声明界面控件、按钮槽函数，以及图片导入、显示等界面相关功能。

#include <QMainWindow>
#include <QString>
#include "FaceDetector.h"
#include "FeatureExtractor.h"
#include "FaceRepository.h"
#include "RecognitionService.h"
#include <vector>
#include <opencv2/opencv.hpp>

class QLabel;
class QPushButton;
class QTableWidget;
class QTextEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onImportImageClicked();
    void onRegisterPersonClicked();
    void onRecognizeClicked();
    void onClearClicked();
    void onStartCameraClicked();
    void onStopCameraClicked();

private:
    void setupUi();

    // 从文件路径读取图片。
    // 使用 Qt 先读取文件字节，再交给 OpenCV 解码，可以更好兼容中文路径。
    cv::Mat readImageFromFile(const QString &filePath);

    // 把 OpenCV 的 cv::Mat 转换成 Qt 的 QImage，供 QLabel 显示。
    QImage matToQImage(const cv::Mat &mat);

    // 将图片按比例缩放后显示到 imageLabel。
    void displayImage(const cv::Mat &mat);

    // 人脸检测器，负责加载模型和检测人脸。
    FaceDetector faceDetector;

    // 当前图片中检测到的人脸框。
    std::vector<cv::Rect> currentFaces;

    // 在图片上绘制人脸框并显示。
    void displayImageWithFaces(const cv::Mat &mat, const std::vector<cv::Rect> &faces);

    // 人脸特征提取器，负责把人脸图像转换成固定长度向量。
    FeatureExtractor featureExtractor;

    // 从当前图片中裁剪指定人脸区域。
    cv::Mat cropFace(const cv::Mat &image, const cv::Rect &faceRect);

    // 数据库访问对象，负责保存和读取人员、特征、日志。
    FaceRepository repository;

    // 初始化 SQLite 数据库。
    void initializeDatabase();

    // 刷新界面中的人员列表。
    void refreshPersonTable();

    // 刷新界面中的识别日志。
    void refreshLogView();

    // 识别服务，负责相似度计算和身份判断。
    RecognitionService recognitionService;

    // 在图片上绘制人脸框和识别结果。
    void displayImageWithRecognitionResults(
        const cv::Mat &mat,
        const std::vector<cv::Rect> &faces,
        const QList<RecognitionResult> &results);

private:
    QLabel *imageLabel;
    QTableWidget *personTable;
    QTextEdit *resultTextEdit;
    QTextEdit *logTextEdit;

    QPushButton *importImageButton;
    QPushButton *registerPersonButton;
    QPushButton *recognizeButton;
    QPushButton *clearButton;
    QPushButton *startCameraButton;
    QPushButton *stopCameraButton;

    // 当前导入的图片路径，后续识别和日志会用到。
    QString currentImagePath;

    // 当前导入的 OpenCV 图片数据，后续人脸检测会直接使用。
    cv::Mat currentImage;
};

#endif