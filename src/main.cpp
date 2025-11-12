#include "MainWindow.h"
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

int main(int argc, char *argv[])
{
    // Устанавливаем путь к плагинам Qt ДО создания QApplication
    // Это критически важно для загрузки SQL драйверов
    
    QStringList pluginPaths;
    
    // 1. Стандартный путь Qt (для разработки)
    QString qtPluginsPath = "C:/Qt/Qt5.5.1/5.5/mingw492_32/plugins";
    if (QDir(qtPluginsPath).exists()) {
        pluginPaths << qtPluginsPath;
    }
    
    // 2. Путь относительно исполняемого файла (для релизных сборок)
    // Используем предполагаемый путь, так как applicationDirPath() еще недоступен
    QString exePath = QString::fromLocal8Bit(argv[0]);
    QFileInfo exeInfo(exePath);
    QDir exeDir = exeInfo.absoluteDir();
    
    // Проверяем plugins рядом с exe
    QString localPlugins = exeDir.absoluteFilePath("plugins");
    if (QDir(localPlugins).exists()) {
        pluginPaths << localPlugins;
    }
    
    // 3. На уровень выше от debug/release
    if (exeDir.cdUp()) {
        QString parentPlugins = exeDir.absoluteFilePath("plugins");
        if (QDir(parentPlugins).exists()) {
            pluginPaths << parentPlugins;
        }
    }
    
    // Устанавливаем переменную окружения QT_PLUGIN_PATH
    if (!pluginPaths.isEmpty()) {
        QString pluginPathEnv = pluginPaths.join(";");
        qputenv("QT_PLUGIN_PATH", pluginPathEnv.toLocal8Bit());
        qDebug() << "Установлена переменная QT_PLUGIN_PATH:" << pluginPathEnv;
    }
    
    // Также добавляем пути через QCoreApplication (до создания QApplication)
    foreach (const QString &path, pluginPaths) {
        if (QDir(path).exists()) {
            QCoreApplication::addLibraryPath(path);
            qDebug() << "Добавлен путь к плагинам Qt:" << path;
        }
    }
    
    // Добавляем путь к PostgreSQL в PATH для поиска зависимостей (libpq.dll)
    QStringList pgPaths;
    pgPaths << "C:/Program Files/PostgreSQL/17/bin"
           << "C:/Program Files/PostgreSQL/16/bin"
           << "C:/Program Files/PostgreSQL/15/bin"
           << "C:/Program Files/PostgreSQL/14/bin";
    
    QString currentPath = QString::fromLocal8Bit(qgetenv("PATH"));
    foreach (const QString &pgPath, pgPaths) {
        if (QDir(pgPath).exists() && QFileInfo::exists(QDir(pgPath).absoluteFilePath("libpq.dll"))) {
            if (!currentPath.contains(pgPath)) {
                QString newPath = currentPath + ";" + pgPath;
                qputenv("PATH", newPath.toLocal8Bit());
                qDebug() << "Добавлен путь PostgreSQL в PATH:" << pgPath;
            }
            break;
        }
    }
    
    // Теперь создаем QApplication - плагины должны быть доступны
    QApplication app(argc, argv);
    
    MainWindow window;
    window.show();
    
    return app.exec();
}

