#include "DatabaseManager.h"
#include <QDebug>
#include <QSqlDriver>
#include <QSqlError>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QPluginLoader>
#include <QLibrary>

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
{
    // Проверяем доступность драйвера QPSQL
    if (!QSqlDatabase::isDriverAvailable("QPSQL")) {
        m_lastError = "Драйвер QPSQL (PostgreSQL) не доступен. Убедитесь, что Qt был собран с поддержкой PostgreSQL.";
        qDebug() << m_lastError;
    }
    m_db = QSqlDatabase::addDatabase("QPSQL", "voenkomat_connection");
}

DatabaseManager::~DatabaseManager()
{
    // Сначала закрываем соединение
    disconnect();
    
    // Ждем немного, чтобы все запросы завершились
    QCoreApplication::processEvents();
    
    // Удаляем соединение из пула Qt для полного освобождения ресурсов
    // Важно: удаляем только если соединение закрыто
    if (QSqlDatabase::contains("voenkomat_connection")) {
        QSqlDatabase db = QSqlDatabase::database("voenkomat_connection", false);
        if (!db.isOpen()) {
            QSqlDatabase::removeDatabase("voenkomat_connection");
        }
    }
}

bool DatabaseManager::connectToDatabase(const QString &host,
                                       const QString &port,
                                       const QString &database,
                                       const QString &username,
                                       const QString &password)
{
    // ВСЕГДА пытаемся явно загрузить драйвер, даже если он в списке доступных
    // Потому что иногда драйвер в списке, но не может загрузиться при использовании
    qDebug() << "=== Попытка явной загрузки драйвера QPSQL ===";
    
    // Сначала проверяем, может ли загрузиться libpq.dll
    QString appDir = QCoreApplication::applicationDirPath();
    QString libpqPath = QDir(appDir).absoluteFilePath("libpq.dll");
    
    if (QFileInfo::exists(libpqPath)) {
        qDebug() << "Проверка загрузки libpq.dll:" << libpqPath;
        QLibrary libpqLib(libpqPath);
        if (libpqLib.load()) {
            qDebug() << "libpq.dll успешно загружен";
            libpqLib.unload();
        } else {
            qDebug() << "ОШИБКА: libpq.dll не может загрузиться:" << libpqLib.errorString();
            QString errorText = libpqLib.errorString();
            bool isArchitectureError = errorText.contains("Win32", Qt::CaseInsensitive) || 
                                      errorText.contains("не является приложением", Qt::CaseInsensitive);
            
            if (isArchitectureError) {
                m_lastError = QString("КРИТИЧЕСКАЯ ОШИБКА: Несовместимость архитектур!\n\n")
                            + QString("Ошибка: %1\n\n").arg(errorText)
                            + QString("ПРОБЛЕМА:\n")
                            + QString("- Ваше приложение: 32-bit (Qt 5.5.1 MinGW 32-bit)\n")
                            + QString("- libpq.dll: 64-bit (от PostgreSQL 17)\n")
                            + QString("- 32-битное приложение НЕ МОЖЕТ загрузить 64-битную DLL\n\n")
                            + QString("РЕШЕНИЕ:\n")
                            + QString("1. Скачайте PostgreSQL 12 или 13 (32-bit версию)\n")
                            + QString("2. Установите только клиентские библиотеки (Command Line Tools)\n")
                            + QString("3. Скопируйте libpq.dll из:\n")
                            + QString("   C:\\Program Files (x86)\\PostgreSQL\\12\\bin\\\n")
                            + QString("   В папку: %1\n\n").arg(appDir)
                            + QString("Подробная инструкция: см. файл GUIDE_32BIT_LIBPQ.md\n")
                            + QString("\nПримечание: 32-битный клиент от PostgreSQL 12/13\n")
                            + QString("может подключиться к PostgreSQL 17 серверу (протокол совместим)");
            } else {
                m_lastError = QString("Критическая ошибка: libpq.dll не может загрузиться.\n")
                            + QString("Ошибка: %1\n\n").arg(errorText)
                            + QString("Возможные причины:\n")
                            + QString("1. Отсутствуют зависимости libpq.dll\n")
                            + QString("2. Поврежденный файл libpq.dll\n")
                            + QString("3. Неправильная архитектура (32-bit vs 64-bit)\n\n")
                            + QString("РЕШЕНИЕ: См. файл GUIDE_32BIT_LIBPQ.md");
            }
            return false;
        }
    } else {
        qDebug() << "libpq.dll не найден в папке приложения:" << appDir;
    }
    
    // Пытаемся загрузить драйвер вручную
    QStringList libraryPaths = QCoreApplication::libraryPaths();
    bool driverLoaded = false;
    QString lastLoaderError;
    
    foreach (const QString &path, libraryPaths) {
        QString driverPath = QDir(path).absoluteFilePath("sqldrivers/qsqlpsqld.dll");
        if (QFileInfo::exists(driverPath)) {
            qDebug() << "Попытка явной загрузки драйвера:" << driverPath;
            QPluginLoader loader(driverPath);
            if (loader.load()) {
                qDebug() << "Драйвер успешно загружен через QPluginLoader";
                driverLoaded = true;
                break;
            } else {
                lastLoaderError = loader.errorString();
                qDebug() << "Ошибка загрузки драйвера:" << lastLoaderError;
                
                // Проверяем зависимости через QLibrary
                QLibrary lib(driverPath);
                if (!lib.load()) {
                    qDebug() << "Ошибка загрузки библиотеки:" << lib.errorString();
                    lastLoaderError += "\nQLibrary ошибка: " + lib.errorString();
                }
            }
        }
    }
    
    // Проверяем доступность драйвера после попытки загрузки
    if (!driverLoaded && !QSqlDatabase::isDriverAvailable("QPSQL")) {
        QString errorMsg = "Драйвер QPSQL (PostgreSQL) не доступен.\n\n";
        
        // Выводим диагностическую информацию
        qDebug() << "=== Диагностика загрузки драйвера QPSQL ===";
        
        if (!lastLoaderError.isEmpty()) {
            qDebug() << "Ошибка QPluginLoader:" << lastLoaderError;
            errorMsg += "Ошибка загрузки драйвера: " + lastLoaderError + "\n\n";
        }
        
        // Список доступных драйверов
        QStringList availableDrivers = QSqlDatabase::drivers();
        qDebug() << "Доступные драйверы:" << availableDrivers;
        
        // Пути к плагинам
        qDebug() << "Пути к библиотекам Qt:";
        foreach (const QString &path, libraryPaths) {
            qDebug() << "  -" << path;
            QString sqldriversPath = QDir(path).absoluteFilePath("sqldrivers");
            qDebug() << "    sqldrivers:" << sqldriversPath << (QDir(sqldriversPath).exists() ? "(существует)" : "(не найден)");
            
            // Проверяем наличие DLL файлов драйвера
            QString driverDll = QDir(sqldriversPath).absoluteFilePath("qsqlpsqld.dll");
            if (QFileInfo::exists(driverDll)) {
                qDebug() << "    Найден драйвер:" << driverDll;
            } else {
                qDebug() << "    Драйвер не найден:" << driverDll;
            }
        }
        
        // Переменная окружения
        QString pluginPathEnv = QString::fromLocal8Bit(qgetenv("QT_PLUGIN_PATH"));
        if (!pluginPathEnv.isEmpty()) {
            qDebug() << "QT_PLUGIN_PATH:" << pluginPathEnv;
        } else {
            qDebug() << "QT_PLUGIN_PATH не установлена";
        }
        
        errorMsg += "Возможные причины:\n";
        errorMsg += "1. Несовместимость Qt 5.5.1 (2015) с PostgreSQL 17 (2023) - слишком большая разница в версиях\n";
        errorMsg += "2. Отсутствуют зависимости Qt (Qt5Sql.dll, Qt5Core.dll)\n";
        errorMsg += "3. Проблема с архитектурой (32-bit vs 64-bit)\n\n";
        errorMsg += "Рекомендация: Используйте более новую версию Qt (5.12+) или более старую версию PostgreSQL (12-14)";
        
        m_lastError = errorMsg;
        qDebug() << m_lastError;
        return false;
    }

    if (m_db.isOpen()) {
        m_db.close();
    }

    // Проверяем пустой пароль
    if (password.isEmpty()) {
        m_lastError = "Пароль не указан. Установите переменную окружения PGPASSWORD (в PowerShell: $env:PGPASSWORD = \"ваш_пароль\")";
        qDebug() << m_lastError;
        return false;
    }

    m_db.setHostName(host);
    m_db.setPort(port.toInt());
    m_db.setDatabaseName(database);
    m_db.setUserName(username);
    m_db.setPassword(password);

    qDebug() << "Попытка подключения к БД:" << host << ":" << port << "/" << database << "пользователь:" << username;

    if (!m_db.open()) {
        QSqlError error = m_db.lastError();
        m_lastError = error.text();
        
        // Специальная обработка ошибки "Driver not loaded"
        if (m_lastError.contains("Driver not loaded", Qt::CaseInsensitive)) {
            // Пытаемся загрузить драйвер явно прямо сейчас
            qDebug() << "=== КРИТИЧЕСКАЯ ОШИБКА: Драйвер не загружен ===";
            qDebug() << "Попытка экстренной загрузки драйвера...";
            
            QStringList libraryPaths = QCoreApplication::libraryPaths();
            bool emergencyLoaded = false;
            foreach (const QString &path, libraryPaths) {
                QString driverPath = QDir(path).absoluteFilePath("sqldrivers/qsqlpsqld.dll");
                if (QFileInfo::exists(driverPath)) {
                    QPluginLoader loader(driverPath);
                    if (loader.load()) {
                        qDebug() << "Драйвер загружен в экстренном режиме!";
                        emergencyLoaded = true;
                        // Пытаемся подключиться снова
                        if (m_db.open()) {
                            qDebug() << "Подключение успешно после экстренной загрузки!";
                            return true;
                        }
                        break;
                    } else {
                        qDebug() << "Экстренная загрузка не удалась:" << loader.errorString();
                    }
                }
            }
            
            QString diagnosticMsg = "\n\n=== Диагностика загрузки драйвера ===\n";
            
            // Пути к плагинам (используем уже объявленную переменную libraryPaths)
            diagnosticMsg += "Пути к библиотекам Qt:\n";
            foreach (const QString &path, libraryPaths) {
                diagnosticMsg += "  - " + path + "\n";
                QString sqldriversPath = QDir(path).absoluteFilePath("sqldrivers");
                bool sqldriversExists = QDir(sqldriversPath).exists();
                diagnosticMsg += "    sqldrivers: " + sqldriversPath;
                diagnosticMsg += sqldriversExists ? " (существует)\n" : " (не найден)\n";
                
                if (sqldriversExists) {
                    QString driverDll = QDir(sqldriversPath).absoluteFilePath("qsqlpsqld.dll");
                    bool driverExists = QFileInfo::exists(driverDll);
                    diagnosticMsg += "    Драйвер: " + driverDll;
                    diagnosticMsg += driverExists ? " (найден)\n" : " (не найден)\n";
                }
            }
            
            // Проверка зависимостей PostgreSQL
            QString appDir = QCoreApplication::applicationDirPath();
            QStringList requiredDlls;
            requiredDlls << "libpq.dll";  // Обязательный
            QStringList optionalDlls;
            optionalDlls << "libiconv-2.dll" << "libintl-8.dll";  // Опциональные
            
            diagnosticMsg += "\nПроверка зависимостей PostgreSQL в папке приложения:\n";
            diagnosticMsg += "  Папка: " + appDir + "\n";
            
            bool criticalDllFound = false;
            foreach (const QString &dll, requiredDlls) {
                QString dllPath = QDir(appDir).absoluteFilePath(dll);
                bool dllExists = QFileInfo::exists(dllPath);
                diagnosticMsg += "  " + dll + " (обязательный): ";
                if (dllExists) {
                    diagnosticMsg += "найден\n";
                    criticalDllFound = true;
                } else {
                    diagnosticMsg += "НЕ НАЙДЕН - КРИТИЧНО!\n";
                }
            }
            
            foreach (const QString &dll, optionalDlls) {
                QString dllPath = QDir(appDir).absoluteFilePath(dll);
                bool dllExists = QFileInfo::exists(dllPath);
                diagnosticMsg += "  " + dll + " (опциональный): ";
                if (dllExists) {
                    diagnosticMsg += "найден\n";
                } else {
                    diagnosticMsg += "не найден (не критично)\n";
                }
            }
            
            // Проверка в стандартной папке PostgreSQL
            QStringList pgPaths;
            pgPaths << "C:/Program Files/PostgreSQL/17/bin"
                   << "C:/Program Files/PostgreSQL/16/bin"
                   << "C:/Program Files/PostgreSQL/15/bin"
                   << "C:/Program Files/PostgreSQL/14/bin";
            
            diagnosticMsg += "\nПоиск libpq.dll в стандартных папках PostgreSQL:\n";
            bool pgFound = false;
            foreach (const QString &pgPath, pgPaths) {
                QString libpqPath = QDir(pgPath).absoluteFilePath("libpq.dll");
                if (QFileInfo::exists(libpqPath)) {
                    diagnosticMsg += "  Найден: " + libpqPath + "\n";
                    pgFound = true;
                    break;
                }
            }
            if (!pgFound) {
                diagnosticMsg += "  PostgreSQL библиотеки не найдены в стандартных папках\n";
            }
            
            // Переменная окружения
            QString pluginPathEnv = QString::fromLocal8Bit(qgetenv("QT_PLUGIN_PATH"));
            if (!pluginPathEnv.isEmpty()) {
                diagnosticMsg += "\nQT_PLUGIN_PATH: " + pluginPathEnv + "\n";
            }
            
            diagnosticMsg += "\n=== РЕШЕНИЕ ===\n";
            if (!criticalDllFound) {
                diagnosticMsg += "ОТСУТСТВУЕТ КРИТИЧЕСКАЯ ЗАВИСИМОСТЬ: libpq.dll!\n\n";
                diagnosticMsg += "Выполните одно из действий:\n";
                diagnosticMsg += "1. Запустите скрипт: .\\copy_drivers.ps1\n";
                diagnosticMsg += "2. Или скопируйте вручную из:\n";
                diagnosticMsg += "   C:\\Program Files\\PostgreSQL\\17\\bin\\libpq.dll\n";
                diagnosticMsg += "   В папку: " + appDir + "\n";
            } else {
                diagnosticMsg += "Основные зависимости найдены, но драйвер все равно не загружается.\n\n";
                diagnosticMsg += "ВОЗМОЖНАЯ ПРИЧИНА: Несовместимость версий!\n";
                diagnosticMsg += "- Qt 5.5.1 (2015 год) - очень старая версия\n";
                diagnosticMsg += "- PostgreSQL 17 (2023 год) - очень новая версия\n";
                diagnosticMsg += "- Разница в 8 лет может вызывать проблемы совместимости\n\n";
                diagnosticMsg += "РЕШЕНИЯ:\n";
                diagnosticMsg += "1. Используйте PostgreSQL 12-14 (совместимы с Qt 5.5.1)\n";
                diagnosticMsg += "2. Или обновите Qt до версии 5.12+ (совместима с PostgreSQL 17)\n";
                diagnosticMsg += "3. Проверьте ошибку QPluginLoader в логах выше\n";
            }
            
            m_lastError += diagnosticMsg;
            qDebug() << diagnosticMsg;
        }
        
        // Добавляем более подробную информацию об ошибке
        if (error.type() == QSqlError::ConnectionError) {
            if (m_lastError.contains("could not connect", Qt::CaseInsensitive)) {
                m_lastError += "\nВозможно, PostgreSQL сервер не запущен или недоступен по указанному адресу.";
            } else if (m_lastError.contains("password", Qt::CaseInsensitive)) {
                m_lastError += "\nПроверьте правильность пароля в переменной окружения PGPASSWORD.";
            } else if (m_lastError.contains("database", Qt::CaseInsensitive)) {
                m_lastError += "\nПроверьте, существует ли база данных '" + database + "'.";
            }
        }
        
        qDebug() << "Database connection error:" << m_lastError;
        qDebug() << "Error type:" << error.type();
        qDebug() << "Error number:" << error.number();
        return false;
    }

    m_lastError.clear();
    qDebug() << "Успешное подключение к базе данных";
    return true;
}

bool DatabaseManager::isConnected() const
{
    return m_db.isOpen();
}

void DatabaseManager::disconnect()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

QSqlQuery DatabaseManager::executeQuery(const QString &query, bool *ok)
{
    QSqlQuery sqlQuery(m_db);
    
    if (!sqlQuery.exec(query)) {
        m_lastError = sqlQuery.lastError().text();
        qDebug() << "Query execution error:" << m_lastError;
        if (ok) *ok = false;
        return sqlQuery;
    }

    if (ok) *ok = true;
    return sqlQuery;
}

QSqlQuery DatabaseManager::prepareQuery(const QString &query)
{
    QSqlQuery sqlQuery(m_db);
    sqlQuery.prepare(query);
    return sqlQuery;
}

bool DatabaseManager::executePreparedQuery(QSqlQuery &query)
{
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qDebug() << "Prepared query execution error:" << m_lastError;
        return false;
    }
    return true;
}

QString DatabaseManager::lastError() const
{
    return m_lastError;
}

QSqlDatabase DatabaseManager::database() const
{
    return m_db;
}

bool DatabaseManager::beginTransaction()
{
    return m_db.transaction();
}

bool DatabaseManager::commitTransaction()
{
    return m_db.commit();
}

bool DatabaseManager::rollbackTransaction()
{
    return m_db.rollback();
}

QStringList DatabaseManager::getTableList()
{
    QStringList tables;
    
    QSqlQuery query(m_db);
    query.prepare("SELECT table_name FROM information_schema.tables "
                  "WHERE table_schema = 'public' AND table_type = 'BASE TABLE' "
                  "ORDER BY table_name");
    
    if (query.exec()) {
        while (query.next()) {
            tables << query.value(0).toString();
        }
    } else {
        m_lastError = query.lastError().text();
    }
    
    return tables;
}

QStringList DatabaseManager::getColumnList(const QString &tableName)
{
    QStringList columns;
    
    QSqlQuery query(m_db);
    query.prepare("SELECT column_name FROM information_schema.columns "
                  "WHERE table_schema = 'public' AND table_name = :table_name "
                  "ORDER BY ordinal_position");
    query.bindValue(":table_name", tableName);
    
    if (query.exec()) {
        while (query.next()) {
            columns << query.value(0).toString();
        }
    } else {
        m_lastError = query.lastError().text();
    }
    
    return columns;
}

QList<QPair<QString, QString>> DatabaseManager::getColumnInfo(const QString &tableName)
{
    QList<QPair<QString, QString>> columnInfo;
    
    QSqlQuery query(m_db);
    query.prepare("SELECT column_name, data_type FROM information_schema.columns "
                  "WHERE table_schema = 'public' AND table_name = :table_name "
                  "ORDER BY ordinal_position");
    query.bindValue(":table_name", tableName);
    
    if (query.exec()) {
        while (query.next()) {
            columnInfo << qMakePair(query.value(0).toString(), query.value(1).toString());
        }
    } else {
        m_lastError = query.lastError().text();
    }
    
    return columnInfo;
}

QStringList DatabaseManager::getPrimaryKeys(const QString &tableName)
{
    QStringList primaryKeys;
    
    QSqlQuery query(m_db);
    query.prepare(
        "SELECT a.attname "
        "FROM pg_index i "
        "JOIN pg_attribute a ON a.attrelid = i.indrelid AND a.attnum = ANY(i.indkey) "
        "WHERE i.indrelid = :table_name::regclass "
        "AND i.indisprimary "
        "ORDER BY a.attnum"
    );
    query.bindValue(":table_name", tableName);
    
    if (query.exec()) {
        while (query.next()) {
            primaryKeys << query.value(0).toString();
        }
    } else {
        m_lastError = query.lastError().text();
    }
    
    return primaryKeys;
}

QString DatabaseManager::getPrimaryKeyColumn(const QString &tableName)
{
    QStringList primaryKeys = getPrimaryKeys(tableName);
    if (!primaryKeys.isEmpty()) {
        return primaryKeys.first();
    }
    return QString();
}

QList<DatabaseManager::ColumnDetail> DatabaseManager::getColumnDetails(const QString &tableName)
{
    QList<ColumnDetail> columnDetails;
    
    QSqlQuery query(m_db);
    query.prepare(
        "SELECT "
        "    column_name, "
        "    data_type, "
        "    is_nullable, "
        "    column_default, "
        "    character_maximum_length "
        "FROM information_schema.columns "
        "WHERE table_schema = 'public' "
        "    AND table_name = :table_name "
        "ORDER BY ordinal_position"
    );
    query.bindValue(":table_name", tableName);
    
    if (query.exec()) {
        while (query.next()) {
            ColumnDetail detail;
            detail.columnName = query.value(0).toString();
            detail.dataType = query.value(1).toString();
            detail.isNullable = (query.value(2).toString() == "YES");
            detail.defaultValue = query.value(3);
            if (query.value(4).isValid() && !query.value(4).isNull()) {
                detail.characterMaxLength = query.value(4).toInt();
            } else {
                detail.characterMaxLength = -1;
            }
            columnDetails << detail;
        }
    } else {
        m_lastError = query.lastError().text();
    }
    
    return columnDetails;
}

QStringList DatabaseManager::getForeignKeys(const QString &tableName)
{
    QStringList foreignKeys;
    
    QSqlQuery query(m_db);
    query.prepare(
        "SELECT "
        "    tc.constraint_name, "
        "    kcu.column_name, "
        "    ccu.table_name AS foreign_table_name, "
        "    ccu.column_name AS foreign_column_name "
        "FROM information_schema.table_constraints AS tc "
        "JOIN information_schema.key_column_usage AS kcu "
        "    ON tc.constraint_name = kcu.constraint_name "
        "    AND tc.table_schema = kcu.table_schema "
        "JOIN information_schema.constraint_column_usage AS ccu "
        "    ON ccu.constraint_name = tc.constraint_name "
        "    AND ccu.table_schema = tc.table_schema "
        "WHERE tc.constraint_type = 'FOREIGN KEY' "
        "    AND tc.table_schema = 'public' "
        "    AND tc.table_name = :table_name"
    );
    query.bindValue(":table_name", tableName);
    
    if (query.exec()) {
        while (query.next()) {
            QString fk = QString("%1(%2) REFERENCES %3(%4)")
                .arg(query.value(0).toString())
                .arg(query.value(1).toString())
                .arg(query.value(2).toString())
                .arg(query.value(3).toString());
            foreignKeys << fk;
        }
    } else {
        m_lastError = query.lastError().text();
    }
    
    return foreignKeys;
}

QList<DatabaseManager::ForeignKeyInfo> DatabaseManager::getForeignKeyInfo(const QString &tableName)
{
    QList<ForeignKeyInfo> fkInfoList;
    
    QSqlQuery query(m_db);
    query.prepare(
        "SELECT "
        "    tc.constraint_name, "
        "    kcu.column_name, "
        "    ccu.table_name AS foreign_table_name, "
        "    ccu.column_name AS foreign_column_name, "
        "    rc.delete_rule "
        "FROM information_schema.table_constraints AS tc "
        "JOIN information_schema.key_column_usage AS kcu "
        "    ON tc.constraint_name = kcu.constraint_name "
        "    AND tc.table_schema = kcu.table_schema "
        "JOIN information_schema.constraint_column_usage AS ccu "
        "    ON ccu.constraint_name = tc.constraint_name "
        "    AND ccu.table_schema = tc.table_schema "
        "LEFT JOIN information_schema.referential_constraints AS rc "
        "    ON rc.constraint_name = tc.constraint_name "
        "    AND rc.constraint_schema = tc.table_schema "
        "WHERE tc.constraint_type = 'FOREIGN KEY' "
        "    AND tc.table_schema = 'public' "
        "    AND tc.table_name = :table_name"
    );
    query.bindValue(":table_name", tableName);
    
    if (query.exec()) {
        while (query.next()) {
            ForeignKeyInfo fkInfo;
            fkInfo.constraintName = query.value(0).toString();
            fkInfo.columnName = query.value(1).toString();
            fkInfo.referencedTable = query.value(2).toString();
            fkInfo.referencedColumn = query.value(3).toString();
            fkInfo.deleteRule = query.value(4).toString();
            if (fkInfo.deleteRule.isEmpty()) {
                fkInfo.deleteRule = "NO ACTION";
            }
            fkInfoList << fkInfo;
        }
    } else {
        m_lastError = query.lastError().text();
    }
    
    return fkInfoList;
}

QString DatabaseManager::getForeignKeyConstraintName(const QString &tableName, const QString &columnName)
{
    QSqlQuery query(m_db);
    query.prepare(
        "SELECT tc.constraint_name "
        "FROM information_schema.table_constraints AS tc "
        "JOIN information_schema.key_column_usage AS kcu "
        "    ON tc.constraint_name = kcu.constraint_name "
        "    AND tc.table_schema = kcu.table_schema "
        "WHERE tc.constraint_type = 'FOREIGN KEY' "
        "    AND tc.table_schema = 'public' "
        "    AND tc.table_name = :table_name "
        "    AND kcu.column_name = :column_name "
        "LIMIT 1"
    );
    query.bindValue(":table_name", tableName);
    query.bindValue(":column_name", columnName);
    
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    
    return QString();
}

QString DatabaseManager::getForeignKeyDeleteRule(const QString &tableName, const QString &constraintName)
{
    QSqlQuery query(m_db);
    query.prepare(
        "SELECT delete_rule "
        "FROM information_schema.referential_constraints "
        "WHERE constraint_schema = 'public' "
        "    AND constraint_name = :constraint_name"
    );
    query.bindValue(":constraint_name", constraintName);
    
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    
    return "NO ACTION";
}

bool DatabaseManager::addForeignKey(const QString &tableName, const QString &columnName,
                                    const QString &referencedTable, const QString &referencedColumn,
                                    const QString &deleteRule)
{
    // Генерируем имя constraint
    QString constraintName = QString("fk_%1_%2").arg(tableName).arg(columnName);
    constraintName = constraintName.replace("\"", "").toLower();
    
    // Проверяем, существует ли уже constraint с таким именем
    QString existingName = getForeignKeyConstraintName(tableName, columnName);
    if (!existingName.isEmpty()) {
        m_lastError = QString("Внешний ключ для колонки %1 уже существует").arg(columnName);
        return false;
    }
    
    // Формируем SQL запрос
    QString sql = QString("ALTER TABLE %1 ADD CONSTRAINT %2 FOREIGN KEY (%3) REFERENCES %4(%5)")
        .arg(escapeIdentifier(tableName))
        .arg(escapeIdentifier(constraintName))
        .arg(escapeIdentifier(columnName))
        .arg(escapeIdentifier(referencedTable))
        .arg(escapeIdentifier(referencedColumn));
    
    // Добавляем ON DELETE правило
    if (deleteRule == "CASCADE" || deleteRule == "RESTRICT" || deleteRule == "SET NULL" || deleteRule == "NO ACTION") {
        sql += QString(" ON DELETE %1").arg(deleteRule);
    }
    
    bool ok;
    executeQuery(sql, &ok);
    return ok;
}

bool DatabaseManager::removeForeignKey(const QString &tableName, const QString &constraintName)
{
    QString sql = QString("ALTER TABLE %1 DROP CONSTRAINT %2")
        .arg(escapeIdentifier(tableName))
        .arg(escapeIdentifier(constraintName));
    
    bool ok;
    executeQuery(sql, &ok);
    return ok;
}

bool DatabaseManager::recordExists(const QString &tableName, const QString &columnName, const QVariant &value)
{
    if (value.isNull() || value.toString().isEmpty()) {
        return true; // NULL значения допустимы для внешних ключей
    }
    
    QSqlQuery query(m_db);
    // Используем экранированные идентификаторы для безопасности
    QString sql = QString("SELECT COUNT(*) FROM %1 WHERE %2 = :value")
        .arg(escapeIdentifier(tableName))
        .arg(escapeIdentifier(columnName));
    query.prepare(sql);
    query.bindValue(":value", value);
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    
    return false;
}

QStringList DatabaseManager::getUniqueConstraints(const QString &tableName)
{
    QStringList uniqueConstraints;
    
    QSqlQuery query(m_db);
    query.prepare(
        "SELECT "
        "    tc.constraint_name, "
        "    STRING_AGG(kcu.column_name, ', ' ORDER BY kcu.ordinal_position) AS columns "
        "FROM information_schema.table_constraints AS tc "
        "JOIN information_schema.key_column_usage AS kcu "
        "    ON tc.constraint_name = kcu.constraint_name "
        "    AND tc.table_schema = kcu.table_schema "
        "WHERE tc.constraint_type = 'UNIQUE' "
        "    AND tc.table_schema = 'public' "
        "    AND tc.table_name = :table_name "
        "GROUP BY tc.constraint_name"
    );
    query.bindValue(":table_name", tableName);
    
    if (query.exec()) {
        while (query.next()) {
            QString uc = QString("%1(%2)")
                .arg(query.value(0).toString())
                .arg(query.value(1).toString());
            uniqueConstraints << uc;
        }
    } else {
        m_lastError = query.lastError().text();
    }
    
    return uniqueConstraints;
}

QStringList DatabaseManager::getIndexes(const QString &tableName)
{
    QStringList indexes;
    
    QSqlQuery query(m_db);
    query.prepare(
        "SELECT "
        "    indexname, "
        "    indexdef "
        "FROM pg_indexes "
        "WHERE schemaname = 'public' "
        "    AND tablename = :table_name "
        "    AND indexname NOT LIKE '%_pkey'"
    );
    query.bindValue(":table_name", tableName);
    
    if (query.exec()) {
        while (query.next()) {
            indexes << query.value(1).toString(); // indexdef содержит полное определение
        }
    } else {
        m_lastError = query.lastError().text();
    }
    
    return indexes;
}

QStringList DatabaseManager::getSequences(const QString &tableName)
{
    QStringList sequences;
    
    QSqlQuery query(m_db);
    query.prepare(
        "SELECT "
        "    column_name, "
        "    column_default "
        "FROM information_schema.columns "
        "WHERE table_schema = 'public' "
        "    AND table_name = :table_name "
        "    AND column_default LIKE 'nextval%'"
    );
    query.bindValue(":table_name", tableName);
    
    if (query.exec()) {
        while (query.next()) {
            QString seq = QString("%1 -> %2")
                .arg(query.value(0).toString())
                .arg(query.value(1).toString());
            sequences << seq;
        }
    } else {
        m_lastError = query.lastError().text();
    }
    
    return sequences;
}

QList<DatabaseManager::SequenceInfo> DatabaseManager::getSequenceInfo(const QString &tableName)
{
    QList<SequenceInfo> sequenceInfoList;
    
    // Получаем информацию о sequences для таблицы
    QSqlQuery query(m_db);
    QString sql = QString(
        "SELECT "
        "    c.column_name, "
        "    c.column_default, "
        "    pg_get_serial_sequence('public.%1', c.column_name) AS sequence_name "
        "FROM information_schema.columns c "
        "WHERE c.table_schema = 'public' "
        "    AND c.table_name = :table_name "
        "    AND c.column_default LIKE 'nextval%%'"
    ).arg(tableName);
    query.prepare(sql);
    query.bindValue(":table_name", tableName);
    
    if (query.exec()) {
        while (query.next()) {
            SequenceInfo info;
            info.columnName = query.value(0).toString();
            QString sequenceName = query.value(2).toString();
            
            if (sequenceName.isEmpty() || sequenceName.isNull()) {
                continue;
            }
            
            // Получаем текущее значение sequence
            QSqlQuery seqQuery(m_db);
            QString seqQueryStr = QString("SELECT last_value FROM %1").arg(sequenceName);
            if (seqQuery.exec(seqQueryStr)) {
                if (seqQuery.next()) {
                    info.currentValue = seqQuery.value(0).toLongLong();
                    // Сохраняем полное имя sequence
                    info.sequenceName = sequenceName;
                    sequenceInfoList << info;
                }
            }
        }
    } else {
        m_lastError = query.lastError().text();
    }
    
    return sequenceInfoList;
}

QString DatabaseManager::getColumnDefinition(const QString &tableName, const QString &columnName)
{
    QSqlQuery query(m_db);
    query.prepare(
        "SELECT "
        "    data_type, "
        "    character_maximum_length, "
        "    is_nullable, "
        "    column_default "
        "FROM information_schema.columns "
        "WHERE table_schema = 'public' "
        "    AND table_name = :table_name "
        "    AND column_name = :column_name"
    );
    query.bindValue(":table_name", tableName);
    query.bindValue(":column_name", columnName);
    
    if (query.exec() && query.next()) {
        QString dataType = query.value(0).toString();
        QVariant maxLength = query.value(1);
        QString nullable = query.value(2).toString();
        QVariant defaultValue = query.value(3);
        
        QString def = dataType;
        if (maxLength.isValid() && !maxLength.isNull()) {
            def += QString("(%1)").arg(maxLength.toInt());
        }
        if (nullable == "NO") {
            def += " NOT NULL";
        }
        if (defaultValue.isValid() && !defaultValue.isNull()) {
            def += QString(" DEFAULT %1").arg(defaultValue.toString());
        }
        
        return def;
    }
    
    return "";
}

QString DatabaseManager::escapeIdentifier(const QString &identifier)
{
    // Экранируем идентификатор двойными кавычками для PostgreSQL
    // Заменяем двойные кавычки на двойные двойные кавычки
    QString escaped = identifier;
    escaped.replace("\"", "\"\"");
    return QString("\"%1\"").arg(escaped);
}

