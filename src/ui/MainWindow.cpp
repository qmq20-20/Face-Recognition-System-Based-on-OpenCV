#include "ui/MainWindow.h"

#include "config/AppConfig.h"
#include "ui/RegisterDialog.h"
#include "utils/ImageUtils.h"

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
#include <QDir>
#include <QTableWidgetItem>
#include <QPainter>
#include <QPen>
#include <QCloseEvent>
#include <QTimer>
#include <vector>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      recognitionService(AppConfig::defaultSimilarityThreshold()),
      cameraTimer(nullptr),
      cameraFrameCount(0)
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

    stopCameraButton->setEnabled(false);

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

    cameraTimer = new QTimer(this);
    cameraTimer->setInterval(AppConfig::cameraFrameIntervalMs());

    connect(cameraTimer, &QTimer::timeout,
            this, &MainWindow::processCameraFrame);

    QString modelPath = AppConfig::faceCascadeModelPath();

    if (!faceDetector.loadModel(modelPath))
    {
        QMessageBox::warning(this, "模型加载失败", "无法加载人脸检测模型：\n" + modelPath);
    }
    initializeDatabase();
}
void MainWindow::initializeDatabase()
{
    QString dataDir = AppConfig::dataDirectory();
    QDir().mkpath(dataDir);

    QString databasePath = AppConfig::databasePath();

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

    if (logs.isEmpty())
    {
        logTextEdit->setPlaceholderText("暂无识别日志");
        return;
    }

    for (const RecognitionLog &log : logs)
    {
        QString line = QString("[%1] %2 | 相似度：%3 | 来源：%4")
                           .arg(log.recognizedAt)
                           .arg(log.personName)
                           .arg(log.similarity, 0, 'f', 2)
                           .arg(log.imagePath);

        logTextEdit->append(line);
    }
}
void MainWindow::displayImageWithFaces(const cv::Mat &mat, const std::vector<cv::Rect> &faces)
{
    QImage image = ImageUtils::matToQImage(mat);

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

void MainWindow::displayImage(const cv::Mat &mat)
{
    QImage image = ImageUtils::matToQImage(mat);

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
    if (camera.isOpened())
    {
        onStopCameraClicked();
    }

    QString filePath = QFileDialog::getOpenFileName(
        this,
        "选择图片",
        QString(),
        "Images (*.png *.jpg *.jpeg *.bmp)");

    if (filePath.isEmpty())
    {
        return;
    }

    cv::Mat image = ImageUtils::readImageFromFile(filePath);

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

    cv::Mat image = ImageUtils::readImageFromFile(imagePath);

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

    cv::Mat faceImage = ImageUtils::cropFace(image, largestFace);
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
void MainWindow::displayImageWithRecognitionResults(
    const cv::Mat &mat,
    const std::vector<cv::Rect> &faces,
    const QList<RecognitionResult> &results)
{
    QImage image = ImageUtils::matToQImage(mat);

    if (image.isNull())
    {
        imageLabel->setText("图片显示失败");
        return;
    }

    QPainter painter(&image);
    painter.setPen(QPen(Qt::red, 3));

    for (int i = 0; i < static_cast<int>(faces.size()); ++i)
    {
        const cv::Rect &face = faces[i];

        painter.drawRect(face.x, face.y, face.width, face.height);

        QString label = "Face";

        if (i < results.size())
        {
            const RecognitionResult &result = results[i];

            label = QString("%1 %2")
                        .arg(result.personName)
                        .arg(result.similarity, 0, 'f', 2);
        }

        int textY = face.y > 10 ? face.y - 5 : face.y + 20;
        painter.drawText(face.x, textY, label);
    }

    painter.end();

    imageLabel->setPixmap(
        QPixmap::fromImage(image).scaled(
            imageLabel->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation));
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

    QList<Person> persons = repository.getAllPersons();
    QList<FaceFeatureRecord> knownFeatures = repository.getAllFaceFeatures();

    if (persons.isEmpty() || knownFeatures.isEmpty())
    {
        QMessageBox::warning(this, "提示", "请先注册至少一名人员。");
        return;
    }

    currentFaces = faceDetector.detect(currentImage);

    if (currentFaces.empty())
    {
        displayImage(currentImage);
        resultTextEdit->setText("未检测到人脸。");
        return;
    }

    QList<RecognitionResult> results;
    QString resultText;

    resultText += QString("检测到 %1 张人脸。\n").arg(currentFaces.size());
    resultText += QString("识别阈值：%1\n\n").arg(recognitionService.threshold(), 0, 'f', 2);

    for (int i = 0; i < static_cast<int>(currentFaces.size()); ++i)
    {
        cv::Mat faceImage = ImageUtils::cropFace(currentImage, currentFaces[i]);
        std::vector<float> feature = featureExtractor.extract(faceImage);

        RecognitionResult result = recognitionService.recognize(
            feature,
            persons,
            knownFeatures);

        results.append(result);

        resultText += QString("人脸 %1：%2，相似度：%3\n")
                          .arg(i + 1)
                          .arg(result.personName)
                          .arg(result.similarity, 0, 'f', 2);

        repository.addRecognitionLog(
            result.personName,
            result.similarity,
            currentImagePath);
    }

    displayImageWithRecognitionResults(currentImage, currentFaces, results);
    resultTextEdit->setText(resultText);
    refreshLogView();
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
    if (camera.isOpened())
    {
        QMessageBox::information(this, "提示", "摄像头已经在运行。");
        return;
    }

    if (!camera.open(0))
    {
        QMessageBox::warning(this, "摄像头打开失败", "无法打开默认摄像头，请检查摄像头是否存在或被其他程序占用。");
        return;
    }

    cameraFrameCount = 0;
    currentImagePath = "camera";

    startCameraButton->setEnabled(false);
    stopCameraButton->setEnabled(true);

    cameraTimer->start();

    resultTextEdit->setText("摄像头已启动，正在实时检测。");
}

void MainWindow::onStopCameraClicked()
{
    if (cameraTimer && cameraTimer->isActive())
    {
        cameraTimer->stop();
    }

    if (camera.isOpened())
    {
        camera.release();
    }

    startCameraButton->setEnabled(true);
    stopCameraButton->setEnabled(false);

    resultTextEdit->append("摄像头已停止。");
}

void MainWindow::processCameraFrame()
{
    if (!camera.isOpened())
    {
        return;
    }

    cv::Mat frame;
    camera.read(frame);

    if (frame.empty())
    {
        resultTextEdit->setText("摄像头画面读取失败。");
        return;
    }

    currentImage = frame.clone();
    currentFaces = faceDetector.detect(currentImage);

    QList<Person> persons = repository.getAllPersons();
    QList<FaceFeatureRecord> knownFeatures = repository.getAllFaceFeatures();

    if (persons.isEmpty() || knownFeatures.isEmpty())
    {
        displayImageWithFaces(currentImage, currentFaces);

        resultTextEdit->setText(
            QString("摄像头实时检测中。\n检测到 %1 张人脸。\n请先注册人员后再进行身份识别。")
                .arg(currentFaces.size()));

        return;
    }

    QList<RecognitionResult> results;
    QString resultText;

    resultText += QString("摄像头实时识别中。\n检测到 %1 张人脸。\n\n")
                      .arg(currentFaces.size());

    ++cameraFrameCount;
    bool shouldWriteLog = (cameraFrameCount % AppConfig::cameraLogFrameInterval() == 0);

    for (int i = 0; i < static_cast<int>(currentFaces.size()); ++i)
    {
        cv::Mat faceImage = ImageUtils::cropFace(currentImage, currentFaces[i]);
        std::vector<float> feature = featureExtractor.extract(faceImage);

        RecognitionResult result = recognitionService.recognize(
            feature,
            persons,
            knownFeatures);

        results.append(result);

        resultText += QString("人脸 %1：%2，相似度：%3\n")
                          .arg(i + 1)
                          .arg(result.personName)
                          .arg(result.similarity, 0, 'f', 2);

        if (shouldWriteLog)
        {
            repository.addRecognitionLog(
                result.personName,
                result.similarity,
                "camera");
        }
    }

    displayImageWithRecognitionResults(currentImage, currentFaces, results);
    resultTextEdit->setText(resultText);

    if (shouldWriteLog)
    {
        refreshLogView();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    onStopCameraClicked();
    QMainWindow::closeEvent(event);
}
