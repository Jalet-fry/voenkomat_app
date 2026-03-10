#include "DatabaseManager.h"
#include <QSettings>
#include <QDir>
#include <QDebug>
#include <QThread>
#include <QFile>

DatabaseManager::DatabaseManager(QObject *parent) : QObject(parent) {}

DatabaseManager::~DatabaseManager() {
    QStringList connNames = QSqlDatabase::connectionNames();
    for (const QString &name : connNames) {
        QSqlDatabase::removeDatabase(name);
    }
}

DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager inst;
    return inst;
}

bool DatabaseManager::connectToDatabase() {
    QString configPath = QDir::current().absoluteFilePath("../../../../config.ini");

    if (!QFile::exists(configPath)) {
        qCritical() << "Config file not found:" << configPath;
        return false;
    }

    QSettings settings(configPath, QSettings::IniFormat);
    if (settings.status() != QSettings::NoError) {
        qCritical() << "Failed to read config file:" << configPath;
        return false;
    }

    settings.beginGroup("Database");
    m_host = settings.value("host", "localhost").toString();
    m_port = settings.value("port", 5432).toInt();
    m_dbName = settings.value("database", "military_db").toString();
    m_user = settings.value("username", "postgres").toString();
    m_pass = settings.value("password", "").toString();
    settings.endGroup();

    // Проверяем подключение
    QSqlDatabase testDb = db();
    if (testDb.isOpen()) {
        qInfo() << "Successfully connected to database" << m_dbName;
        return true;
    }
    return false;
}

QSqlDatabase DatabaseManager::db() {
    QString connName = QString("conn_%1").arg(quintptr(QThread::currentThreadId()));

    if (QSqlDatabase::contains(connName)) {
        QSqlDatabase d = QSqlDatabase::database(connName);
        if (d.isOpen()) return d;
        if (d.open()) return d;
        QSqlDatabase::removeDatabase(connName);
    }

    QSqlDatabase d = QSqlDatabase::addDatabase("QPSQL", connName);
    d.setHostName(m_host);
    d.setPort(m_port);
    d.setDatabaseName(m_dbName);
    d.setUserName(m_user);
    d.setPassword(m_pass);

    if (!d.open()) {
        qCritical() << "Database connection failed in thread" << QThread::currentThreadId() << ":" << d.lastError().text();
    }
    return d;
}

QJsonArray DatabaseManager::executeSelect(const QString &queryStr, const QVariantList &params) {
    QJsonArray results;
    QSqlQuery query(db());
    query.prepare(queryStr);
    for (const auto &p : params) query.addBindValue(p);

    if (query.exec()) {
        QSqlRecord rec = query.record();
        while (query.next()) {
            QJsonObject obj;
            for (int i = 0; i < rec.count(); ++i) {
                QVariant val = query.value(i);
                obj[rec.fieldName(i)] = val.isNull() ? QJsonValue::Null : QJsonValue::fromVariant(val);
            }
            results.append(obj);
        }
    } else {
        qWarning() << "SQL Error:" << query.lastError().text() << "Query:" << queryStr;
    }
    return results;
}

QJsonObject DatabaseManager::executeModify(const QString &queryStr, const QVariantList &params) {
    QJsonObject res;
    QSqlQuery query(db());
    query.prepare(queryStr);
    for (const auto &p : params) query.addBindValue(p);

    if (query.exec()) {
        res["status"] = "success";
        QJsonArray dataList;

        // В PostgreSQL INSERT/UPDATE/DELETE с RETURNING возвращают данные как SELECT
        if (query.isSelect() || query.isActive()) {
             QSqlRecord rec = query.record();
             if (rec.count() > 0) {
                 while (query.next()) {
                     QJsonObject row;
                     for (int i = 0; i < rec.count(); ++i) {
                         QVariant val = query.value(i);
                         row[rec.fieldName(i)] = val.isNull() ? QJsonValue::Null : QJsonValue::fromVariant(val);
                     }
                     dataList.append(row);
                 }
             }
        }

        res["data"] = dataList;
        res["rows_affected"] = query.numRowsAffected();
    } else {
        res["status"] = "error";
        res["message"] = query.lastError().text();
        qWarning() << "SQL Error:" << query.lastError().text() << "Query:" << queryStr;
    }
    return res;
}

QString DatabaseManager::getPrimaryKeyColumn(const QString &tableName) {
    QSqlQuery query(db());
    query.prepare("SELECT kcu.column_name FROM information_schema.table_constraints tc "
                  "JOIN information_schema.key_column_usage kcu ON tc.constraint_name = kcu.constraint_name "
                  "WHERE tc.constraint_type = 'PRIMARY KEY' AND tc.table_name = ?");
    query.addBindValue(tableName);
    if (query.exec() && query.next()) return query.value(0).toString();
    return "id";
}
