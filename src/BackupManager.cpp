#include "BackupManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDateTime>
#include <QDebug>
#include <QApplication>

QString BackupManager::getExportsDirectory() const
{
    // Список возможных путей к директории exports (в порядке приоритета)
    QStringList possiblePaths;
    
    // 1. В директории приложения (предпочтительно для релизных сборок)
    QString appDir = QApplication::applicationDirPath();
    possiblePaths << QDir(appDir).absoluteFilePath("exports");
    
    // 2. В текущей рабочей директории (для разработки)
    QDir currentDir = QDir::current();
    possiblePaths << currentDir.absoluteFilePath("exports");
    
    // 3. В директории проекта (на уровень выше от debug/release)
    QDir appDirParent = QDir(appDir);
    if (appDirParent.cdUp()) {
        possiblePaths << appDirParent.absoluteFilePath("exports");
    }
    
    // 4. Проверяем родительские директории от текущей рабочей директории
    QDir parentDir = currentDir;
    for (int i = 0; i < 5; ++i) {
        QString parentPath = parentDir.absoluteFilePath("exports");
        possiblePaths << parentPath;
        if (!parentDir.cdUp()) {
            break;
        }
    }
    
    // Ищем существующую директорию
    foreach (const QString &path, possiblePaths) {
        QDir dir(path);
        if (dir.exists()) {
            return dir.absolutePath();
        }
    }
    
    // Если директория не найдена, создаем её в директории приложения
    QString exportsPath = QDir(appDir).absoluteFilePath("exports");
    QDir exportsDir(exportsPath);
    if (!exportsDir.exists()) {
        if (!exportsDir.mkpath(".")) {
            qDebug() << "Не удалось создать директорию exports:" << exportsPath;
            // Если не удалось создать в директории приложения, пытаемся в текущей
            exportsPath = currentDir.absoluteFilePath("exports");
            exportsDir = QDir(exportsPath);
            exportsDir.mkpath(".");
        }
    }
    
    qDebug() << "Директория exports:" << exportsPath;
    return exportsPath;
}

BackupManager::BackupManager(DatabaseManager *dbManager)
    : m_dbManager(dbManager)
{
}

bool BackupManager::exportAllTables()
{
    if (!m_dbManager) {
        m_lastError = "DatabaseManager не инициализирован";
        return false;
    }
    
    if (!m_dbManager->isConnected()) {
        m_lastError = "База данных не подключена. Проверьте параметры подключения.";
        return false;
    }

    QStringList tables = m_dbManager->getTableList();
    if (tables.isEmpty()) {
        m_lastError = "В базе данных нет таблиц для экспорта";
        return false;
    }

    // Получаем путь к директории exports
    QString exportsDir = getExportsDirectory();
    QDir dir(exportsDir);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            m_lastError = QString("Не удалось создать директорию exports: %1").arg(exportsDir);
            return false;
        }
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss");
    QString sqlPath = QDir(exportsDir).absoluteFilePath(QString("full_export_%1.sql").arg(timestamp));

    QFile file(sqlPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = QString("Не удалось создать файл: %1\nОшибка: %2")
                      .arg(sqlPath).arg(file.errorString());
        return false;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << QString("-- Полный экспорт базы данных от %1\n\n").arg(timestamp);

    foreach (const QString &table, tables) {
        out << generateSQLBackup(table);
        out << "\n";
    }

    file.close();
    return true;
}

bool BackupManager::exportTable(const QString &tableName, const QString &filePath)
{
    if (!m_dbManager || !m_dbManager->isConnected()) {
        m_lastError = "База данных не подключена";
        return false;
    }

    QString sqlContent = generateSQLBackup(tableName);
    
    QString finalPath = filePath;
    if (finalPath.isEmpty()) {
        // Получаем путь к директории exports
        QString exportsDir = getExportsDirectory();
        QDir dir(exportsDir);
        if (!dir.exists()) {
            if (!dir.mkpath(".")) {
                m_lastError = QString("Не удалось создать директорию exports: %1").arg(exportsDir);
                return false;
            }
        }
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss");
        finalPath = QDir(exportsDir).absoluteFilePath(QString("%1_backup_%2.sql").arg(tableName).arg(timestamp));
    }

    QFile file(finalPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = "Не удалось создать файл: " + finalPath;
        return false;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << sqlContent;
    file.close();

    return true;
}

QString BackupManager::generateSQLBackup(const QString &tableName)
{
    QString result;
    result += QString("-- Резервная копия таблицы %1\n").arg(tableName);
    result += generateTableDDL(tableName);
    result += "\n";
    result += generateTableDML(tableName);
    return result;
}

QString BackupManager::generateTableDDL(const QString &tableName)
{
    QString ddl;
    
    // Удаление таблицы если существует
    ddl += QString("DROP TABLE IF EXISTS %1 CASCADE;\n\n").arg(tableName);
    
    // Создание таблицы
    ddl += QString("CREATE TABLE %1 (\n").arg(tableName);

    // Получаем информацию о колонках с полными определениями
    QStringList columnNames = m_dbManager->getColumnList(tableName);
    QStringList columnDefs;
    
    foreach (const QString &columnName, columnNames) {
        QString colDef = m_dbManager->getColumnDefinition(tableName, columnName);
        if (colDef.isEmpty()) {
            // Fallback на старый метод если новый не сработал
            QList<QPair<QString, QString>> columns = m_dbManager->getColumnInfo(tableName);
            foreach (const auto &col, columns) {
                if (col.first == columnName) {
                    colDef = QString("%1 %2").arg(col.first).arg(col.second);
                    break;
                }
            }
        } else {
            colDef = QString("%1 %2").arg(columnName).arg(colDef);
        }
        columnDefs << QString("    %1").arg(colDef);
    }
    
    // Получаем PRIMARY KEY
    QStringList primaryKeys = m_dbManager->getPrimaryKeys(tableName);
    if (!primaryKeys.isEmpty()) {
        columnDefs << QString("    PRIMARY KEY (%1)").arg(primaryKeys.join(", "));
    }
    
    ddl += columnDefs.join(",\n");
    ddl += "\n);\n\n";
    
    // Добавляем UNIQUE constraints
    QStringList uniqueConstraints = m_dbManager->getUniqueConstraints(tableName);
    foreach (const QString &uc, uniqueConstraints) {
        // Формат: constraint_name(columns)
        int parenPos = uc.indexOf('(');
        if (parenPos > 0) {
            QString constraintName = uc.left(parenPos);
            QString columns = uc.mid(parenPos);
            ddl += QString("ALTER TABLE %1 ADD CONSTRAINT %2 UNIQUE %3;\n")
                .arg(tableName).arg(constraintName).arg(columns);
        }
    }
    
    if (!uniqueConstraints.isEmpty()) {
        ddl += "\n";
    }
    
    // Добавляем FOREIGN KEY constraints
    QStringList foreignKeys = m_dbManager->getForeignKeys(tableName);
    foreach (const QString &fk, foreignKeys) {
        // Формат: constraint_name(column) REFERENCES table(column)
        int refPos = fk.indexOf("REFERENCES");
        if (refPos > 0) {
            QString beforeRef = fk.left(refPos).trimmed();
            QString afterRef = fk.mid(refPos + 10).trimmed();
            
            int parenPos = beforeRef.indexOf('(');
            if (parenPos > 0) {
                QString constraintName = beforeRef.left(parenPos);
                QString column = beforeRef.mid(parenPos);
                
                ddl += QString("ALTER TABLE %1 ADD CONSTRAINT %2 FOREIGN KEY %3 REFERENCES %4;\n")
                    .arg(tableName).arg(constraintName).arg(column).arg(afterRef);
            }
        }
    }
    
    if (!foreignKeys.isEmpty()) {
        ddl += "\n";
    }
    
    // Добавляем индексы (кроме PRIMARY KEY)
    QStringList indexes = m_dbManager->getIndexes(tableName);
    foreach (const QString &indexDef, indexes) {
        ddl += indexDef;
        ddl += ";\n";
    }
    
    if (!indexes.isEmpty()) {
        ddl += "\n";
    }
    
    // Добавляем sequences с их текущими значениями
    QList<DatabaseManager::SequenceInfo> sequences = m_dbManager->getSequenceInfo(tableName);
    foreach (const DatabaseManager::SequenceInfo &seqInfo, sequences) {
        if (!seqInfo.sequenceName.isEmpty() && seqInfo.currentValue > 0) {
            // sequenceName уже содержит полное имя (schema.sequence_name)
            // Устанавливаем текущее значение sequence (false означает, что следующее значение будет currentValue + 1)
            ddl += QString("SELECT setval('%1', %2, false);\n")
                .arg(seqInfo.sequenceName)
                .arg(seqInfo.currentValue);
        }
    }
    
    if (!sequences.isEmpty()) {
        ddl += "\n";
    }
    
    return ddl;
}

QString BackupManager::generateTableDML(const QString &tableName)
{
    QString dml;
    
    QSqlQuery query = m_dbManager->executeQuery(QString("SELECT * FROM %1").arg(tableName));
    if (query.lastError().isValid()) {
        m_lastError = query.lastError().text();
        return "";
    }

    QStringList columnNames = m_dbManager->getColumnList(tableName);
    if (columnNames.isEmpty()) {
        return "";
    }

    dml += QString("INSERT INTO %1 (%2) VALUES\n").arg(tableName).arg(columnNames.join(", "));

    QStringList valueRows;
    while (query.next()) {
        QStringList values;
        for (int i = 0; i < columnNames.size(); ++i) {
            QVariant value = query.value(i);
            if (value.isNull()) {
                values << "NULL";
            } else if (value.type() == QVariant::Int || value.type() == QVariant::Double) {
                values << value.toString();
            } else {
                QString str = value.toString();
                str.replace("'", "''");
                values << QString("'%1'").arg(str);
            }
        }
        valueRows << QString("(%1)").arg(values.join(", "));
    }

    if (!valueRows.isEmpty()) {
        dml += valueRows.join(",\n");
        dml += ";\n";
    }

    return dml;
}

bool BackupManager::restoreFromBackup(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_lastError = "Не удалось открыть файл: " + filePath;
        return false;
    }

    QTextStream in(&file);
    in.setCodec("UTF-8");
    QString sqlScript = in.readAll();
    file.close();

    return executeSQLScript(sqlScript);
}

bool BackupManager::restoreTableFromBackup(const QString &filePath)
{
    return restoreFromBackup(filePath);
}

bool BackupManager::executeSQLScript(const QString &sqlScript)
{
    if (!m_dbManager || !m_dbManager->isConnected()) {
        m_lastError = "База данных не подключена";
        return false;
    }

    // Разбиваем скрипт на отдельные команды
    QStringList commands;
    QString currentCommand;
    foreach (QChar c, sqlScript) {
        if (c == ';') {
            QString trimmed = currentCommand.trimmed();
            if (!trimmed.isEmpty() && !trimmed.startsWith("--")) {
                commands << trimmed;
            }
            currentCommand.clear();
        } else {
            currentCommand += c;
        }
    }
    
    // Добавляем последнюю команду, если скрипт не заканчивается на ';'
    QString lastCommand = currentCommand.trimmed();
    if (!lastCommand.isEmpty() && !lastCommand.startsWith("--")) {
        commands << lastCommand;
    }
    
    if (!m_dbManager->beginTransaction()) {
        m_lastError = "Не удалось начать транзакцию";
        return false;
    }

    foreach (const QString &command, commands) {
        QString trimmed = command.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith("--")) {
            continue;
        }

        bool ok;
        m_dbManager->executeQuery(trimmed, &ok);
        if (!ok) {
            m_dbManager->rollbackTransaction();
            m_lastError = QString("Ошибка при выполнении команды:\n%1\n\nТекст команды:\n%2")
                         .arg(m_dbManager->lastError())
                         .arg(trimmed.left(200)); // Показываем первые 200 символов команды
            return false;
        }
    }

    if (!m_dbManager->commitTransaction()) {
        m_lastError = "Не удалось зафиксировать транзакцию";
        m_dbManager->rollbackTransaction();
        return false;
    }

    return true;
}

