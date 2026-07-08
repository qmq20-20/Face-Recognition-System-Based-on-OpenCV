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
    if (tableExists("persons") && tableHasColumn("persons", "id"))
    {
        if (!migrateLegacyPersonTables())
        {
            return false;
        }
    }

    return createCurrentTables();
}

bool FaceRepository::addPerson(const QString &name,
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
        return false;
    }

    return true;
}

QList<Person> FaceRepository::getAllPersons() const
{
    QList<Person> persons;

    QSqlQuery query(database);
    query.exec(
        "SELECT name, student_id, department, created_at "
        "FROM persons "
        "ORDER BY student_id ASC");

    while (query.next())
    {
        persons.append(readPerson(query));
    }

    return persons;
}

QList<Person> FaceRepository::searchPersons(const QString &keyword) const
{
    QString trimmedKeyword = keyword.trimmed();

    if (trimmedKeyword.isEmpty())
    {
        return getAllPersons();
    }

    QList<Person> persons;
    QString likeKeyword = "%" + trimmedKeyword + "%";

    QSqlQuery query(database);
    query.prepare(
        "SELECT name, student_id, department, created_at "
        "FROM persons "
        "WHERE name LIKE :keyword "
        "OR student_id LIKE :keyword "
        "OR department LIKE :keyword "
        "ORDER BY student_id ASC");

    query.bindValue(":keyword", likeKeyword);
    query.exec();

    while (query.next())
    {
        persons.append(readPerson(query));
    }

    return persons;
}

bool FaceRepository::studentIdExists(const QString &studentId) const
{
    QSqlQuery query(database);

    query.prepare("SELECT 1 FROM persons WHERE student_id = :student_id LIMIT 1");
    query.bindValue(":student_id", studentId);

    if (!query.exec())
    {
        return false;
    }

    return query.next();
}

bool FaceRepository::updatePerson(const QString &originalStudentId,
                                  const QString &name,
                                  const QString &studentId,
                                  const QString &department)
{
    QSqlQuery query(database);

    query.prepare(
        "UPDATE persons "
        "SET name = :name, student_id = :student_id, department = :department "
        "WHERE student_id = :original_student_id");

    query.bindValue(":name", name);
    query.bindValue(":student_id", studentId);
    query.bindValue(":department", department);
    query.bindValue(":original_student_id", originalStudentId);

    if (!query.exec())
    {
        lastErrorText = query.lastError().text();
        return false;
    }

    return true;
}

bool FaceRepository::deletePerson(const QString &studentId)
{
    QSqlQuery query(database);

    query.prepare("DELETE FROM persons WHERE student_id = :student_id");
    query.bindValue(":student_id", studentId);

    if (!query.exec())
    {
        lastErrorText = query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

bool FaceRepository::addFaceFeature(const QString &studentId, const std::vector<float> &feature)
{
    QSqlQuery query(database);

    query.prepare(
        "INSERT INTO face_features (student_id, feature, created_at) "
        "VALUES (:student_id, :feature, :created_at)");

    query.bindValue(":student_id", studentId);
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
        "SELECT id, student_id, feature, created_at "
        "FROM face_features "
        "ORDER BY id DESC");

    while (query.next())
    {
        FaceFeatureRecord record;
        record.id = query.value("id").toInt();
        record.studentId = query.value("student_id").toString();
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

bool FaceRepository::tableExists(const QString &tableName) const
{
    return database.tables().contains(tableName);
}

bool FaceRepository::tableHasColumn(const QString &tableName, const QString &columnName) const
{
    QSqlQuery query(database);
    query.exec("PRAGMA table_info(" + tableName + ")");

    while (query.next())
    {
        if (query.value("name").toString() == columnName)
        {
            return true;
        }
    }

    return false;
}

bool FaceRepository::migrateLegacyPersonTables()
{
    QSqlQuery checkQuery(database);

    if (!checkQuery.exec("SELECT COUNT(*) FROM persons WHERE student_id IS NULL OR TRIM(student_id) = ''"))
    {
        lastErrorText = checkQuery.lastError().text();
        return false;
    }

    if (checkQuery.next() && checkQuery.value(0).toInt() > 0)
    {
        lastErrorText = "旧数据库中存在空学号记录，无法迁移到以学号为唯一标识的结构。";
        return false;
    }

    if (!checkQuery.exec(
            "SELECT student_id FROM persons "
            "GROUP BY student_id "
            "HAVING COUNT(*) > 1 "
            "LIMIT 1"))
    {
        lastErrorText = checkQuery.lastError().text();
        return false;
    }

    if (checkQuery.next())
    {
        lastErrorText = "旧数据库中存在重复学号，无法迁移到以学号为唯一标识的结构。";
        return false;
    }

    bool hasLegacyFeatures = tableExists("face_features");

    QSqlQuery pragmaQuery(database);
    pragmaQuery.exec("PRAGMA foreign_keys = OFF");

    if (!database.transaction())
    {
        lastErrorText = database.lastError().text();
        pragmaQuery.exec("PRAGMA foreign_keys = ON");
        return false;
    }

    QSqlQuery query(database);

    if (!query.exec("ALTER TABLE persons RENAME TO persons_legacy"))
    {
        lastErrorText = query.lastError().text();
        database.rollback();
        pragmaQuery.exec("PRAGMA foreign_keys = ON");
        return false;
    }

    if (hasLegacyFeatures && !query.exec("ALTER TABLE face_features RENAME TO face_features_legacy"))
    {
        lastErrorText = query.lastError().text();
        database.rollback();
        pragmaQuery.exec("PRAGMA foreign_keys = ON");
        return false;
    }

    if (!createCurrentTables())
    {
        database.rollback();
        pragmaQuery.exec("PRAGMA foreign_keys = ON");
        return false;
    }

    if (!query.exec(
            "INSERT INTO persons (student_id, name, department, created_at) "
            "SELECT student_id, name, department, created_at "
            "FROM persons_legacy"))
    {
        lastErrorText = query.lastError().text();
        database.rollback();
        pragmaQuery.exec("PRAGMA foreign_keys = ON");
        return false;
    }

    if (hasLegacyFeatures &&
        !query.exec(
            "INSERT INTO face_features (student_id, feature, created_at) "
            "SELECT p.student_id, f.feature, f.created_at "
            "FROM face_features_legacy f "
            "JOIN persons_legacy p ON f.person_id = p.id"))
    {
        lastErrorText = query.lastError().text();
        database.rollback();
        pragmaQuery.exec("PRAGMA foreign_keys = ON");
        return false;
    }

    if (hasLegacyFeatures && !query.exec("DROP TABLE face_features_legacy"))
    {
        lastErrorText = query.lastError().text();
        database.rollback();
        pragmaQuery.exec("PRAGMA foreign_keys = ON");
        return false;
    }

    if (!query.exec("DROP TABLE persons_legacy"))
    {
        lastErrorText = query.lastError().text();
        database.rollback();
        pragmaQuery.exec("PRAGMA foreign_keys = ON");
        return false;
    }

    if (!database.commit())
    {
        lastErrorText = database.lastError().text();
        pragmaQuery.exec("PRAGMA foreign_keys = ON");
        return false;
    }

    pragmaQuery.exec("PRAGMA foreign_keys = ON");
    return true;
}

bool FaceRepository::createCurrentTables()
{
    QSqlQuery query(database);

    const QString createPersonsTable =
        "CREATE TABLE IF NOT EXISTS persons ("
        "student_id TEXT PRIMARY KEY NOT NULL CHECK(TRIM(student_id) <> ''),"
        "name TEXT NOT NULL,"
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
        "student_id TEXT NOT NULL,"
        "feature TEXT NOT NULL,"
        "created_at TEXT NOT NULL,"
        "FOREIGN KEY(student_id) REFERENCES persons(student_id) ON UPDATE CASCADE ON DELETE CASCADE"
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

Person FaceRepository::readPerson(const QSqlQuery &query) const
{
    Person person;
    person.name = query.value("name").toString();
    person.studentId = query.value("student_id").toString();
    person.department = query.value("department").toString();
    person.createdAt = query.value("created_at").toString();

    return person;
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
