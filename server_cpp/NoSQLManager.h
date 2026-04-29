#ifndef NOSQLMANAGER_H
#define NOSQLMANAGER_H

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVariant>

#include <QSqlDatabase>

class NoSQLManager : public QObject
{
    Q_OBJECT
public:
    static NoSQLManager& instance();

    // Основные операции
    QJsonArray getData(const QString &tableName, const QString &filterStr = "", int limit = 0);
    bool insertData(const QString &tableName, QJsonObject &data);
    bool updateData(const QString &tableName, const QString &id, QJsonObject &data);
    bool deleteData(const QString &tableName, const QString &id);

    // Специальные запросы (Лаб 5 и 6)
    QJsonArray executeSpecialQuery(const QString &lab, const QString &queryNumber);
    QJsonObject getSpecialQueriesInfo();

    // Метаданные
    QString getPrimaryKey(const QString &tableName);
    QStringList getAllTables();
    QStringList getColumns(const QString &tableName);

private:
    explicit NoSQLManager(QObject *parent = nullptr);
    QSqlDatabase getDatabase(const QString &tableName);
};

#endif // NOSQLMANAGER_H
