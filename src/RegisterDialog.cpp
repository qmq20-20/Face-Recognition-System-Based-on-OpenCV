#include "RegisterDialog.h"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

RegisterDialog::RegisterDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi();
}

void RegisterDialog::setupUi()
{
    setWindowTitle("注册人员");
    resize(460, 220);

    nameEdit = new QLineEdit(this);
    studentIdEdit = new QLineEdit(this);
    departmentEdit = new QLineEdit(this);
    imagePathEdit = new QLineEdit(this);
    chooseImageButton = new QPushButton("选择图片", this);

    imagePathEdit->setReadOnly(true);

    QHBoxLayout *imageLayout = new QHBoxLayout;
    imageLayout->addWidget(imagePathEdit);
    imageLayout->addWidget(chooseImageButton);

    QFormLayout *formLayout = new QFormLayout;
    formLayout->addRow("姓名：", nameEdit);
    formLayout->addRow("学号：", studentIdEdit);
    formLayout->addRow("部门：", departmentEdit);
    formLayout->addRow("人脸图片：", imageLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        this);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(buttonBox);

    connect(chooseImageButton, &QPushButton::clicked,
            this, &RegisterDialog::onChooseImageClicked);
    connect(buttonBox, &QDialogButtonBox::accepted,
            this, &RegisterDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected,
            this, &RegisterDialog::reject);
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

QString RegisterDialog::imagePath() const
{
    return imagePathEdit->text().trimmed();
}

void RegisterDialog::onChooseImageClicked()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "选择注册图片",
        QString(),
        "Images (*.png *.jpg *.jpeg *.bmp)");

    if (!filePath.isEmpty())
    {
        imagePathEdit->setText(filePath);
    }
}