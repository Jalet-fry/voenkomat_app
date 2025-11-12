#include "ConfigManager.h"
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QDebug>
#include <QProcessEnvironment>
#include <QApplication>

QString ConfigManager::findConfigFilePath(const QString &configFile) const
{
    // Если указан абсолютный путь, используем его
    if (QDir::isAbsolutePath(configFile)) {
        return configFile;
    }
    
    QStringList possiblePaths;
    
    // 1. В директории приложения (предпочтительно для релизных сборок)
    QString appDir = QApplication::applicationDirPath();
    possiblePaths << QDir(appDir).absoluteFilePath(configFile);
    
    // 2. В текущей рабочей директории (для разработки)
    QDir currentDir = QDir::current();
    possiblePaths << currentDir.absoluteFilePath(configFile);
    
    // 3. В директории проекта (на уровень выше от debug/release)
    QDir appDirParent = QDir(appDir);
    if (appDirParent.cdUp()) {
        possiblePaths << appDirParent.absoluteFilePath(configFile);
    }
    
    // 4. Проверяем родительские директории от текущей рабочей директории
    QDir parentDir = currentDir;
    for (int i = 0; i < 5; ++i) {
        QString parentPath = parentDir.absoluteFilePath(configFile);
        possiblePaths << parentPath;
        if (!parentDir.cdUp()) {
            break;
        }
    }
    
    // Ищем существующий файл
    foreach (const QString &path, possiblePaths) {
        if (QFile::exists(path)) {
            qDebug() << "Найден config.ini:" << path;
            return path;
        }
    }
    
    // Если файл не найден, возвращаем путь в директории приложения (для создания)
    QString configPath = QDir(appDir).absoluteFilePath(configFile);
    qDebug() << "config.ini не найден, будет создан:" << configPath;
    return configPath;
}

ConfigManager::ConfigManager(const QString &configFile)
{
    // Находим правильный путь к config.ini
    m_configFile = findConfigFilePath(configFile);
    
    // QSettings для INI файлов требует указания формата
    m_settings = new QSettings(m_configFile, QSettings::IniFormat);
    m_settings->setIniCodec("UTF-8");
}

ConfigManager::~ConfigManager()
{
    delete m_settings;
}

QString ConfigManager::getDatabaseHost() const
{
    return readValue("Database/host", "localhost");
}

QString ConfigManager::getDatabasePort() const
{
    return readValue("Database/port", "5432");
}

QString ConfigManager::getDatabaseName() const
{
    return readValue("Database/database", "voenkomat");
}

QString ConfigManager::getDatabaseUsername() const
{
    return readValue("Database/username", "postgres");
}

QString ConfigManager::getDatabasePassword() const
{
    // Пароль берется из системной переменной окружения PGPASSWORD
    // Это более безопасно, чем хранить пароль в файле
    QString envPassword = QProcessEnvironment::systemEnvironment().value("PGPASSWORD");
    if (!envPassword.isEmpty()) {
        qDebug() << "Пароль найден в переменной окружения PGPASSWORD";
        return envPassword;
    }
    
    // Если переменная окружения не установлена, можно попробовать прочитать из конфига
    // (но это не рекомендуется для безопасности)
    QString configPassword = readValue("Database/password", "");
    if (!configPassword.isEmpty()) {
        qDebug() << "Пароль найден в config.ini (не рекомендуется для безопасности)";
        return configPassword;
    }
    
    qDebug() << "ВНИМАНИЕ: Пароль не найден! Установите переменную окружения PGPASSWORD";
    return "";
}

void ConfigManager::setDatabaseHost(const QString &host)
{
    writeValue("Database/host", host);
}

void ConfigManager::setDatabasePort(const QString &port)
{
    writeValue("Database/port", port);
}

void ConfigManager::setDatabaseName(const QString &database)
{
    writeValue("Database/database", database);
}

void ConfigManager::setDatabaseUsername(const QString &username)
{
    writeValue("Database/username", username);
}

void ConfigManager::setDatabasePassword(const QString &password)
{
    writeValue("Database/password", password);
}

bool ConfigManager::configFileExists() const
{
    return QFile::exists(m_configFile);
}

bool ConfigManager::createDefaultConfig() const
{
    // Убеждаемся, что директория существует
    QFileInfo fileInfo(m_configFile);
    QDir configDir = fileInfo.absoluteDir();
    if (!configDir.exists()) {
        if (!configDir.mkpath(".")) {
            qDebug() << "Не удалось создать директорию для config.ini:" << configDir.absolutePath();
            return false;
        }
    }
    
    QFile file(m_configFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Не удалось создать config.ini:" << m_configFile;
        qDebug() << "Ошибка:" << file.errorString();
        return false;
    }
    
    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << "# Конфигурационный файл для приложения \"Военкомат\"\n";
    out << "# Пароль берется из системной переменной окружения PGPASSWORD\n\n";
    out << "[Database]\n";
    out << "host=localhost\n";
    out << "port=5432\n";
    out << "database=voenkomat\n";
    out << "username=postgres\n";
    out << "# Пароль не хранится здесь - используется переменная окружения PGPASSWORD\n";
    
    file.close();
    qDebug() << "Создан config.ini:" << m_configFile;
    return true;
}

QString ConfigManager::readValue(const QString &key, const QString &defaultValue) const
{
    return m_settings->value(key, defaultValue).toString();
}

void ConfigManager::writeValue(const QString &key, const QString &value)
{
    m_settings->setValue(key, value);
    m_settings->sync();
}

