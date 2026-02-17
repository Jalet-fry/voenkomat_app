#include "ConfigManager.h"
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QDebug>
#include <QProcessEnvironment>
#include <QApplication>

QString ConfigManager::findConfigFilePath(const QString &configFile) const
{
    if (QDir::isAbsolutePath(configFile)) return configFile;
    
    QStringList possiblePaths;
    possiblePaths << QDir(QApplication::applicationDirPath()).absoluteFilePath(configFile);
    possiblePaths << QDir::current().absoluteFilePath(configFile);
    
    QDir appDirParent(QApplication::applicationDirPath());
    if (appDirParent.cdUp()) possiblePaths << appDirParent.absoluteFilePath(configFile);
    
    foreach (const QString &path, possiblePaths) {
        if (QFile::exists(path)) return path;
    }
    return QDir(QApplication::applicationDirPath()).absoluteFilePath(configFile);
}

ConfigManager::ConfigManager(const QString &configFile)
{
    m_configFile = findConfigFilePath(configFile);
    m_settings = new QSettings(m_configFile, QSettings::IniFormat);
    // In Qt 6, QSettings uses UTF-8 by default for INI files. setIniCodec is removed.
}

ConfigManager::~ConfigManager() { delete m_settings; }

QString ConfigManager::getDatabaseHost() const { return readValue("Database/host", "localhost"); }
QString ConfigManager::getDatabasePort() const { return readValue("Database/port", "5432"); }
QString ConfigManager::getDatabaseName() const { return readValue("Database/database", "military_db"); }
QString ConfigManager::getDatabaseUsername() const { return readValue("Database/username", "postgres"); }

QString ConfigManager::getDatabasePassword() const
{
    QString envPassword = QProcessEnvironment::systemEnvironment().value("PGPASSWORD");
    if (!envPassword.isEmpty()) return envPassword;
    return readValue("Database/password", "");
}

bool ConfigManager::isHttpMode() const
{
    return m_settings->value("Mode/use_http", false).toBool();
}

void ConfigManager::setHttpMode(bool enabled)
{
    writeValue("Mode/use_http", enabled);
}

void ConfigManager::setDatabaseHost(const QString &host) { writeValue("Database/host", host); }
void ConfigManager::setDatabasePort(const QString &port) { writeValue("Database/port", port); }
void ConfigManager::setDatabaseName(const QString &database) { writeValue("Database/database", database); }
void ConfigManager::setDatabaseUsername(const QString &username) { writeValue("Database/username", username); }
void ConfigManager::setDatabasePassword(const QString &password) { writeValue("Database/password", password); }

bool ConfigManager::configFileExists() const { return QFile::exists(m_configFile); }

bool ConfigManager::createDefaultConfig() const
{
    QFile file(m_configFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QTextStream out(&file);
    // In Qt 6, QTextStream uses UTF-8 by default. setCodec is removed.
    out << "[Database]\n";
    out << "host=localhost\n";
    out << "port=5432\n";
    out << "database=military_db\n";
    out << "username=postgres\n\n";
    out << "[Mode]\n";
    out << "use_http=false\n";
    file.close();
    return true;
}

QString ConfigManager::readValue(const QString &key, const QString &defaultValue) const { return m_settings->value(key, defaultValue).toString(); }
void ConfigManager::writeValue(const QString &key, const QVariant &value) { m_settings->setValue(key, value); m_settings->sync(); }
