#include "DatabaseManager.h"
#include <QDebug>
#include <QSqlDriver>
#include <QSqlError>
#include <QCoreApplication>
#include <QDir>
#include <QLibrary>
#include <QSqlRecord>
#include <QUrl>

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent), m_httpMode(false), m_networkManager(new QNetworkAccessManager(this))
{
    m_serverUrl = "http://127.0.0.1:8000";
}

DatabaseManager::~DatabaseManager() { disconnect(); }

bool DatabaseManager::connectToDatabase(const QString &host, const QString &port, const QString &database, const QString &username, const QString &password)
{
    if (m_httpMode) {
        QString targetHost = (host == "localhost" || host.isEmpty()) ? "127.0.0.1" : host;
        m_serverUrl = QString("http://%1:%2").arg(targetHost).arg(port == "5432" ? "8000" : port);
        QByteArray response = sendHttpRequest("GET", m_serverUrl + "/");
        if (response.isEmpty()) {
            m_lastError = "Сервер API не отвечает по адресу " + m_serverUrl;
            return false;
        }
        return true;
    }

    if (QSqlDatabase::contains("military_connection")) {
        { QSqlDatabase db = QSqlDatabase::database("military_connection"); db.close(); }
        QSqlDatabase::removeDatabase("military_connection");
    }

    m_db = QSqlDatabase::addDatabase("QPSQL", "military_connection");
    m_db.setHostName(host);
    m_db.setPort(port.toInt());
    m_db.setDatabaseName(database);
    m_db.setUserName(username);
    m_db.setPassword(password);

    if (!m_db.open()) { m_lastError = m_db.lastError().text(); return false; }
    return true;
}

bool DatabaseManager::isConnected() const { return m_httpMode ? !m_serverUrl.isEmpty() : m_db.isOpen(); }
void DatabaseManager::disconnect() { if (m_db.isOpen()) m_db.close(); }

QSqlQuery DatabaseManager::executeQuery(const QString &query, bool *ok)
{
    if (m_httpMode) {
        if (ok) *ok = false;
        m_lastError = "Прямые SQL-запросы запрещены в режиме HTTP. Используйте executeCustomQueryHttp.";
        return QSqlQuery(m_db);
    }
    QSqlQuery sqlQuery(m_db);
    if (!sqlQuery.exec(query)) { m_lastError = sqlQuery.lastError().text(); if (ok) *ok = false; }
    else { if (ok) *ok = true; }
    return sqlQuery;
}

QJsonArray DatabaseManager::executeCustomQueryHttp(const QString &sql, bool *ok)
{
    QJsonObject json; json["sql"] = sql;
    QByteArray response = sendHttpRequest("POST", m_serverUrl + "/api/execute-query", QJsonDocument(json).toJson());
    if (response.isEmpty()) { if (ok) *ok = false; return QJsonArray(); }
    if (ok) *ok = true;
    return QJsonDocument::fromJson(response).object()["data"].toArray();
}

QJsonArray DatabaseManager::fetchTableDataHttp(const QString &tableName, const QString &filters)
{
    QString url = m_serverUrl + "/api/" + tableName;
    if (!filters.isEmpty()) url += "?filters=" + QUrl::toPercentEncoding(filters);
    QByteArray response = sendHttpRequest("GET", url);
    return QJsonDocument::fromJson(response).object()["data"].toArray();
}

QJsonObject DatabaseManager::addRecordHttp(const QString &tableName, const QJsonObject &data)
{
    QByteArray response = sendHttpRequest("POST", m_serverUrl + "/api/" + tableName, QJsonDocument(data).toJson());
    if (response.isEmpty()) return QJsonObject();
    QJsonObject obj = QJsonDocument::fromJson(response).object();
    return obj["data"].toObject();
}

bool DatabaseManager::updateRecordHttp(const QString &tableName, int recordId, const QJsonObject &data)
{
    QByteArray response = sendHttpRequest("PUT", m_serverUrl + "/api/" + tableName + "/" + QString::number(recordId), QJsonDocument(data).toJson());
    return !response.isEmpty();
}

bool DatabaseManager::deleteRecordHttp(const QString &tableName, int recordId)
{
    QByteArray response = sendHttpRequest("DELETE", m_serverUrl + "/api/" + tableName + "/" + QString::number(recordId));
    return !response.isEmpty();
}

bool DatabaseManager::createBackupHttp()
{
    QByteArray response = sendHttpRequest("POST", m_serverUrl + "/api/backup");
    return !response.isEmpty();
}

bool DatabaseManager::createTable(const QString &tableName, const QList<QPair<QString, QString>> &columns, const QList<QString> &primaryKeys)
{
    if (m_httpMode) {
        QJsonObject json;
        json["table_name"] = tableName;
        QJsonArray colsArr;
        for (const auto &col : columns) {
            QJsonObject c; c["name"] = col.first; c["type"] = col.second;
            colsArr.append(c);
        }
        json["columns"] = colsArr;
        json["primary_keys"] = QJsonArray::fromStringList(primaryKeys);
        QByteArray resp = sendHttpRequest("POST", m_serverUrl + "/api/create-table", QJsonDocument(json).toJson());
        return !resp.isEmpty();
    }

    QString sql = QString("CREATE TABLE public.%1 (").arg(tableName);
    QStringList colDefs;
    for (const auto &col : columns) {
        colDefs << QString("%1 %2").arg(col.first).arg(col.second);
    }
    if (!primaryKeys.isEmpty()) {
        colDefs << QString("PRIMARY KEY (%1)").arg(primaryKeys.join(", "));
    }
    sql += colDefs.join(", ") + ")";

    bool ok;
    executeQuery(sql, &ok);
    return ok;
}

QByteArray DatabaseManager::sendHttpRequest(const QString &method, const QString &url, const QByteArray &data)
{
    QNetworkRequest request((QUrl(url)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!m_authToken.isEmpty()) {
        request.setRawHeader("X-Auth-Token", m_authToken.toUtf8());
    }

    QNetworkReply *reply = nullptr;
    if (method == "GET") reply = m_networkManager->get(request);
    else if (method == "POST") reply = m_networkManager->post(request, data);
    else if (method == "PUT") reply = m_networkManager->put(request, data);
    else if (method == "DELETE") reply = m_networkManager->deleteResource(request);

    if (!reply) return QByteArray();

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QByteArray responseData;
    if (reply->error() == QNetworkReply::NoError) {
        responseData = reply->readAll();
    } else {
        m_lastError = reply->errorString();
        QByteArray errorBody = reply->readAll();
        QJsonDocument errDoc = QJsonDocument::fromJson(errorBody);
        if (!errDoc.isNull() && errDoc.isObject()) {
            m_lastError = errDoc.object()["detail"].toString();
        }
    }
    reply->deleteLater();
    return responseData;
}

QStringList DatabaseManager::getTableList()
{
    if (m_httpMode) {
        QByteArray response = sendHttpRequest("GET", m_serverUrl + "/api/all-tables");
        QJsonArray arr = QJsonDocument::fromJson(response).object()["tables"].toArray();
        QStringList res; for (int i = 0; i < arr.size(); ++i) res << arr[i].toString();
        return res;
    }
    QStringList tables;
    QSqlQuery query(m_db);
    query.exec("SELECT table_name FROM information_schema.tables WHERE table_schema = 'public' AND table_type = 'BASE TABLE' ORDER BY table_name");
    while (query.next()) tables << query.value(0).toString();
    return tables;
}

QString DatabaseManager::getPrimaryKeyColumn(const QString &t) {
    if (m_httpMode) {
        QByteArray resp = sendHttpRequest("GET", m_serverUrl + "/api/metadata/" + t);
        return QJsonDocument::fromJson(resp).object()["pk"].toString();
    }
    QSqlQuery q(m_db);
    q.prepare("SELECT kcu.column_name FROM information_schema.table_constraints tc JOIN information_schema.key_column_usage kcu ON tc.constraint_name = kcu.constraint_name WHERE tc.constraint_type = 'PRIMARY KEY' AND tc.table_name = :t");
    q.bindValue(":t", t);
    return q.exec() && q.next() ? q.value(0).toString() : "id";
}

QStringList DatabaseManager::getColumnList(const QString &t) {
    if(m_httpMode) {
        QByteArray resp = sendHttpRequest("GET", m_serverUrl + "/api/columns/" + t);
        QJsonArray arr = QJsonDocument::fromJson(resp).object()["columns"].toArray();
        QStringList res; for(int i=0; i<arr.size(); ++i) res << arr[i].toString();
        return res;
    }
    QSqlQuery q(m_db);
    q.prepare("SELECT column_name FROM information_schema.columns WHERE table_name = :t AND table_schema = 'public' ORDER BY ordinal_position");
    q.bindValue(":t", t);
    QStringList c; if(q.exec()) while(q.next()) c << q.value(0).toString();
    return c;
}

QList<DatabaseManager::ColumnDetail> DatabaseManager::getColumnDetails(const QString &t) {
    QList<ColumnDetail> details;
    if(m_httpMode) {
        QByteArray resp = sendHttpRequest("GET", m_serverUrl + "/api/column-details/" + t);
        QJsonArray arr = QJsonDocument::fromJson(resp).object()["details"].toArray();
        for(int i=0; i<arr.size(); ++i) {
            QJsonObject obj = arr[i].toObject();
            ColumnDetail d; d.columnName = obj["name"].toString(); d.dataType = obj["type"].toString();
            d.isNullable = obj["nullable"].toBool(); d.defaultValue = obj["default"].toVariant();
            d.characterMaxLength = 0;
            details << d;
        }
        return details;
    }
    QSqlQuery q(m_db);
    q.prepare("SELECT column_name, data_type, is_nullable, column_default, character_maximum_length FROM information_schema.columns WHERE table_name = :t AND table_schema = 'public' ORDER BY ordinal_position");
    q.bindValue(":t", t);
    if(q.exec()) while(q.next()) {
        ColumnDetail d; d.columnName = q.value(0).toString(); d.dataType = q.value(1).toString();
        d.isNullable = (q.value(2).toString() == "YES"); d.defaultValue = q.value(3);
        d.characterMaxLength = q.value(4).toInt();
        details << d;
    }
    return details;
}

QString DatabaseManager::lastError() const { return m_lastError; }
QSqlDatabase DatabaseManager::database() const { return m_db; }
bool DatabaseManager::beginTransaction() { return m_httpMode ? true : m_db.transaction(); }
bool DatabaseManager::commitTransaction() { return m_httpMode ? true : m_db.commit(); }
bool DatabaseManager::rollbackTransaction() { return m_httpMode ? true : m_db.rollback(); }
QString DatabaseManager::escapeIdentifier(const QString &i) { return QString("\"%1\"").arg(i); }
QStringList DatabaseManager::getPrimaryKeys(const QString &t) { return QStringList() << getPrimaryKeyColumn(t); }

QList<DatabaseManager::ForeignKeyInfo> DatabaseManager::getForeignKeyInfo(const QString &t) {
    QList<ForeignKeyInfo> res;
    if(m_httpMode) {
        QByteArray resp = sendHttpRequest("GET", m_serverUrl + "/api/foreign-keys/" + t);
        QJsonArray arr = QJsonDocument::fromJson(resp).object()["foreign_keys"].toArray();
        for(int i=0; i<arr.size(); ++i) {
            QJsonObject obj = arr[i].toObject();
            ForeignKeyInfo fi;
            fi.columnName = obj["column"].toString();
            fi.referencedTable = obj["referenced_table"].toString();
            fi.referencedColumn = obj["referenced_column"].toString();
            res << fi;
        }
        return res;
    }
    QSqlQuery q(m_db);
    q.prepare("SELECT tc.constraint_name, kcu.column_name, ccu.table_name AS referenced_table, ccu.column_name AS referenced_column FROM information_schema.table_constraints AS tc JOIN information_schema.key_column_usage AS kcu ON tc.constraint_name = kcu.constraint_name JOIN information_schema.constraint_column_usage AS ccu ON ccu.constraint_name = tc.constraint_name WHERE tc.constraint_type = 'FOREIGN KEY' AND tc.table_name = :t");
    q.bindValue(":t", t);
    if(q.exec()) while(q.next()) {
        ForeignKeyInfo fi; fi.columnName = q.value(1).toString();
        fi.referencedTable = q.value(2).toString(); fi.referencedColumn = q.value(3).toString();
        res << fi;
    }
    return res;
}

QStringList DatabaseManager::getForeignKeys(const QString &t) {
    QStringList res;
    foreach(const auto &fi, getForeignKeyInfo(t)) res << fi.columnName;
    return res;
}

QList<QPair<QString, QString>> DatabaseManager::getColumnInfo(const QString &t) {
    QList<QPair<QString, QString>> res;
    foreach(const auto &d, getColumnDetails(t)) res << qMakePair(d.columnName, d.dataType);
    return res;
}

QString DatabaseManager::getColumnDefinition(const QString &t, const QString &c) {
    foreach(const auto &d, getColumnDetails(t)) if(d.columnName == c) return d.dataType;
    return "";
}

bool DatabaseManager::dropTable(const QString &t, bool cascade) {
    QString sql = QString("DROP TABLE IF EXISTS public.%1 %2").arg(t).arg(cascade ? "CASCADE" : "");
    bool ok; executeQuery(sql, &ok); return ok;
}

bool DatabaseManager::addColumn(const QString &t, const QString &c, const QString &type, bool null, const QVariant &val) {
    QString sql = QString("ALTER TABLE public.%1 ADD COLUMN %2 %3").arg(t).arg(c).arg(type);
    if (!null) sql += " NOT NULL";
    if (!val.isNull()) sql += QString(" DEFAULT '%1'").arg(val.toString());
    bool ok; executeQuery(sql, &ok); return ok;
}

bool DatabaseManager::dropColumn(const QString &t, const QString &c) {
    QString sql = QString("ALTER TABLE public.%1 DROP COLUMN IF EXISTS %2").arg(t).arg(c);
    bool ok; executeQuery(sql, &ok); return ok;
}

bool DatabaseManager::alterColumnType(const QString &t, const QString &c, const QString &type) {
    QString sql = QString("ALTER TABLE public.%1 ALTER COLUMN %2 TYPE %3").arg(t).arg(c).arg(type);
    bool ok; executeQuery(sql, &ok); return ok;
}
