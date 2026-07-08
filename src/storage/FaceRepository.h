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
    QString name;
    QString studentId;
    QString department;
    QString createdAt;
};

struct FaceFeatureRecord
{
    int id;
    QString studentId;
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

    bool addPerson(const QString &name,
                   const QString &studentId,
                   const QString &department);

    QList<Person> getAllPersons() const;
    QList<Person> searchPersons(const QString &keyword) const;
    bool studentIdExists(const QString &studentId) const;
    bool updatePerson(const QString &originalStudentId,
                      const QString &name,
                      const QString &studentId,
                      const QString &department);
    bool deletePerson(const QString &studentId);

    bool addFaceFeature(const QString &studentId, const std::vector<float> &feature);
    QList<FaceFeatureRecord> getAllFaceFeatures() const;

    bool addRecognitionLog(const QString &personName,
                           double similarity,
                           const QString &imagePath);

    QList<RecognitionLog> getRecentLogs(int limit = 50) const;

    QString lastError() const;

private:
    bool tableExists(const QString &tableName) const;
    bool tableHasColumn(const QString &tableName, const QString &columnName) const;
    bool migrateLegacyPersonTables();
    bool createCurrentTables();
    Person readPerson(const QSqlQuery &query) const;
    QString featureToJson(const std::vector<float> &feature) const;
    std::vector<float> jsonToFeature(const QString &jsonText) const;

private:
    QSqlDatabase database;
    QString connectionName;
    QString lastErrorText;
};

#endif
