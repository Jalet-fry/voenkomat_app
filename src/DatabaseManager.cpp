#include "DatabaseManager.h"
#include <QDebug>
#include <QSqlDriver>
#include <QSqlError>
#include <QCoreApplication>
#include <QDir>
#include <QLibrary>

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
{
    // Не создаем базу в конструкторе, будем создавать при подключении
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
    // Гарантируем закрытие старого соединения перед созданием нового
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
        qDebug() << "Ошибка подключения к" << database << ":" << m_lastError;
        return false;
    }

    qDebug() << "Успешное подключение к базе данных:" << database;
    return true;
}

bool DatabaseManager::isConnected() const { return m_db.isOpen(); }

void DatabaseManager::disconnect() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}

QSqlQuery DatabaseManager::executeQuery(const QString &query, bool *ok)
{
    QSqlQuery sqlQuery(m_db);
    if (!sqlQuery.exec(query)) {
        m_lastError = sqlQuery.lastError().text();
        qDebug() << "SQL Error:" << m_lastError;
        if (ok) *ok = false;
    } else {
        if (ok) *ok = true;
    }
    return sqlQuery;
}

QSqlQuery DatabaseManager::prepareQuery(const QString &query)
{
    QSqlQuery q(m_db);
    q.prepare(query);
    return q;
}

bool DatabaseManager::executePreparedQuery(QSqlQuery &query) { return query.exec(); }
QString DatabaseManager::lastError() const { return m_lastError; }
QSqlDatabase DatabaseManager::database() const { return m_db; }
bool DatabaseManager::beginTransaction() { return m_db.transaction(); }
bool DatabaseManager::commitTransaction() { return m_db.commit(); }
bool DatabaseManager::rollbackTransaction() { return m_db.rollback(); }

QStringList DatabaseManager::getTableList()
{
    QStringList tables;
    QSqlQuery query(m_db);
    query.exec("SELECT table_name FROM information_schema.tables WHERE table_schema = 'public' AND table_type = 'BASE TABLE'");
    while (query.next()) tables << query.value(0).toString();
    return tables;
}

QStringList DatabaseManager::getColumnList(const QString &tableName)
{
    QStringList columns;
    QSqlQuery query(m_db);
    query.prepare("SELECT column_name FROM information_schema.columns WHERE table_name = :t AND table_schema = 'public' ORDER BY ordinal_position");
    query.bindValue(":t", tableName);
    if (query.exec()) while (query.next()) columns << query.value(0).toString();
    return columns;
}

QList<QPair<QString, QString>> DatabaseManager::getColumnInfo(const QString &tableName)
{
    QList<QPair<QString, QString>> info;
    QSqlQuery query(m_db);
    query.prepare("SELECT column_name, data_type FROM information_schema.columns WHERE table_name = :t AND table_schema = 'public' ORDER BY ordinal_position");
    query.bindValue(":t", tableName);
    if (query.exec()) while (query.next()) info << qMakePair(query.value(0).toString(), query.value(1).toString());
    return info;
}

QStringList DatabaseManager::getPrimaryKeys(const QString &tableName)
{
    QStringList pks;
    QSqlQuery query(m_db);
    query.prepare("SELECT kcu.column_name FROM information_schema.table_constraints tc JOIN information_schema.key_column_usage kcu ON tc.constraint_name = kcu.constraint_name WHERE tc.constraint_type = 'PRIMARY KEY' AND tc.table_name = :t AND tc.table_schema = 'public'");
    query.bindValue(":t", tableName);
    if (query.exec()) while (query.next()) pks << query.value(0).toString();
    return pks;
}

QString DatabaseManager::getPrimaryKeyColumn(const QString &tableName)
{
    QStringList pks = getPrimaryKeys(tableName);
    return pks.isEmpty() ? "" : pks.first();
}

QList<DatabaseManager::ColumnDetail> DatabaseManager::getColumnDetails(const QString &tableName)
{
    QList<ColumnDetail> details;
    QSqlQuery query(m_db);
    query.prepare("SELECT column_name, data_type, is_nullable, column_default, character_maximum_length FROM information_schema.columns WHERE table_name = :t AND table_schema = 'public' ORDER BY ordinal_position");
    query.bindValue(":t", tableName);
    if (query.exec()) {
        while (query.next()) {
            ColumnDetail d;
            d.columnName = query.value(0).toString();
            d.dataType = query.value(1).toString();
            d.isNullable = (query.value(2).toString() == "YES");
            d.defaultValue = query.value(3);
            d.characterMaxLength = query.value(4).toInt();
            details << d;
        }
    }
    return details;
}

QStringList DatabaseManager::getForeignKeys(const QString &tableName) { return QStringList(); }
QList<DatabaseManager::ForeignKeyInfo> DatabaseManager::getForeignKeyInfo(const QString &tableName) { return QList<ForeignKeyInfo>(); }
QString DatabaseManager::getForeignKeyConstraintName(const QString &tableName, const QString &columnName) { return ""; }
QString DatabaseManager::getForeignKeyDeleteRule(const QString &tableName, const QString &constraintName) { return ""; }
bool DatabaseManager::addForeignKey(const QString &tableName, const QString &columnName, const QString &referencedTable, const QString &referencedColumn, const QString &deleteRule) { return false; }
bool DatabaseManager::removeForeignKey(const QString &tableName, const QString &constraintName) { return false; }

bool DatabaseManager::recordExists(const QString &tableName, const QString &columnName, const QVariant &value)
{
    QSqlQuery query(m_db);
    query.prepare(QString("SELECT COUNT(*) FROM %1 WHERE %2 = :v").arg(escapeIdentifier(tableName)).arg(escapeIdentifier(columnName)));
    query.bindValue(":v", value);
    return query.exec() && query.next() && query.value(0).toInt() > 0;
}

bool DatabaseManager::checkUniqueValue(const QString &tableName, const QString &columnName, const QVariant &value, int excludeRecordId) { return true; }
bool DatabaseManager::checkUniqueConstraint(const QString &tableName, const QString &constraintName, const QStringList &columnNames, const QList<QVariant> &values, int excludeRecordId) { return true; }
QStringList DatabaseManager::getUniqueConstraints(const QString &tableName) { return QStringList(); }
QStringList DatabaseManager::getIndexes(const QString &tableName) { return QStringList(); }
QStringList DatabaseManager::getSequences(const QString &tableName) { return QStringList(); }
QList<DatabaseManager::SequenceInfo> DatabaseManager::getSequenceInfo(const QString &tableName) { return QList<SequenceInfo>(); }
QString DatabaseManager::getColumnDefinition(const QString &tableName, const QString &columnName) { return ""; }

bool DatabaseManager::createTable(const QString &tableName, const QList<QPair<QString, QString>> &columns, const QStringList &primaryKeys) { return false; }
bool DatabaseManager::dropTable(const QString &tableName, bool cascade) { return false; }
bool DatabaseManager::addColumn(const QString &tableName, const QString &columnName, const QString &dataType, bool isNullable, const QVariant &defaultValue) { return false; }
bool DatabaseManager::dropColumn(const QString &tableName, const QString &columnName) { return false; }
bool DatabaseManager::alterColumnType(const QString &tableName, const QString &columnName, const QString &newDataType) { return false; }

QString DatabaseManager::escapeIdentifier(const QString &identifier)
{
    if (identifier.contains("\"")) return identifier; // Already escaped
    return QString("\"%1\"").arg(identifier);
}
