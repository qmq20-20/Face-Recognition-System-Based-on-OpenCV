// main.cpp
// 程序入口文件。
// 负责创建 QApplication 对象，创建并显示主窗口 MainWindow，
// 最后进入 Qt 的事件循环，等待用户点击按钮、导入图片、关闭窗口等操作。
#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec();
}