#ifndef PERSONMANAGEMENTDIALOG_H
#define PERSONMANAGEMENTDIALOG_H

#include "storage/FaceRepository.h"
#include "vision/FaceDetector.h"
#include "vision/FeatureExtractor.h"

#include <QDialog>
#include <QList>
#include <QString>

#include <vector>

class QLineEdit;
class QPushButton;
class QTableWidget;

class PersonManagementDialog : public QDialog
{
    Q_OBJECT

public:
    PersonManagementDialog(FaceRepository *repository,
                           FaceDetector *faceDetector,
                           FeatureExtractor *featureExtractor,
                           QWidget *parent = nullptr);

private slots:
    void onSearchClicked();
    void onResetClicked();
    void onAddClicked();
    void onEditClicked();
    void onDeleteClicked();
    void onTableDoubleClicked(int row, int column);

private:
    void setupUi();
    void loadPersons(const QString &keyword = QString());
    bool selectedPerson(Person *person) const;
    bool extractFaceFeatures(const QStringList &imagePaths,
                             std::vector<std::vector<float>> *features,
                             QString *errorMessage) const;
    std::vector<float> averageFeature(const std::vector<std::vector<float>> &features) const;
    bool saveFaceFeatures(const QString &studentId,
                          const std::vector<std::vector<float>> &features,
                          QString *errorMessage) const;

private:
    FaceRepository *repository;
    FaceDetector *faceDetector;
    FeatureExtractor *featureExtractor;

    QLineEdit *searchEdit;
    QTableWidget *personTable;
    QPushButton *searchButton;
    QPushButton *resetButton;
    QPushButton *addButton;
    QPushButton *editButton;
    QPushButton *deleteButton;
    QPushButton *closeButton;

    QList<Person> currentPersons;
};

#endif
