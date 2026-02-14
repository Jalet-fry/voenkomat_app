#include "DatabaseManager.h"
#include <QDebug>
#include <QSqlDriver>
#include <QSqlError>
#include <QCoreApplication>
#include <QDir>
#include <QLibrary>

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent), m_httpMode(false), m_networkManager(new QNetworkAccessManager(this))
{
    m_serverUrl = "http://127.0.0.1:8000";
}

DatabaseManager::~DatabaseManager()
{
    disconnect();
}

bool DatabaseManager::connectToDatabase(const QString &host,
                                       const QString &port,
                                       const QString &database,
                                       const QString &username,
                                       const QString &password)
{
    if (m_httpMode) {
        QString targetHost = (host == "localhost" || host.isEmpty()) ? "127.0.0.1" : host;
        m_serverUrl = QString("http://%1:%2").arg(targetHost).arg(port == "5432" ? "8000" : port);

        QByteArray response = sendHttpGetRequest(m_serverUrl + "/");
        if (response.isEmpty()) {
            m_lastError = "Сервер Python не отвечает по адресу " + m_serverUrl;
            return false;
        }
        qDebug() << "Успешное подключение к серверу (HTTP):" << m_serverUrl;
        return true;
    }

    if (QSqlDatabase::contains("military_connection")) {
        {
            QSqlDatabase db = QSqlDatabase::database("military_connection");
            db.close();
        }
        QSqlDatabase::removeDatabase("military_connection");
    }

    m_db = QSqlDatabase::addDatabase("QPSQL", "military_connection");
    m_db.setHostName(host);
    m_db.setPort(port.toInt());
    m_db.setDatabaseName(database);
    m_db.setUserName(username);
    m_db.setPassword(password);

    if (!m_db.open()) {
        m_lastError = m_db.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::isConnected() const {
    return m_httpMode ? !m_serverUrl.isEmpty() : m_db.isOpen();
}

void DatabaseManager::disconnect() {
    if (m_db.isOpen()) m_db.close();
}

QSqlQuery DatabaseManager::executeQuery(const QString &query, bool *ok)
{
    if (m_httpMode) {
        qDebug() << "Warning: executeQuery called in HTTP mode. Use executeCustomQueryHttp for data results.";
        if (ok) *ok = false;
        return QSqlQuery(m_db);
    }

    QSqlQuery sqlQuery(m_db);
    if (!sqlQuery.exec(query)) {
        m_lastError = sqlQuery.lastError().text();
        if (ok) *ok = false;
    } else {
        if (ok) *ok = true;
    }
    return sqlQuery;
}

QJsonArray DatabaseManager::executeCustomQueryHttp(const QString &sql, bool *ok)
{
    QNetworkRequest request(QUrl(m_serverUrl + "/api/execute-query"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["sql"] = sql;
    QJsonDocument doc(json);

    QNetworkReply *reply = m_networkManager->post(request, doc.toJson());

    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    QJsonArray data;
    if (reply->error() == QNetworkReply::NoError) {
        if (ok) *ok = true;
        QJsonDocument resDoc = QJsonDocument::fromJson(reply->readAll());
        data = resDoc.object()["data"].toArray();
    } else {
        if (ok) *ok = false;
        m_lastError = reply->errorString();
        qDebug() << "Custom Query Error:" << m_lastError;
    }
    reply->deleteLater();
    return data;
}

QJsonArray DatabaseManager::fetchTableDataHttp(const QString &tableName)
{
    QByteArray response = sendHttpGetRequest(m_serverUrl + "/api/" + tableName + "?limit=1000");
    if (response.isEmpty()) return QJsonArray();
    QJsonDocument doc = QJsonDocument::fromJson(response);
    return doc.object()["data"].toArray();
}

QStringList DatabaseManager::getTableList()
{
    if (m_httpMode) {
        QStringList tables;
        QByteArray response = sendHttpGetRequest(m_serverUrl + "/api/all-tables");
        if (!response.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(response);
            QJsonArray arr = doc.object()["tables"].toArray();
            for (int i = 0; i < arr.size(); ++i) tables << arr[i].toString();
        }
        return tables;
    }
    QStringList tables;
    QSqlQuery query(m_db);
    query.exec("SELECT table_name FROM information_schema.tables WHERE table_schema = 'public' AND table_type = 'BASE TABLE'");
    while (query.next()) tables << query.value(0).toString();
    return tables;
}

QByteArray DatabaseManager::sendHttpGetRequest(const QString &url)
{
    QNetworkRequest request((QUrl(url)));
    QNetworkReply *reply = m_networkManager->get(request);
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();
    QByteArray data;
    if (reply->error() == QNetworkReply::NoError) data = reply->readAll();
    else m_lastError = reply->errorString();
    reply->deleteLater();
    return data;
}

// Реализация недостающих методов для компиляции и работы BackupManager/RecordDialog
QSqlQuery DatabaseManager::prepareQuery(const QString &query) { QSqlQuery q(m_db); q.prepare(query); return q; }
bool DatabaseManager::executePreparedQuery(QSqlQuery &query) { return query.exec(); }
QString DatabaseManager::lastError() const { return m_lastError; }
QSqlDatabase DatabaseManager::database() const { return m_db; }
bool DatabaseManager::beginTransaction() { return m_httpMode ? true : m_db.transaction(); }
bool DatabaseManager::commitTransaction() { return m_httpMode ? true : m_db.commit(); }
bool DatabaseManager::rollbackTransaction() { return m_httpMode ? true : m_db.rollback(); }

QStringList DatabaseManager::getColumnList(const QString &t) {
    if(m_httpMode) return QStringList();
    QSqlQuery q(m_db);
    q.prepare("SELECT column_name FROM information_schema.columns WHERE table_name = :t AND table_schema = 'public' ORDER BY ordinal_position");
    q.bindValue(":t", t);
    QStringList c;
    if(q.exec()) while(q.next()) c << q.value(0).toString();
    return c;
}

QStringList DatabaseManager::getPrimaryKeys(const QString &t) {
    if(m_httpMode) return QStringList();
    QSqlQuery q(m_db);
    q.prepare("SELECT kcu.column_name FROM information_schema.table_constraints tc JOIN information_schema.key_column_usage kcu ON tc.constraint_name = kcu.constraint_name WHERE tc.constraint_type = 'PRIMARY KEY' AND tc.table_name = :t");
    q.bindValue(":t", t);
    QStringList pks;
    if(q.exec()) while(q.next()) pks << q.value(0).toString();
    return pks;
}

QString DatabaseManager::getPrimaryKeyColumn(const QString &t) {
    QStringList pks = getPrimaryKeys(t);
    return pks.isEmpty() ? "" : pks.first();
}

QList<DatabaseManager::ColumnDetail> DatabaseManager::getColumnDetails(const QString &t) {
    QList<ColumnDetail> details;
    if(m_httpMode) return details;
    QSqlQuery q(m_db);
    q.prepare("SELECT column_name, data_type, is_nullable, column_default, character_maximum_length FROM information_schema.columns WHERE table_name = :t AND table_schema = 'public' ORDER BY ordinal_position");
    q.bindValue(":t", t);
    if(q.exec()) {
        while(q.next()) {
            ColumnDetail d;
            d.columnName = q.value(0).toString();
            d.dataType = q.value(1).toString();
            d.isNullable = (q.value(2).toString() == "YES");
            d.defaultValue = q.value(3);
            d.characterMaxLength = q.value(4).toInt();
            details << d;
        }
    }
    return details;
}

QList<DatabaseManager::ForeignKeyInfo> DatabaseManager::getForeignKeyInfo(const QString &) { return QList<ForeignKeyInfo>(); }
QStringList DatabaseManager::getForeignKeys(const QString &) { return QStringList(); }
QList<QPair<QString, QString>> DatabaseManager::getColumnInfo(const QString &) { return QList<QPair<QString, QString>>(); }
QString DatabaseManager::getColumnDefinition(const QString &, const QString &) { return ""; }
QStringList DatabaseManager::getUniqueConstraints(const QString &) { return QStringList(); }
QStringList DatabaseManager::getIndexes(const QString &) { return QStringList(); }
QStringList DatabaseManager::getSequences(const QString &) { return QStringList(); }
QList<DatabaseManager::SequenceInfo> DatabaseManager::getSequenceInfo(const QString &) { return QList<SequenceInfo>(); }

bool DatabaseManager::createTable(const QString &, const QList<QPair<QString, QString>> &, const QStringList &) { return false; }
bool DatabaseManager::dropTable(const QString &, bool) { return false; }
bool DatabaseManager::addColumn(const QString &, const QString &, const QString &, bool, const QVariant &) { return false; }
bool DatabaseManager::dropColumn(const QString &, const QString &) { return false; }
bool DatabaseManager::alterColumnType(const QString &, const QString &, const QString &) { return false; }

QString DatabaseManager::escapeIdentifier(const QString &i) {
    if (i.contains("\"")) return i;
    return QString("\"%1\"").arg(i);
}
