#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QVariant>
#include <QList>
#include <QPair>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QEventLoop>

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();

    void setHttpMode(bool enabled) { m_httpMode = enabled; }
    bool isHttpMode() const { return m_httpMode; }
    QString serverUrl() const { return m_serverUrl; }

    void setAuthToken(const QString &token) { m_authToken = token; }
    QString authToken() const { return m_authToken; }
    bool isSuperuser() const { return !m_authToken.isEmpty(); }

    bool connectToDatabase(const QString &host = "localhost",
                          const QString &port = "5432",
                          const QString &database = "military_db",
                          const QString &username = "postgres",
                          const QString &password = "");

    bool isConnected() const;
    void disconnect();

    QSqlQuery executeQuery(const QString &query, bool *ok = nullptr);
    QJsonArray executeCustomQueryHttp(const QString &sql, bool *ok = nullptr);

    QJsonArray fetchTableDataHttp(const QString &tableName, const QString &filters = "");
    bool addRecordHttp(const QString &tableName, const QJsonObject &data);
    bool updateRecordHttp(const QString &tableName, int recordId, const QJsonObject &data);
    bool deleteRecordHttp(const QString &tableName, int recordId);
    bool createBackupHttp();

    QStringList getTableList();
    QString lastError() const;
    QSqlDatabase database() const;

    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    QStringList getColumnList(const QString &tableName);

    struct ColumnDetail {
        QString columnName;
        QString dataType;
        bool isNullable;
        QVariant defaultValue;
        int characterMaxLength;
    };

    struct ForeignKeyInfo {
        QString constraintName;
        QString columnName;
        QString referencedTable;
        QString referencedColumn;
        QString updateRule;
        QString deleteRule;
    };

    struct SequenceInfo {
        QString sequenceName;
        long long currentValue;
        long long increment;
    };
    
    QStringList getPrimaryKeys(const QString &tableName);
    QString getPrimaryKeyColumn(const QString &tableName);
    QList<ColumnDetail> getColumnDetails(const QString &tableName);
    QList<ForeignKeyInfo> getForeignKeyInfo(const QString &tableName);
    QStringList getForeignKeys(const QString &tableName);

    QList<QPair<QString, QString>> getColumnInfo(const QString &tableName);
    QString getColumnDefinition(const QString &tableName, const QString &columnName);
    QStringList getUniqueConstraints(const QString &tableName);
    QStringList getIndexes(const QString &tableName);
    QStringList getSequences(const QString &tableName);
    QList<SequenceInfo> getSequenceInfo(const QString &tableName);

    // Исправлено: используем явный QList<QString> для соответствия вызову
    bool createTable(const QString &tableName, const QList<QPair<QString, QString>> &columns, const QList<QString> &primaryKeys);

    bool dropTable(const QString &tableName, bool cascade = false);
    bool addColumn(const QString &tableName, const QString &columnName, const QString &dataType, bool isNullable = true, const QVariant &defaultValue = QVariant());
    bool dropColumn(const QString &tableName, const QString &columnName);
    bool alterColumnType(const QString &tableName, const QString &columnName, const QString &newDataType);

    QByteArray sendHttpRequest(const QString &method, const QString &url, const QByteArray &data = QByteArray());
    static QString escapeIdentifier(const QString &identifier);
    QSqlQuery prepareQuery(const QString &query);
    bool executePreparedQuery(QSqlQuery &query);

private:
    QSqlDatabase m_db;
    QString m_lastError;
    bool m_httpMode;
    QNetworkAccessManager *m_networkManager;
    QString m_serverUrl;
    QString m_authToken;
};

#endif // DATABASEMANAGER_H
