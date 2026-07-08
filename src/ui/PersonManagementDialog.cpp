#include "ui/PersonManagementDialog.h"

#include "ui/RegisterDialog.h"
#include "utils/ImageUtils.h"

#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

PersonManagementDialog::PersonManagementDialog(FaceRepository *repository,
                                               FaceDetector *faceDetector,
                                               FeatureExtractor *featureExtractor,
                                               QWidget *parent)
    : QDialog(parent),
      repository(repository),
      faceDetector(faceDetector),
      featureExtractor(featureExtractor)
{
    setupUi();
    loadPersons();
}

void PersonManagementDialog::setupUi()
{
    setWindowTitle("人员信息管理");
    resize(760, 520);

    searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText("输入姓名、学号或部门查询");

    searchButton = new QPushButton("查询", this);
    resetButton = new QPushButton("重置", this);

    QHBoxLayout *searchLayout = new QHBoxLayout;
    searchLayout->addWidget(new QLabel("关键字：", this));
    searchLayout->addWidget(searchEdit);
    searchLayout->addWidget(searchButton);
    searchLayout->addWidget(resetButton);

    personTable = new QTableWidget(this);
    personTable->setColumnCount(4);
    personTable->setHorizontalHeaderLabels(QStringList() << "学号" << "姓名" << "部门" << "创建时间");
    personTable->horizontalHeader()->setStretchLastSection(true);
    personTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    personTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    personTable->setSelectionMode(QAbstractItemView::SingleSelection);

    addButton = new QPushButton("新增", this);
    editButton = new QPushButton("编辑", this);
    deleteButton = new QPushButton("删除", this);
    closeButton = new QPushButton("关闭", this);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(editButton);
    buttonLayout->addWidget(deleteButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(searchLayout);
    mainLayout->addWidget(personTable);
    mainLayout->addLayout(buttonLayout);

    connect(searchButton, &QPushButton::clicked,
            this, &PersonManagementDialog::onSearchClicked);
    connect(resetButton, &QPushButton::clicked,
            this, &PersonManagementDialog::onResetClicked);
    connect(addButton, &QPushButton::clicked,
            this, &PersonManagementDialog::onAddClicked);
    connect(editButton, &QPushButton::clicked,
            this, &PersonManagementDialog::onEditClicked);
    connect(deleteButton, &QPushButton::clicked,
            this, &PersonManagementDialog::onDeleteClicked);
    connect(closeButton, &QPushButton::clicked,
            this, &PersonManagementDialog::accept);
    connect(personTable, &QTableWidget::cellDoubleClicked,
            this, &PersonManagementDialog::onTableDoubleClicked);
}

void PersonManagementDialog::loadPersons(const QString &keyword)
{
    currentPersons = repository->searchPersons(keyword);
    personTable->setRowCount(currentPersons.size());

    for (int row = 0; row < currentPersons.size(); ++row)
    {
        const Person &person = currentPersons[row];

        personTable->setItem(row, 0, new QTableWidgetItem(person.studentId));
        personTable->setItem(row, 1, new QTableWidgetItem(person.name));
        personTable->setItem(row, 2, new QTableWidgetItem(person.department));
        personTable->setItem(row, 3, new QTableWidgetItem(person.createdAt));
    }

    personTable->resizeColumnsToContents();
    personTable->horizontalHeader()->setStretchLastSection(true);
}

bool PersonManagementDialog::selectedPerson(Person *person) const
{
    int row = personTable->currentRow();

    if (row < 0 || row >= currentPersons.size())
    {
        return false;
    }

    if (person)
    {
        *person = currentPersons[row];
    }

    return true;
}

bool PersonManagementDialog::extractFaceFeature(const QString &imagePath,
                                                std::vector<float> *feature,
                                                QString *errorMessage) const
{
    cv::Mat image = ImageUtils::readImageFromFile(imagePath);

    if (image.empty())
    {
        *errorMessage = "图片读取失败，请选择有效的图片文件。";
        return false;
    }

    if (!faceDetector->isLoaded())
    {
        *errorMessage = "人脸检测模型尚未加载成功。";
        return false;
    }

    std::vector<cv::Rect> faces = faceDetector->detect(image);

    if (faces.empty())
    {
        *errorMessage = "图片中未检测到人脸。";
        return false;
    }

    cv::Rect largestFace = faces[0];

    for (const cv::Rect &face : faces)
    {
        if (face.area() > largestFace.area())
        {
            largestFace = face;
        }
    }

    cv::Mat faceImage = ImageUtils::cropFace(image, largestFace);
    *feature = featureExtractor->extract(faceImage);

    if (feature->empty())
    {
        *errorMessage = "人脸特征提取失败。";
        return false;
    }

    return true;
}

void PersonManagementDialog::onSearchClicked()
{
    loadPersons(searchEdit->text());
}

void PersonManagementDialog::onResetClicked()
{
    searchEdit->clear();
    loadPersons();
}

void PersonManagementDialog::onAddClicked()
{
    RegisterDialog dialog(this, true);

    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    if (dialog.name().isEmpty())
    {
        QMessageBox::warning(this, "新增失败", "姓名不能为空。");
        return;
    }

    if (dialog.studentId().isEmpty())
    {
        QMessageBox::warning(this, "新增失败", "学号不能为空。");
        return;
    }

    if (repository->studentIdExists(dialog.studentId()))
    {
        QMessageBox::warning(this, "新增失败", "该学号已存在，请使用唯一学号。");
        return;
    }

    if (dialog.imagePath().isEmpty())
    {
        QMessageBox::warning(this, "新增失败", "请选择一张人脸图片。");
        return;
    }

    std::vector<float> feature;
    QString errorMessage;

    if (!extractFaceFeature(dialog.imagePath(), &feature, &errorMessage))
    {
        QMessageBox::warning(this, "新增失败", errorMessage);
        return;
    }

    if (!repository->addPerson(dialog.name(), dialog.studentId(), dialog.department()))
    {
        QMessageBox::warning(this, "新增失败", "人员信息保存失败：\n" + repository->lastError());
        return;
    }

    if (!repository->addFaceFeature(dialog.studentId(), feature))
    {
        repository->deletePerson(dialog.studentId());
        QMessageBox::warning(this, "新增失败", "人脸特征保存失败：\n" + repository->lastError());
        return;
    }

    loadPersons(searchEdit->text());
    QMessageBox::information(this, "新增成功", "人员信息已保存。");
}

void PersonManagementDialog::onEditClicked()
{
    Person person;

    if (!selectedPerson(&person))
    {
        QMessageBox::information(this, "提示", "请先选择一名人员。");
        return;
    }

    RegisterDialog dialog(this, false);
    dialog.setPersonInfo(person.name, person.studentId, person.department);

    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    if (dialog.name().isEmpty())
    {
        QMessageBox::warning(this, "编辑失败", "姓名不能为空。");
        return;
    }

    if (dialog.studentId().isEmpty())
    {
        QMessageBox::warning(this, "编辑失败", "学号不能为空。");
        return;
    }

    if (dialog.studentId() != person.studentId && repository->studentIdExists(dialog.studentId()))
    {
        QMessageBox::warning(this, "编辑失败", "该学号已存在，请使用唯一学号。");
        return;
    }

    if (!repository->updatePerson(person.studentId, dialog.name(), dialog.studentId(), dialog.department()))
    {
        QMessageBox::warning(this, "编辑失败", "人员信息更新失败：\n" + repository->lastError());
        return;
    }

    if (!dialog.imagePath().isEmpty())
    {
        std::vector<float> feature;
        QString errorMessage;

        if (!extractFaceFeature(dialog.imagePath(), &feature, &errorMessage))
        {
            QMessageBox::warning(this, "人脸更新失败", errorMessage);
            loadPersons(searchEdit->text());
            return;
        }

        if (!repository->addFaceFeature(dialog.studentId(), feature))
        {
            QMessageBox::warning(this, "人脸更新失败", "人脸特征保存失败：\n" + repository->lastError());
            loadPersons(searchEdit->text());
            return;
        }
    }

    loadPersons(searchEdit->text());
    QMessageBox::information(this, "编辑成功", "人员信息已更新。");
}

void PersonManagementDialog::onDeleteClicked()
{
    Person person;

    if (!selectedPerson(&person))
    {
        QMessageBox::information(this, "提示", "请先选择一名人员。");
        return;
    }

    int answer = QMessageBox::question(
        this,
        "确认删除",
        QString("确定要删除人员“%1”吗？\n对应人脸特征也会一并删除。").arg(person.name));

    if (answer != QMessageBox::Yes)
    {
        return;
    }

    if (!repository->deletePerson(person.studentId))
    {
        QMessageBox::warning(this, "删除失败", "人员删除失败：\n" + repository->lastError());
        return;
    }

    loadPersons(searchEdit->text());
}

void PersonManagementDialog::onTableDoubleClicked(int row, int column)
{
    Q_UNUSED(row);
    Q_UNUSED(column);

    onEditClicked();
}
