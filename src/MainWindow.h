#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// MainWindow.h
// 主窗口类声明文件。
// 负责声明界面控件、按钮槽函数，以及后续与人脸检测、识别、数据库模块交互的接口。

#include <QMainWindow>

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
    // 以下槽函数先只做占位提示，后续步骤再逐个实现具体功能。
    void onImportImageClicked();
    void onRegisterPersonClicked();
    void onRecognizeClicked();
    void onClearClicked();
    void onStartCameraClicked();
    void onStopCameraClicked();

private:
    // 初始化主界面布局和控件。
    void setupUi();

    // 图片显示区域，用于显示导入图片或摄像头画面。
    QLabel *imageLabel;

    // 人员列表，用于显示已经注册的人员。
    QTableWidget *personTable;

    // 识别结果显示区域，用于显示姓名、相似度等信息。
    QTextEdit *resultTextEdit;

    // 识别日志显示区域，用于显示历史识别记录。
    QTextEdit *logTextEdit;

    // 功能按钮。
    QPushButton *importImageButton;
    QPushButton *registerPersonButton;
    QPushButton *recognizeButton;
    QPushButton *clearButton;
    QPushButton *startCameraButton;
    QPushButton *stopCameraButton;
};

#endif