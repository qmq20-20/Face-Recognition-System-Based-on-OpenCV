#include "MainWindow.h"
#include "RegisterDialog.h"
#include <QByteArray>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QImage>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <QCoreApplication>
#include <QDir>
#include <QTableWidgetItem>
#include <QCoreApplication>
#include <QPainter>
#include <QPen>

#include <vector>

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

    QString modelPath = QCoreApplication::applicationDirPath() + "/resources/haarcascade_frontalface_default.xml";

    if (!faceDetector.loadModel(modelPath))
    {
        QMessageBox::warning(this, "模型加载失败", "无法加载人脸检测模型：\n" + modelPath);
    }
    initializeDatabase();
}
void MainWindow::initializeDatabase()
{
    QString dataDir = QCoreApplication::applicationDirPath() + "/data";
    QDir().mkpath(dataDir);

    QString databasePath = dataDir + "/face_recognition.db";

    if (!repository.open(databasePath))
    {
        QMessageBox::critical(
            this,
            "数据库初始化失败",
            "无法打开或创建数据库：\n" + databasePath + "\n\n错误信息：\n" + repository.lastError());
        return;
    }

    refreshPersonTable();
    refreshLogView();

    resultTextEdit->setText("数据库初始化成功：\n" + databasePath);
}

void MainWindow::refreshPersonTable()
{
    QList<Person> persons = repository.getAllPersons();

    personTable->setRowCount(persons.size());

    for (int row = 0; row < persons.size(); ++row)
    {
        const Person &person = persons[row];

        personTable->setItem(row, 0, new QTableWidgetItem(person.name));
        personTable->setItem(row, 1, new QTableWidgetItem(person.studentId));
        personTable->setItem(row, 2, new QTableWidgetItem(person.department));
    }
}

void MainWindow::refreshLogView()
{
    QList<RecognitionLog> logs = repository.getRecentLogs(50);

    logTextEdit->clear();

    for (const RecognitionLog &log : logs)
    {
        logTextEdit->append(
            QString("%1 | %2 | 相似度：%3 | %4")
                .arg(log.recognizedAt)
                .arg(log.personName)
                .arg(log.similarity, 0, 'f', 2)
                .arg(log.imagePath));
    }
}
void MainWindow::displayImageWithFaces(const cv::Mat &mat, const std::vector<cv::Rect> &faces)
{
    QImage image = matToQImage(mat);

    if (image.isNull())
    {
        imageLabel->setText("图片显示失败");
        return;
    }

    QPainter painter(&image);
    painter.setPen(QPen(Qt::red, 3));

    // 把 OpenCV 检测到的人脸框画到 Qt 图片上。
    for (const cv::Rect &face : faces)
    {
        painter.drawRect(face.x, face.y, face.width, face.height);
    }

    painter.end();

    imageLabel->setPixmap(
        QPixmap::fromImage(image).scaled(
            imageLabel->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation));
}
cv::Mat MainWindow::readImageFromFile(const QString &filePath)
{
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly))
    {
        return cv::Mat();
    }

    QByteArray imageData = file.readAll();

    std::vector<uchar> buffer(
        reinterpret_cast<const uchar *>(imageData.constData()),
        reinterpret_cast<const uchar *>(imageData.constData()) + imageData.size());

    return cv::imdecode(buffer, cv::IMREAD_COLOR);
}
cv::Mat MainWindow::cropFace(const cv::Mat &image, const cv::Rect &faceRect)
{
    if (image.empty())
    {
        return cv::Mat();
    }

    // 防止人脸框超出图片边界。
    cv::Rect imageRect(0, 0, image.cols, image.rows);
    cv::Rect safeRect = faceRect & imageRect;

    if (safeRect.width <= 0 || safeRect.height <= 0)
    {
        return cv::Mat();
    }

    return image(safeRect).clone();
}
QImage MainWindow::matToQImage(const cv::Mat &mat)
{
    if (mat.empty())
    {
        return QImage();
    }

    cv::Mat rgbImage;

    if (mat.channels() == 3)
    {
        cv::cvtColor(mat, rgbImage, cv::COLOR_BGR2RGB);
        return QImage(
                   rgbImage.data,
                   rgbImage.cols,
                   rgbImage.rows,
                   static_cast<int>(rgbImage.step),
                   QImage::Format_RGB888)
            .copy();
    }

    if (mat.channels() == 1)
    {
        return QImage(
                   mat.data,
                   mat.cols,
                   mat.rows,
                   static_cast<int>(mat.step),
                   QImage::Format_Grayscale8)
            .copy();
    }

    return QImage();
}

void MainWindow::displayImage(const cv::Mat &mat)
{
    QImage image = matToQImage(mat);

    if (image.isNull())
    {
        imageLabel->setText("图片显示失败");
        return;
    }

    QPixmap pixmap = QPixmap::fromImage(image);
    imageLabel->setPixmap(
        pixmap.scaled(
            imageLabel->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation));
}

void MainWindow::onImportImageClicked()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "选择图片",
        QString(),
        "Images (*.png *.jpg *.jpeg *.bmp)");

    if (filePath.isEmpty())
    {
        return;
    }

    cv::Mat image = readImageFromFile(filePath);

    if (image.empty())
    {
        QMessageBox::warning(this, "导入失败", "图片读取失败，请选择有效的图片文件。");
        return;
    }

    currentImagePath = filePath;
    currentImage = image;

    displayImage(currentImage);

    resultTextEdit->setText("图片导入成功：\n" + currentImagePath);
}

void MainWindow::onRegisterPersonClicked()
{
    RegisterDialog dialog(this);

    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    QString name = dialog.name();
    QString studentId = dialog.studentId();
    QString department = dialog.department();
    QString imagePath = dialog.imagePath();

    if (name.isEmpty())
    {
        QMessageBox::warning(this, "注册失败", "姓名不能为空。");
        return;
    }

    if (imagePath.isEmpty())
    {
        QMessageBox::warning(this, "注册失败", "请选择一张人脸图片。");
        return;
    }

    cv::Mat image = readImageFromFile(imagePath);

    if (image.empty())
    {
        QMessageBox::warning(this, "注册失败", "注册图片读取失败。");
        return;
    }

    if (!faceDetector.isLoaded())
    {
        QMessageBox::warning(this, "注册失败", "人脸检测模型尚未加载成功。");
        return;
    }

    std::vector<cv::Rect> faces = faceDetector.detect(image);

    if (faces.empty())
    {
        QMessageBox::warning(this, "注册失败", "注册图片中未检测到人脸。");
        return;
    }

    // 如果一张图片中有多张脸，第一版默认选择面积最大的人脸。
    cv::Rect largestFace = faces[0];

    for (const cv::Rect &face : faces)
    {
        if (face.area() > largestFace.area())
        {
            largestFace = face;
        }
    }

    cv::Mat faceImage = cropFace(image, largestFace);
    std::vector<float> feature = featureExtractor.extract(faceImage);

    if (feature.empty())
    {
        QMessageBox::warning(this, "注册失败", "人脸特征提取失败。");
        return;
    }

    int personId = repository.addPerson(name, studentId, department);

    if (personId <= 0)
    {
        QMessageBox::warning(
            this,
            "注册失败",
            "人员信息保存失败：\n" + repository.lastError());
        return;
    }

    if (!repository.addFaceFeature(personId, feature))
    {
        QMessageBox::warning(
            this,
            "注册失败",
            "人脸特征保存失败：\n" + repository.lastError());
        return;
    }

    refreshPersonTable();

    resultTextEdit->setText(
        QString("注册成功：%1\n特征长度：%2")
            .arg(name)
            .arg(feature.size()));

    QMessageBox::information(this, "注册成功", "人员注册成功。");
}

void MainWindow::onRecognizeClicked()
{
    if (currentImage.empty())
    {
        QMessageBox::warning(this, "提示", "请先导入一张图片。");
        return;
    }

    if (!faceDetector.isLoaded())
    {
        QMessageBox::warning(this, "提示", "人脸检测模型尚未加载成功。");
        return;
    }

    currentFaces = faceDetector.detect(currentImage);
    displayImageWithFaces(currentImage, currentFaces);

    if (currentFaces.empty())
    {
        resultTextEdit->setText("未检测到人脸。");
        return;
    }

    QString resultText;
    resultText += QString("检测到 %1 张人脸。\n").arg(currentFaces.size());
    resultText += QString("每张人脸的特征长度：%1\n\n").arg(featureExtractor.featureLength());

    for (int i = 0; i < static_cast<int>(currentFaces.size()); ++i)
    {
        cv::Mat faceImage = cropFace(currentImage, currentFaces[i]);
        std::vector<float> feature = featureExtractor.extract(faceImage);

        resultText += QString("人脸 %1：特征向量长度 = %2\n")
                          .arg(i + 1)
                          .arg(feature.size());
    }

    resultTextEdit->setText(resultText);
}

void MainWindow::onClearClicked()
{
    currentImagePath.clear();
    currentImage.release();

    imageLabel->clear();
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