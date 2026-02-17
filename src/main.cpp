#include "MainWindow.h"
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

int main(int argc, char *argv[])
{
    // Динамически определяем путь к плагинам в зависимости от версии Qt
    QStringList pluginPaths;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // Пути для Qt 5.5.1 (32-bit)
    pluginPaths << "C:/Qt/Qt5.5.1/5.5/mingw492_32/plugins";
#else
    // Пути для Qt 6.10 (64-bit) - берем из стандартной папки установки
    pluginPaths << "C:/Qt/Qt6.10/6.10.2/mingw_64/plugins";
#endif

    // Путь рядом с EXE (для развертывания)
    QString exePath = QString::fromLocal8Bit(argv[0]);
    QDir exeDir = QFileInfo(exePath).absoluteDir();
    if (exeDir.exists("plugins")) pluginPaths << exeDir.absoluteFilePath("plugins");

    foreach (const QString &path, pluginPaths) {
        if (QDir(path).exists()) {
            QCoreApplication::addLibraryPath(path);
        }
    }

    // Добавляем путь к PostgreSQL в PATH
    QStringList pgPaths;
    pgPaths << "C:/Program Files/PostgreSQL/17/bin"
           << "C:/Program Files/PostgreSQL/16/bin"
           << "C:/Qt/Qt6.10/6.10.2/mingw_64/bin"; // В Qt6 часто нужные dll тут
    
    QString currentPath = QString::fromLocal8Bit(qgetenv("PATH"));
    foreach (const QString &pgPath, pgPaths) {
        if (QDir(pgPath).exists()) {
            currentPath = pgPath + ";" + currentPath;
        }
    }
    qputenv("PATH", currentPath.toLocal8Bit());

    QApplication app(argc, argv);
    
    MainWindow window;
    window.show();
    
    return app.exec();
}
