#include "storage/FaceRepository.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

FaceRepository::FaceRepository()
    : connectionName("face_repository_connection")
{
}

FaceRepository::~FaceRepository()
{
    if (database.isValid())
    {
        database.close();
    }

    database = QSqlDatabase();
    QSqlDatabase::removeDatabase(connectionName);
}

bool FaceRepository::open(const QString &databasePath)
{
    QFileInfo fileInfo(databasePath);
    QDir dir = fileInfo.dir();

    if (!dir.exists())
    {
        dir.mkpath(".");
    }

    if (QSqlDatabase::contains(connectionName))
    {
        database = QSqlDatabase::database(connectionName);
    }
    else
    {
        database = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    }

    database.setDatabaseName(databasePath);

    if (!database.open())
    {
        lastErrorText = database.lastError().text();
        return false;
    }

    QSqlQuery pragmaQuery(database);
    pragmaQuery.exec("PRAGMA foreign_keys = ON");

    return initializeTables();
}

bool FaceRepository::initializeTables()
{
    QSqlQuery query(database);

    const QString createPersonsTable =
        "CREATE TABLE IF NOT EXISTS persons ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL,"
        "student_id TEXT,"
        "department TEXT,"
        "created_at TEXT NOT NULL"
        ")";

    if (!query.exec(createPersonsTable))
    {
        lastErrorText = query.lastError().text();
        return false;
    }

    const QString createFeaturesTable =
        "CREATE TABLE IF NOT EXISTS face_features ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "person_id INTEGER NOT NULL,"
        "feature TEXT NOT NULL,"
        "created_at TEXT NOT NULL,"
        "FOREIGN KEY(person_id) REFERENCES persons(id) ON DELETE CASCADE"
        ")";

    if (!query.exec(createFeaturesTable))
    {
        lastErrorText = query.lastError().text();
        return false;
    }

    const QString createLogsTable =
        "CREATE TABLE IF NOT EXISTS recognition_logs ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "recognized_at TEXT NOT NULL,"
        "person_name TEXT NOT NULL,"
        "similarity REAL NOT NULL,"
        "image_path TEXT"
        ")";

    if (!query.exec(createLogsTable))
    {
        lastErrorText = query.lastError().text();
        return false;
    }

    return true;
}

int FaceRepository::addPerson(const QString &name,
                              const QString &studentId,
                              const QString &department)
{
    QSqlQuery query(database);

    query.prepare(
        "INSERT INTO persons (name, student_id, department, created_at) "
        "VALUES (:name, :student_id, :department, :created_at)");

    query.bindValue(":name", name);
    query.bindValue(":student_id", studentId);
    query.bindValue(":department", department);
    query.bindValue(":created_at", QDateTime::currentDateTime().toString(Qt::ISODate));

    if (!query.exec())
    {
        lastErrorText = query.lastError().text();
        return -1;
    }

    return query.lastInsertId().toInt();
}

QList<Person> FaceRepository::getAllPersons() const
{
    QList<Person> persons;

    QSqlQuery query(database);
    query.exec(
        "SELECT id, name, student_id, department, created_at "
        "FROM persons "
        "ORDER BY id DESC");

    while (query.next())
    {
        Person person;
        person.id = query.value("id").toInt();
        person.name = query.value("name").toString();
        person.studentId = query.value("student_id").toString();
        person.department = query.value("department").toString();
        person.createdAt = query.value("created_at").toString();

        persons.append(person);
    }

    return persons;
}

bool FaceRepository::addFaceFeature(int personId, const std::vector<float> &feature)
{
    QSqlQuery query(database);

    query.prepare(
        "INSERT INTO face_features (person_id, feature, created_at) "
        "VALUES (:person_id, :feature, :created_at)");

    query.bindValue(":person_id", personId);
    query.bindValue(":feature", featureToJson(feature));
    query.bindValue(":created_at", QDateTime::currentDateTime().toString(Qt::ISODate));

    if (!query.exec())
    {
        lastErrorText = query.lastError().text();
        return false;
    }

    return true;
}

QList<FaceFeatureRecord> FaceRepository::getAllFaceFeatures() const
{
    QList<FaceFeatureRecord> records;

    QSqlQuery query(database);
    query.exec(
        "SELECT id, person_id, feature, created_at "
        "FROM face_features "
        "ORDER BY id DESC");

    while (query.next())
    {
        FaceFeatureRecord record;
        record.id = query.value("id").toInt();
        record.personId = query.value("person_id").toInt();
        record.feature = jsonToFeature(query.value("feature").toString());
        record.createdAt = query.value("created_at").toString();

        records.append(record);
    }

    return records;
}

bool FaceRepository::addRecognitionLog(const QString &personName,
                                       double similarity,
                                       const QString &imagePath)
{
    QSqlQuery query(database);

    query.prepare(
        "INSERT INTO recognition_logs "
        "(recognized_at, person_name, similarity, image_path) "
        "VALUES (:recognized_at, :person_name, :similarity, :image_path)");

    query.bindValue(":recognized_at", QDateTime::currentDateTime().toString(Qt::ISODate));
    query.bindValue(":person_name", personName);
    query.bindValue(":similarity", similarity);
    query.bindValue(":image_path", imagePath);

    if (!query.exec())
    {
        lastErrorText = query.lastError().text();
        return false;
    }

    return true;
}

QList<RecognitionLog> FaceRepository::getRecentLogs(int limit) const
{
    QList<RecognitionLog> logs;

    QSqlQuery query(database);
    query.prepare(
        "SELECT id, recognized_at, person_name, similarity, image_path "
        "FROM recognition_logs "
        "ORDER BY id DESC "
        "LIMIT :limit");

    query.bindValue(":limit", limit);
    query.exec();

    while (query.next())
    {
        RecognitionLog log;
        log.id = query.value("id").toInt();
        log.recognizedAt = query.value("recognized_at").toString();
        log.personName = query.value("person_name").toString();
        log.similarity = query.value("similarity").toDouble();
        log.imagePath = query.value("image_path").toString();

        logs.append(log);
    }

    return logs;
}

QString FaceRepository::lastError() const
{
    return lastErrorText;
}

QString FaceRepository::featureToJson(const std::vector<float> &feature) const
{
    QJsonArray array;

    for (float value : feature)
    {
        array.append(value);
    }

    QJsonDocument document(array);
    return QString::fromUtf8(document.toJson(QJsonDocument::Compact));
}

std::vector<float> FaceRepository::jsonToFeature(const QString &jsonText) const
{
    std::vector<float> feature;

    QJsonDocument document = QJsonDocument::fromJson(jsonText.toUtf8());

    if (!document.isArray())
    {
        return feature;
    }

    QJsonArray array = document.array();
    feature.reserve(array.size());

    for (const QJsonValue &value : array)
    {
        feature.push_back(static_cast<float>(value.toDouble()));
    }

    return feature;
}
