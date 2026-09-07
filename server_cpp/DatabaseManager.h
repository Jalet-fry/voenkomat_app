#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonArray>
#include <QJsonObject>
#include <QSqlRecord>
#include <QThread>
#include <QVariantList>

class DatabaseManager : public QObject
{
    Q_OBJECT
public:
    static DatabaseManager& instance();
    ~DatabaseManager();
    bool connectToDatabase();

    void setNoSqlMode(bool enabled) { m_isNoSqlMode = enabled; }
    bool isNoSqlMode() const { return m_isNoSqlMode; }

    void setNoSqlPath(const QString &path) { m_noSqlPath = path; }
    QString noSqlPath() const { return m_noSqlPath; }

    QString projectRoot() const;

    QJsonArray executeSelect(const QString &queryStr, const QVariantList &params = {});
    QJsonObject executeModify(const QString &queryStr, const QVariantList &params = {});

    QString getPrimaryKeyColumn(const QString &tableName);

private:
    explicit DatabaseManager(QObject *parent = nullptr);
    QString m_host, m_dbName, m_user, m_pass;
    int m_port;
    bool m_isNoSqlMode = false;
    QString m_noSqlPath;
    QSqlDatabase db();
};

#endif
