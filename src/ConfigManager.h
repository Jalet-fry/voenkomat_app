#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QString>
#include <QSettings>

class ConfigManager
{
public:
    explicit ConfigManager(const QString &configFile = "config.ini");
    ~ConfigManager();
    
    // Чтение параметров подключения к БД
    QString getDatabaseHost() const;
    QString getDatabasePort() const;
    QString getDatabaseName() const;
    QString getDatabaseUsername() const;
    QString getDatabasePassword() const;

    // Параметры режима работы
    bool isHttpMode() const;
    void setHttpMode(bool enabled);
    
    // Сохранение параметров подключения к БД
    void setDatabaseHost(const QString &host);
    void setDatabasePort(const QString &port);
    void setDatabaseName(const QString &database);
    void setDatabaseUsername(const QString &username);
    void setDatabasePassword(const QString &password);
    
    // Проверка существования конфигурационного файла
    bool configFileExists() const;
    
    // Создание конфигурационного файла с значениями по умолчанию
    bool createDefaultConfig() const;

private:
    QString m_configFile;
    QSettings* m_settings;
    
    QString findConfigFilePath(const QString &configFile) const;
    QString readValue(const QString &key, const QString &defaultValue = "") const;
    void writeValue(const QString &key, const QVariant &value);
};

#endif // CONFIGMANAGER_H
