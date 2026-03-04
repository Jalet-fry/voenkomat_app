// TODO: [REVIEW] OK.
// TODO: [FIX] exportAllTablesToXlsx and other methods are currently stubs (return false/empty).
// TODO: [FIX] generateSQLBackup uses "DELETE FROM" - verify if this is the intended restore behavior (destructive).

#include "BackupManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDateTime>
#include <QDebug>
#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "xlsxdocument.h"
#include "xlsxformat.h"
using namespace QXlsx;

QString BackupManager::getExportsDirectory() const
{
    QString appDir = QApplication::applicationDirPath();
    QString exportsPath = QDir(appDir).absoluteFilePath("exports");
    QDir(exportsPath).mkpath(".");
    return exportsPath;
}

QString BackupManager::getBackupsExportPath(const QString &format) const
{
    QString path = QDir(getExportsDirectory()).absoluteFilePath(QString("backups/%1").arg(format.toLower()));
    QDir(path).mkpath(".");
    return path;
}

BackupManager::BackupManager(DatabaseManager *dbManager) : m_dbManager(dbManager) {}

bool BackupManager::exportAllTables()
{
    if (!m_dbManager || !m_dbManager->isConnected()) { m_lastError = "БД не подключена"; return false; }
    QStringList tables = m_dbManager->getTableList();
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    QString sqlPath = QDir(getBackupsExportPath("sql")).absoluteFilePath(QString("backup_%1.sql").arg(timestamp));

    QFile file(sqlPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QTextStream out(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    out.setCodec("UTF-8");
#endif
    out << "-- Full Backup " << timestamp << "\n";
    foreach (const QString &t, tables) {
        out << generateSQLBackup(t) << "\n";
    }
    file.close();
    return true;
}

QString BackupManager::generateSQLBackup(const QString &tableName)
{
    QString res = QString("-- Table %1\n").arg(tableName);
    res += QString("DELETE FROM public.%1;\n").arg(tableName);
    
    QJsonArray data;
    QStringList cols = m_dbManager->getColumnList(tableName);

    if (m_dbManager->isHttpMode()) {
        data = m_dbManager->fetchTableDataHttp(tableName);
    } else {
        QSqlQuery query = m_dbManager->executeQuery(QString("SELECT * FROM %1").arg(tableName));
        while (query.next()) {
            QJsonObject obj;
            for(int i=0; i<cols.size(); ++i) obj[cols[i]] = QJsonValue::fromVariant(query.value(i));
            data.append(obj);
        }
    }

    if (cols.isEmpty()) return res;

    for (int i = 0; i < data.size(); ++i) {
        QJsonObject obj = data[i].toObject();
        QStringList vals;
        foreach (const QString &col, cols) {
            QVariant v = obj[col].toVariant();
            if (v.isNull() || v.toString().isEmpty()) vals << "NULL";
            else vals << QString("'%1'").arg(v.toString().replace("'", "''"));
        }
        res += QString("INSERT INTO public.%1 (%2) VALUES (%3);\n").arg(tableName).arg(cols.join(",")).arg(vals.join(","));
    }
    return res;
}

bool BackupManager::restoreFromBackup(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    QTextStream in(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    in.setCodec("UTF-8");
#endif
    QString sql = in.readAll();
    file.close();

    if (m_dbManager->isHttpMode()) {
        QJsonObject json; json["sql"] = sql;
        // ИСПРАВЛЕН АДРЕС: используем m_serverUrl
        QByteArray resp = m_dbManager->sendHttpRequest("POST", m_dbManager->serverUrl() + "/api/restore", QJsonDocument(json).toJson());
        return !resp.isEmpty();
    }

    return executeSQLScript(sql);
}

bool BackupManager::executeSQLScript(const QString &sqlScript)
{
    QStringList commands = sqlScript.split(';', Qt::SkipEmptyParts);
    if (!m_dbManager->beginTransaction()) return false;

    foreach (const QString &cmd, commands) {
        QString trimmed = cmd.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith("--")) continue;
        bool ok;
        m_dbManager->executeQuery(trimmed, &ok);
        if (!ok) {
            m_dbManager->rollbackTransaction();
            m_lastError = m_dbManager->lastError();
            return false;
        }
    }
    return m_dbManager->commitTransaction();
}

bool BackupManager::exportAllTablesToXlsx() { return false; }
bool BackupManager::exportTable(const QString&, const QString&) { return false; }
bool BackupManager::restoreTableFromBackup(const QString&) { return false; }
QString BackupManager::generateTableDDL(const QString&) { return ""; }
QString BackupManager::generateTableDML(const QString&) { return ""; }
QString BackupManager::getQueriesExportPath(const QString &format) const { return ""; }
