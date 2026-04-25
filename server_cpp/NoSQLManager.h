#ifndef NOSQLMANAGER_H
#define NOSQLMANAGER_H

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVariant>

class NoSQLManager : public QObject
{
    Q_OBJECT
public:
    static NoSQLManager& instance();

    // Основные операции
    QJsonArray getData(const QString &tableName, const QString &filterStr = "", int limit = 0);
    bool insertData(const QString &tableName, const QJsonObject &data);
    bool updateData(const QString &tableName, const QString &id, const QJsonObject &data);
    bool deleteData(const QString &tableName, const QString &id);

    // Метаданные
    QString getPrimaryKey(const QString &tableName);
    QStringList getAllTables();
    QStringList getColumns(const QString &tableName);

private:
    explicit NoSQLManager(QObject *parent = nullptr);
    QString getDbPath(const QString &tableName);
    bool applyFilter(const QJsonObject &obj, const QString &filterStr);
    bool compare(const QVariant &actual, const QString &op, const QString &target);
};

#endif // NOSQLMANAGER_H
