#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

// RegisterDialog.h
// 人员注册对话框声明文件。
// 负责收集姓名、学号、部门和注册图片路径。

#include <QDialog>
#include <QString>

class QLineEdit;
class QPushButton;

class RegisterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterDialog(QWidget *parent = nullptr, bool imageRequired = true);

    void setPersonInfo(const QString &name,
                       const QString &studentId,
                       const QString &department);
    QString name() const;
    QString studentId() const;
    QString department() const;
    QString imagePath() const;

private slots:
    void onChooseImageClicked();

private:
    void setupUi();

private:
    QLineEdit *nameEdit;
    QLineEdit *studentIdEdit;
    QLineEdit *departmentEdit;
    QLineEdit *imagePathEdit;
    QPushButton *chooseImageButton;
    bool imageRequired;
};

#endif
