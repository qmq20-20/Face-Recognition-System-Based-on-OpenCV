#include "ui/MainWindow.h"

#include "config/AppConfig.h"
#include "ui/PersonManagementDialog.h"
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
#include <QtConcurrent/QtConcurrentRun>
#include <vector>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      recognitionService(AppConfig::defaultSimilarityThreshold()),
      cameraTimer(nullptr),
      cameraFrameCount(0),
      cameraRecognitionWatcher(nullptr),
      cameraSessionId(0),
      hasCameraRecognitionOutput(false),
      cameraOutputHasKnownFeatures(false)
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
    managePersonsButton = new QPushButton("人员管理", this);
    recognizeButton = new QPushButton("开始识别", this);
    clearButton = new QPushButton("清空", this);
    startCameraButton = new QPushButton("开始摄像头", this);
    stopCameraButton = new QPushButton("停止摄像头", this);

    stopCameraButton->setEnabled(false);

    QVBoxLayout *buttonLayout = new QVBoxLayout;
    buttonLayout->addWidget(importImageButton);
    buttonLayout->addWidget(managePersonsButton);
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
    connect(managePersonsButton, &QPushButton::clicked,
            this, &MainWindow::onManagePersonsClicked);
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

    cameraRecognitionWatcher = new QFutureWatcher<CameraRecognitionResult>(this);
    connect(cameraRecognitionWatcher, &QFutureWatcher<CameraRecognitionResult>::finished,
            this, &MainWindow::onCameraRecognitionFinished);

    const bool haarLoaded = faceDetector.loadModel(AppConfig::faceCascadeModelPath());
    const bool yoloLoaded = faceDetector.loadYoloModel(AppConfig::yoloFaceDetectorModelPath());

    if (!faceDetector.isLoaded())
    {
        QMessageBox::warning(
            this,
            "模型加载失败",
            "无法加载 YOLO 或 Haar 人脸检测模型。\n\nYOLO：" +
                AppConfig::yoloFaceDetectorModelPath() + "\nHaar：" +
                AppConfig::faceCascadeModelPath());
    }
    else if (!yoloLoaded && haarLoaded)
    {
        resultTextEdit->setText(
            "未找到或无法加载 YOLO ONNX 模型，当前使用 Haar Cascade 检测器。\n" +
            AppConfig::yoloFaceDetectorModelPath() + "\n原因：" + faceDetector.yoloLoadError());
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

    QString initializationText =
        "数据库初始化成功：\n" + databasePath + "\n当前检测器：" + faceDetector.activeModelName();

    if (faceDetector.activeModelName() == "Haar Cascade" && !faceDetector.yoloLoadError().isEmpty())
    {
        initializationText += "\nYOLO 加载失败原因：" + faceDetector.yoloLoadError();
    }

    resultTextEdit->setText(initializationText);
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

void MainWindow::onManagePersonsClicked()
{
    PersonManagementDialog dialog(&repository, &faceDetector, &featureExtractor, this);
    dialog.exec();
    refreshPersonTable();
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
        QMessageBox::warning(this, "提示", "请先在人员管理中新增至少一名人员。");
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

    if (!faceDetector.isLoaded())
    {
        QMessageBox::warning(this, "摄像头无法启动", "未加载可用的人脸检测模型。");
        return;
    }

    if (!camera.open(0))
    {
        QMessageBox::warning(this, "摄像头打开失败", "无法打开默认摄像头，请检查摄像头是否存在或被其他程序占用。");
        return;
    }

    cameraFrameCount = 0;
    ++cameraSessionId;
    hasCameraRecognitionOutput = false;
    cameraOutputHasKnownFeatures = false;
    latestCameraRecognitionResults.clear();
    currentImagePath = "camera";
    cameraPersons = repository.getAllPersons();
    cameraKnownFeatures = repository.getAllFaceFeatures();
    cameraFaceDetector = std::make_shared<FaceDetector>();
    cameraFaceDetector->loadModel(AppConfig::faceCascadeModelPath());
    cameraFaceDetector->loadYoloModel(AppConfig::yoloFaceDetectorModelPath());

    startCameraButton->setEnabled(false);
    stopCameraButton->setEnabled(true);

    cameraTimer->start();

    QString cameraStatus = QString("摄像头已启动，正在后台连续检测。\n检测器：%1")
                               .arg(cameraFaceDetector->activeModelName());

    if (cameraFaceDetector->activeModelName() == "Haar Cascade" &&
        !cameraFaceDetector->yoloLoadError().isEmpty())
    {
        cameraStatus += "\nYOLO 加载失败原因：\n" + cameraFaceDetector->yoloLoadError();
    }

    resultTextEdit->setText(cameraStatus);
}

void MainWindow::onStopCameraClicked()
{
    ++cameraSessionId;

    if (cameraTimer && cameraTimer->isActive())
    {
        cameraTimer->stop();
    }

    if (camera.isOpened())
    {
        camera.release();
    }

    // 正在执行的后台任务会持有自己的模型副本；停止后其结果会按会话编号直接丢弃。
    cameraFaceDetector.reset();
    cameraPersons.clear();
    cameraKnownFeatures.clear();
    latestCameraRecognitionResults.clear();
    hasCameraRecognitionOutput = false;
    cameraOutputHasKnownFeatures = false;

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
    ++cameraFrameCount;

    if (!cameraRecognitionWatcher->isRunning() && cameraFaceDetector)
    {
        const bool shouldWriteLog =
            cameraFrameCount % AppConfig::cameraLogFrameInterval() == 0;
        queueCameraRecognition(currentImage, shouldWriteLog);
    }

    // 始终在最新画面上绘制最近结果，避免旧帧和新帧交替显示造成闪烁。
    if (!hasCameraRecognitionOutput)
    {
        displayImage(currentImage);
    }
    else if (cameraOutputHasKnownFeatures)
    {
        displayImageWithRecognitionResults(
            currentImage,
            currentFaces,
            latestCameraRecognitionResults);
    }
    else
    {
        displayImageWithFaces(currentImage, currentFaces);
    }
}

void MainWindow::queueCameraRecognition(const cv::Mat &frame, bool shouldWriteLog)
{
    const cv::Mat frameCopy = frame.clone();
    const QList<Person> persons = cameraPersons;
    const QList<FaceFeatureRecord> knownFeatures = cameraKnownFeatures;
    const std::shared_ptr<FaceDetector> detector = cameraFaceDetector;
    const double threshold = recognitionService.threshold();
    const quint64 sessionId = cameraSessionId;

    cameraRecognitionWatcher->setFuture(QtConcurrent::run(
        [frameCopy, persons, knownFeatures, detector, threshold, shouldWriteLog, sessionId]() {
            CameraRecognitionResult output;
            output.frame = frameCopy;
            output.cameraSessionId = sessionId;
            output.shouldWriteLog = shouldWriteLog;
            output.hasKnownFeatures = !persons.isEmpty() && !knownFeatures.isEmpty();

            try
            {
                output.faces = detector->detect(frameCopy);
                output.detectorName = detector->lastDetectionModelName();
                output.detectorFallbackReason = detector->yoloInferenceError();
            }
            catch (const std::exception &error)
            {
                output.detectorName = "检测任务失败";
                output.processingError = QString::fromStdString(error.what());
                return output;
            }

            if (!output.hasKnownFeatures)
            {
                return output;
            }

            FeatureExtractor extractor;
            RecognitionService service(threshold);

            for (const cv::Rect &face : output.faces)
            {
                const cv::Mat faceImage = ImageUtils::cropFace(frameCopy, face);
                const std::vector<float> feature = extractor.extract(faceImage);
                output.recognitionResults.append(
                    service.recognize(feature, persons, knownFeatures));
            }

            return output;
        }));
}

void MainWindow::onCameraRecognitionFinished()
{
    const CameraRecognitionResult output = cameraRecognitionWatcher->result();

    if (!camera.isOpened() || output.cameraSessionId != cameraSessionId)
    {
        return;
    }

    currentFaces = output.faces;
    latestCameraRecognitionResults = output.recognitionResults;
    hasCameraRecognitionOutput = true;
    cameraOutputHasKnownFeatures = output.hasKnownFeatures;

    if (!output.hasKnownFeatures)
    {
        displayImageWithFaces(output.frame, output.faces);
        QString resultText =
            QString("摄像头实时检测中。\n检测器：%1\n检测到 %2 张人脸。\n请先在人员管理中新增人员后再进行身份识别。")
                .arg(output.detectorName)
                .arg(output.faces.size());

        if (!output.detectorFallbackReason.isEmpty())
        {
            resultText += "\n\nYOLO 推理失败原因：\n" + output.detectorFallbackReason;
        }

        if (!output.processingError.isEmpty())
        {
            resultText += "\n\n检测任务失败原因：\n" + output.processingError;
        }

        resultTextEdit->setText(resultText);
        return;
    }

    QString resultText = QString("摄像头实时识别中。\n检测器：%1\n检测到 %2 张人脸。\n\n")
                             .arg(output.detectorName)
                             .arg(output.faces.size());

    for (int i = 0; i < output.recognitionResults.size(); ++i)
    {
        const RecognitionResult &result = output.recognitionResults[i];
        resultText += QString("人脸 %1：%2，相似度：%3\n")
                          .arg(i + 1)
                          .arg(result.personName)
                          .arg(result.similarity, 0, 'f', 2);

        if (output.shouldWriteLog)
        {
            repository.addRecognitionLog(result.personName, result.similarity, "camera");
        }
    }

    if (!output.detectorFallbackReason.isEmpty())
    {
        resultText += "\nYOLO 推理失败原因：\n" + output.detectorFallbackReason;
    }

    if (!output.processingError.isEmpty())
    {
        resultText += "\n检测任务失败原因：\n" + output.processingError;
    }

    resultTextEdit->setText(resultText);

    if (output.shouldWriteLog)
    {
        refreshLogView();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    onStopCameraClicked();
    QMainWindow::closeEvent(event);
}
