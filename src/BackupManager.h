// TODO: [REVIEW] OK.
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
    bool exportAllTablesToXlsx();
    bool exportTable(const QString &tableName, const QString &filePath = "");
    bool restoreFromBackup(const QString &filePath);
    bool restoreTableFromBackup(const QString &filePath);
    
    QString lastError() const { return m_lastError; }
    
    // Получить путь к директории exports
    QString getExportsDirectory() const;
    
    // Получить путь для экспорта таблиц (xlsx/csv)
    QString getTablesExportPath(const QString &format) const;
    
    // Получить путь для экспорта запросов (xlsx/csv)
    QString getQueriesExportPath(const QString &format) const;
    
    // Получить путь для резервных копий (sql/xlsx)
    QString getBackupsExportPath(const QString &format) const;

private:
    DatabaseManager *m_dbManager;
    QString m_lastError;
    
    QString generateSQLBackup(const QString &tableName);
    QString generateTableDDL(const QString &tableName);
    QString generateTableDML(const QString &tableName);
    bool executeSQLScript(const QString &sqlScript);
};

#endif // BACKUPMANAGER_H
