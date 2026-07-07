// MainWindow.cpp
// 主窗口类实现文件。
// 负责创建和布局界面，响应按钮点击事件，显示图片、人脸框、识别结果和识别日志。
// MainWindow 只负责界面流程，不直接实现复杂的人脸检测、特征提取或数据库逻辑。

#include "MainWindow.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
}

void MainWindow::setupUi()
{
    setWindowTitle("Face Recognition System");
    resize(1200, 760);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    imageLabel = new QLabel("图片 / 摄像头画面显示区域", this);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setMinimumSize(720, 520);
    imageLabel->setStyleSheet(
        "QLabel {"
        "border: 1px solid #999;"
        "background-color: #f5f5f5;"
        "font-size: 18px;"
        "color: #666;"
        "}");

    importImageButton = new QPushButton("导入图片", this);
    registerPersonButton = new QPushButton("注册人员", this);
    recognizeButton = new QPushButton("开始识别", this);
    clearButton = new QPushButton("清空", this);
    startCameraButton = new QPushButton("开始摄像头", this);
    stopCameraButton = new QPushButton("停止摄像头", this);

    QVBoxLayout *buttonLayout = new QVBoxLayout;
    buttonLayout->addWidget(importImageButton);
    buttonLayout->addWidget(registerPersonButton);
    buttonLayout->addWidget(recognizeButton);
    buttonLayout->addWidget(clearButton);
    buttonLayout->addWidget(startCameraButton);
    buttonLayout->addWidget(stopCameraButton);
    buttonLayout->addStretch();

    personTable = new QTableWidget(this);
    personTable->setColumnCount(3);
    personTable->setHorizontalHeaderLabels(QStringList() << "姓名" << "学号" << "部门");
    personTable->horizontalHeader()->setStretchLastSection(true);
    personTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    personTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    resultTextEdit = new QTextEdit(this);
    resultTextEdit->setReadOnly(true);
    resultTextEdit->setPlaceholderText("识别结果将在这里显示");

    logTextEdit = new QTextEdit(this);
    logTextEdit->setReadOnly(true);
    logTextEdit->setPlaceholderText("识别日志将在这里显示");

    QVBoxLayout *rightLayout = new QVBoxLayout;
    rightLayout->addWidget(new QLabel("功能按钮", this));
    rightLayout->addLayout(buttonLayout);
    rightLayout->addWidget(new QLabel("人员列表", this));
    rightLayout->addWidget(personTable);
    rightLayout->addWidget(new QLabel("识别结果", this));
    rightLayout->addWidget(resultTextEdit);
    rightLayout->addWidget(new QLabel("识别日志", this));
    rightLayout->addWidget(logTextEdit);

    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->addWidget(imageLabel, 3);
    mainLayout->addLayout(rightLayout, 2);

    connect(importImageButton, &QPushButton::clicked,
            this, &MainWindow::onImportImageClicked);
    connect(registerPersonButton, &QPushButton::clicked,
            this, &MainWindow::onRegisterPersonClicked);
    connect(recognizeButton, &QPushButton::clicked,
            this, &MainWindow::onRecognizeClicked);
    connect(clearButton, &QPushButton::clicked,
            this, &MainWindow::onClearClicked);
    connect(startCameraButton, &QPushButton::clicked,
            this, &MainWindow::onStartCameraClicked);
    connect(stopCameraButton, &QPushButton::clicked,
            this, &MainWindow::onStopCameraClicked);
}

void MainWindow::onImportImageClicked()
{
    QMessageBox::information(this, "提示", "导入图片功能将在 Step 4 实现。");
}

void MainWindow::onRegisterPersonClicked()
{
    QMessageBox::information(this, "提示", "注册人员功能将在 Step 8 实现。");
}

void MainWindow::onRecognizeClicked()
{
    QMessageBox::information(this, "提示", "身份识别功能将在 Step 9 实现。");
}

void MainWindow::onClearClicked()
{
    imageLabel->setText("图片 / 摄像头画面显示区域");
    resultTextEdit->clear();
    logTextEdit->clear();
}

void MainWindow::onStartCameraClicked()
{
    QMessageBox::information(this, "提示", "摄像头功能将在 Step 11 实现。");
}

void MainWindow::onStopCameraClicked()
{
    QMessageBox::information(this, "提示", "摄像头停止功能将在 Step 11 实现。");
}