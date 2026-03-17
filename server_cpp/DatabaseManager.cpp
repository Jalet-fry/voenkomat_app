#include "DatabaseManager.h"
#include <QSettings>
#include <QDir>
#include <QDebug>
#include <QThread>
#include <QFile>
#include <QCoreApplication>

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
    // Ищем конфиг относительно папки с EXE (поднимаемся из build/debug в корень проекта)
    QString appDir = QCoreApplication::applicationDirPath();

    // Пытаемся найти config.ini в разных местах (в текущей папке или выше)
    QStringList paths;
    paths << appDir + "/config.ini";
    paths << appDir + "/../config.ini";
    paths << appDir + "/../../config.ini";
    paths << appDir + "/../../../config.ini";
    paths << appDir + "/../../../../config.ini";

    QString configPath;
    for (const QString &p : paths) {
        if (QFile::exists(p)) {
            configPath = QDir::cleanPath(p);
            break;
        }
    }

    if (configPath.isEmpty()) {
        qCritical() << "CRITICAL: config.ini not found in search paths!";
        return false;
    }

    qInfo() << "Using config file:" << configPath;
    QSettings settings(configPath, QSettings::IniFormat);

    settings.beginGroup("Database");
    m_host = settings.value("host", "localhost").toString();
    m_port = settings.value("port", 5432).toInt();
    m_dbName = settings.value("database", "military_db").toString();
    m_user = settings.value("username", "postgres").toString();
    m_pass = settings.value("password", "").toString();
    settings.endGroup();

    QSqlDatabase testDb = db();
    return testDb.isOpen();
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
        qCritical() << "Database connection failed:" << d.lastError().text();
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
        qWarning() << "SQL Error:" << query.lastError().text();
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
        if (dataList.isEmpty()) {
            QJsonObject affected; affected["rows_affected"] = query.numRowsAffected();
            dataList.append(affected);
        }
        res["data"] = dataList;
    } else {
        res["status"] = "error";
        res["message"] = query.lastError().text();
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
