#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

// RegisterDialog.h
// 人员注册对话框声明文件。
// 负责收集姓名、学号、部门和注册图片路径。

#include <QDialog>
#include <QString>
#include <QStringList>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>

class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QTimer;
class QCloseEvent;

class RegisterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterDialog(QWidget *parent = nullptr, bool imageRequired = true);
    ~RegisterDialog();

    void setPersonInfo(const QString &name,
                       const QString &studentId,
                       const QString &department);
    QString name() const;
    QString studentId() const;
    QString department() const;
    QStringList imagePaths() const;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onImportImagesClicked();
    void onRemoveSelectedImageClicked();
    void onStartCameraClicked();
    void onCapturePhotoClicked();
    void onStopCameraClicked();
    void updateCameraFrame();

private:
    void setupUi();
    void addImagePath(const QString &filePath);
    void stopCamera();
    void displayPreview(const cv::Mat &frame);

private:
    QLineEdit *nameEdit;
    QLineEdit *studentIdEdit;
    QLineEdit *departmentEdit;
    QListWidget *imageListWidget;
    QLabel *previewLabel;
    QPushButton *importImagesButton;
    QPushButton *removeImageButton;
    QPushButton *startCameraButton;
    QPushButton *capturePhotoButton;
    QPushButton *stopCameraButton;
    QTimer *cameraTimer;
    cv::VideoCapture camera;
    cv::Mat currentFrame;
    bool imageRequired;
};

#endif
