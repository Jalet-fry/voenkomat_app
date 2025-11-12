#ifndef BACKUPMANAGER_H
#define BACKUPMANAGER_H

#include "DatabaseManager.h"
#include <QString>
#include <QStringList>

class BackupManager
{
public:
    explicit BackupManager(DatabaseManager *dbManager);
    
    bool exportAllTables();
    bool exportTable(const QString &tableName, const QString &filePath = "");
    bool restoreFromBackup(const QString &filePath);
    bool restoreTableFromBackup(const QString &filePath);
    
    QString lastError() const { return m_lastError; }
    
    // Получить путь к директории exports
    QString getExportsDirectory() const;

private:
    DatabaseManager *m_dbManager;
    QString m_lastError;
    
    QString generateSQLBackup(const QString &tableName);
    QString generateTableDDL(const QString &tableName);
    QString generateTableDML(const QString &tableName);
    bool executeSQLScript(const QString &sqlScript);
};

#endif // BACKUPMANAGER_H

