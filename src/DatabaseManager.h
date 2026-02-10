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

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();

    bool connectToDatabase(const QString &host = "localhost",
                          const QString &port = "5432",
                          const QString &database = "military_db",
                          const QString &username = "postgres",
                          const QString &password = "");

    bool isConnected() const;
    void disconnect();

    QSqlQuery executeQuery(const QString &query, bool *ok = nullptr);
    QSqlQuery prepareQuery(const QString &query);
    bool executePreparedQuery(QSqlQuery &query);

    QString lastError() const;
    QSqlDatabase database() const;

    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    QStringList getTableList();
    QStringList getColumnList(const QString &tableName);
    QList<QPair<QString, QString>> getColumnInfo(const QString &tableName);
    
    struct ColumnDetail {
        QString columnName;
        QString dataType;
        bool isNullable;
        QVariant defaultValue;
        int characterMaxLength;
    };
    
    QStringList getPrimaryKeys(const QString &tableName);
    QString getPrimaryKeyColumn(const QString &tableName);
    QList<ColumnDetail> getColumnDetails(const QString &tableName);
    QStringList getForeignKeys(const QString &tableName);

    struct ForeignKeyInfo {
        QString constraintName;
        QString columnName;
        QString referencedTable;
        QString referencedColumn;
        QString deleteRule;
    };
    QList<ForeignKeyInfo> getForeignKeyInfo(const QString &tableName);
    QString getForeignKeyConstraintName(const QString &tableName, const QString &columnName);
    QString getForeignKeyDeleteRule(const QString &tableName, const QString &constraintName);
    bool addForeignKey(const QString &tableName, const QString &columnName, 
                      const QString &referencedTable, const QString &referencedColumn,
                      const QString &deleteRule = "RESTRICT");
    bool removeForeignKey(const QString &tableName, const QString &constraintName);
    bool recordExists(const QString &tableName, const QString &columnName, const QVariant &value);
    bool checkUniqueValue(const QString &tableName, const QString &columnName, 
                         const QVariant &value, int excludeRecordId = -1);
    bool checkUniqueConstraint(const QString &tableName, const QString &constraintName,
                               const QStringList &columnNames, const QList<QVariant> &values,
                               int excludeRecordId = -1);
    QStringList getUniqueConstraints(const QString &tableName);
    QStringList getIndexes(const QString &tableName);
    QStringList getSequences(const QString &tableName);

    struct SequenceInfo {
        QString sequenceName;
        QString columnName;
        qint64 currentValue;
    };
    QList<SequenceInfo> getSequenceInfo(const QString &tableName);
    QString getColumnDefinition(const QString &tableName, const QString &columnName);
    
    bool createTable(const QString &tableName, const QList<QPair<QString, QString>> &columns,
                     const QStringList &primaryKeys = QStringList());
    bool dropTable(const QString &tableName, bool cascade = false);
    bool addColumn(const QString &tableName, const QString &columnName, const QString &dataType, 
                  bool isNullable = true, const QVariant &defaultValue = QVariant());
    bool dropColumn(const QString &tableName, const QString &columnName);
    bool alterColumnType(const QString &tableName, const QString &columnName, const QString &newDataType);
    
    static QString escapeIdentifier(const QString &identifier);

private:
    QSqlDatabase m_db;
    QString m_lastError;
};

#endif // DATABASEMANAGER_H
