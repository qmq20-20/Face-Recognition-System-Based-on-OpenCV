#ifndef FACEREPOSITORY_H
#define FACEREPOSITORY_H

// FaceRepository.h
// 数据库访问模块声明文件。
// 负责管理 SQLite 数据库中的人员信息、人脸特征和识别日志。
// MainWindow 不直接写 SQL，而是通过这个类访问数据库。

#include <QList>
#include <QString>
#include <QSqlDatabase>

#include <vector>

struct Person
{
    int id;
    QString name;
    QString studentId;
    QString department;
    QString createdAt;
};

struct FaceFeatureRecord
{
    int id;
    int personId;
    std::vector<float> feature;
    QString createdAt;
};

struct RecognitionLog
{
    int id;
    QString recognizedAt;
    QString personName;
    double similarity;
    QString imagePath;
};

class FaceRepository
{
public:
    FaceRepository();
    ~FaceRepository();

    bool open(const QString &databasePath);
    bool initializeTables();

    int addPerson(const QString &name,
                  const QString &studentId,
                  const QString &department);

    QList<Person> getAllPersons() const;

    bool addFaceFeature(int personId, const std::vector<float> &feature);
    QList<FaceFeatureRecord> getAllFaceFeatures() const;

    bool addRecognitionLog(const QString &personName,
                           double similarity,
                           const QString &imagePath);

    QList<RecognitionLog> getRecentLogs(int limit = 50) const;

    QString lastError() const;

private:
    QString featureToJson(const std::vector<float> &feature) const;
    std::vector<float> jsonToFeature(const QString &jsonText) const;

private:
    QSqlDatabase database;
    QString connectionName;
    QString lastErrorText;
};

#endif