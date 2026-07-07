#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// MainWindow.h
// 主窗口类声明文件。
// 负责声明界面控件、按钮槽函数，以及图片导入、显示等界面相关功能。

#include <QMainWindow>
#include <QString>

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