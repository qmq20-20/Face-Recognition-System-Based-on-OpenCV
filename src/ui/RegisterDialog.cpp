#include "ui/RegisterDialog.h"

#include "config/AppConfig.h"
#include "utils/ImageUtils.h"

#include <QCloseEvent>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <opencv2/imgcodecs.hpp>

RegisterDialog::RegisterDialog(QWidget *parent, bool imageRequired)
    : QDialog(parent),
      cameraTimer(nullptr),
      imageRequired(imageRequired)
{
    setupUi();
}

RegisterDialog::~RegisterDialog()
{
    stopCamera();
}

void RegisterDialog::setupUi()
{
    setWindowTitle(imageRequired ? "新增人员" : "编辑人员");
    resize(680, 560);

    nameEdit = new QLineEdit(this);
    studentIdEdit = new QLineEdit(this);
    departmentEdit = new QLineEdit(this);

    QFormLayout *formLayout = new QFormLayout;
    formLayout->addRow("姓名：", nameEdit);
    formLayout->addRow("学号：", studentIdEdit);
    formLayout->addRow("部门：", departmentEdit);

    imageListWidget = new QListWidget(this);
    imageListWidget->setMinimumHeight(120);

    importImagesButton = new QPushButton(imageRequired ? "导入照片" : "导入新照片", this);
    removeImageButton = new QPushButton("移除选中", this);

    QHBoxLayout *imageButtonLayout = new QHBoxLayout;
    imageButtonLayout->addWidget(importImagesButton);
    imageButtonLayout->addWidget(removeImageButton);
    imageButtonLayout->addStretch();

    previewLabel = new QLabel("摄像头预览", this);
    previewLabel->setAlignment(Qt::AlignCenter);
    previewLabel->setMinimumSize(420, 240);
    previewLabel->setStyleSheet(
        "QLabel {"
        "border: 1px solid #999;"
        "background-color: #f5f5f5;"
        "color: #666;"
        "}");

    startCameraButton = new QPushButton("打开摄像头", this);
    capturePhotoButton = new QPushButton("拍照", this);
    stopCameraButton = new QPushButton("停止摄像头", this);

    capturePhotoButton->setEnabled(false);
    stopCameraButton->setEnabled(false);

    QHBoxLayout *cameraButtonLayout = new QHBoxLayout;
    cameraButtonLayout->addWidget(startCameraButton);
    cameraButtonLayout->addWidget(capturePhotoButton);
    cameraButtonLayout->addWidget(stopCameraButton);
    cameraButtonLayout->addStretch();

    QVBoxLayout *photoLayout = new QVBoxLayout;
    photoLayout->addWidget(new QLabel(imageRequired ? "人脸照片：" : "人脸照片（可选）：", this));
    photoLayout->addWidget(imageListWidget);
    photoLayout->addLayout(imageButtonLayout);
    photoLayout->addWidget(previewLabel);
    photoLayout->addLayout(cameraButtonLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        this);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(photoLayout);
    mainLayout->addWidget(buttonBox);

    cameraTimer = new QTimer(this);
    cameraTimer->setInterval(AppConfig::cameraFrameIntervalMs());

    connect(importImagesButton, &QPushButton::clicked,
            this, &RegisterDialog::onImportImagesClicked);
    connect(removeImageButton, &QPushButton::clicked,
            this, &RegisterDialog::onRemoveSelectedImageClicked);
    connect(startCameraButton, &QPushButton::clicked,
            this, &RegisterDialog::onStartCameraClicked);
    connect(capturePhotoButton, &QPushButton::clicked,
            this, &RegisterDialog::onCapturePhotoClicked);
    connect(stopCameraButton, &QPushButton::clicked,
            this, &RegisterDialog::onStopCameraClicked);
    connect(cameraTimer, &QTimer::timeout,
            this, &RegisterDialog::updateCameraFrame);
    connect(buttonBox, &QDialogButtonBox::accepted,
            this, &RegisterDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected,
            this, &RegisterDialog::reject);
}

void RegisterDialog::setPersonInfo(const QString &name,
                                   const QString &studentId,
                                   const QString &department)
{
    nameEdit->setText(name);
    studentIdEdit->setText(studentId);
    departmentEdit->setText(department);
}

QString RegisterDialog::name() const
{
    return nameEdit->text().trimmed();
}

QString RegisterDialog::studentId() const
{
    return studentIdEdit->text().trimmed();
}

QString RegisterDialog::department() const
{
    return departmentEdit->text().trimmed();
}

QStringList RegisterDialog::imagePaths() const
{
    QStringList paths;

    for (int row = 0; row < imageListWidget->count(); ++row)
    {
        paths.append(imageListWidget->item(row)->data(Qt::UserRole).toString());
    }

    return paths;
}

void RegisterDialog::closeEvent(QCloseEvent *event)
{
    stopCamera();
    QDialog::closeEvent(event);
}

void RegisterDialog::onImportImagesClicked()
{
    QStringList filePaths = QFileDialog::getOpenFileNames(
        this,
        "选择人脸照片",
        QString(),
        "Images (*.png *.jpg *.jpeg *.bmp)");

    for (const QString &filePath : filePaths)
    {
        addImagePath(filePath);
    }
}

void RegisterDialog::onRemoveSelectedImageClicked()
{
    delete imageListWidget->takeItem(imageListWidget->currentRow());
}

void RegisterDialog::onStartCameraClicked()
{
    if (camera.isOpened())
    {
        return;
    }

    if (!camera.open(0))
    {
        QMessageBox::warning(this, "摄像头打开失败", "无法打开默认摄像头，请检查摄像头是否存在或被其他程序占用。");
        return;
    }

    startCameraButton->setEnabled(false);
    capturePhotoButton->setEnabled(true);
    stopCameraButton->setEnabled(true);
    cameraTimer->start();
}

void RegisterDialog::onCapturePhotoClicked()
{
    if (currentFrame.empty())
    {
        QMessageBox::warning(this, "拍照失败", "当前没有可保存的摄像头画面。");
        return;
    }

    QString captureDir = AppConfig::dataDirectory() + "/captures";
    QDir().mkpath(captureDir);

    QString fileName = "face_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz") + ".jpg";
    QString filePath = captureDir + "/" + fileName;

    if (!cv::imwrite(filePath.toStdString(), currentFrame))
    {
        QMessageBox::warning(this, "拍照失败", "摄像头照片保存失败。");
        return;
    }

    addImagePath(filePath);
}

void RegisterDialog::onStopCameraClicked()
{
    stopCamera();
}

void RegisterDialog::updateCameraFrame()
{
    if (!camera.isOpened())
    {
        return;
    }

    cv::Mat frame;
    camera.read(frame);

    if (frame.empty())
    {
        return;
    }

    currentFrame = frame.clone();
    displayPreview(currentFrame);
}

void RegisterDialog::addImagePath(const QString &filePath)
{
    if (filePath.isEmpty())
    {
        return;
    }

    for (int row = 0; row < imageListWidget->count(); ++row)
    {
        if (imageListWidget->item(row)->data(Qt::UserRole).toString() == filePath)
        {
            return;
        }
    }

    QFileInfo fileInfo(filePath);
    QListWidgetItem *item = new QListWidgetItem(fileInfo.fileName(), imageListWidget);
    item->setToolTip(filePath);
    item->setData(Qt::UserRole, filePath);
}

void RegisterDialog::stopCamera()
{
    if (cameraTimer && cameraTimer->isActive())
    {
        cameraTimer->stop();
    }

    if (camera.isOpened())
    {
        camera.release();
    }

    currentFrame.release();

    if (startCameraButton)
    {
        startCameraButton->setEnabled(true);
    }

    if (capturePhotoButton)
    {
        capturePhotoButton->setEnabled(false);
    }

    if (stopCameraButton)
    {
        stopCameraButton->setEnabled(false);
    }
}

void RegisterDialog::displayPreview(const cv::Mat &frame)
{
    QImage image = ImageUtils::matToQImage(frame);

    if (image.isNull())
    {
        previewLabel->setText("摄像头预览失败");
        return;
    }

    previewLabel->setPixmap(
        QPixmap::fromImage(image).scaled(
            previewLabel->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation));
}
