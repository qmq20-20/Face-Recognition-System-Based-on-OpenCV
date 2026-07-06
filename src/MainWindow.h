// MainWindow.h
// 主窗口类声明文件。
// 负责声明界面中需要用到的控件、按钮槽函数，以及和人脸检测、识别、数据库模块交互的接口。
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget *parent = nullptr);
};

#endif