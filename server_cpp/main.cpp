#include <QCoreApplication>
#include "DatabaseManager.h"
#include "RestServer.h"
#include <QDebug>
#include <QDir>
#include <QSqlDatabase>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    a.setApplicationName("VoenkomatServer");

    // Получаем путь к папке с exe
    QString appDir = QCoreApplication::applicationDirPath();

    // Поднимаемся на 4 уровня вверх к корню проекта (voenkomat_app)
    QString projectRoot = QDir(appDir).absoluteFilePath("../../../..");
    projectRoot = QDir::cleanPath(projectRoot);

    // Путь к папке dll в корне проекта
    QString dllPath = projectRoot + "/dll";

    qDebug() << "App dir:" << appDir;
    qDebug() << "Project root:" << projectRoot;
    qDebug() << "DLL path:" << dllPath;

    // Проверяем существование папки dll
    if (!QDir(dllPath).exists()) {
        qWarning() << "DLL folder does not exist:" << dllPath;
        qWarning() << "Please create folder and copy PostgreSQL DLLs";
    } else {
        qDebug() << "DLL folder found, adding to PATH";

        // Добавляем путь к DLL в PATH
        QString currentPath = QString::fromLocal8Bit(qgetenv("PATH"));
        currentPath = dllPath + ";" + currentPath;
        qputenv("PATH", currentPath.toLocal8Bit());
    }

    qInfo() << "Starting Voenkomat C++ Server...";
    qDebug() << "Available SQL drivers:" << QSqlDatabase::drivers();

    if (!DatabaseManager::instance().connectToDatabase()) {
        qCritical() << "Failed to connect to database. Exiting...";
        return -1;
    }
    qInfo() << "Database connected successfully.";

    RestServer server;
    if (!server.start(8000)) {
        return -1;
    }

    return a.exec();
}