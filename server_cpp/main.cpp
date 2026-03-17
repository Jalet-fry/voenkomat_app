#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QSqlDatabase>
#include <iostream>
#include "DatabaseManager.h"
#include "RestServer.h"
#include "Converter.h"
#include "DataGenerator.h"

int main(int argc, char *argv[])
{
    // Отключаем буферизацию для мгновенного вывода в консоль
    setvbuf(stdout, NULL, _IONBF, 0);

    QCoreApplication a(argc, argv);
    a.setApplicationName("VoenkomatServer");

    // --- БЛОК ПОИСКА ПУТЕЙ DLL ---
    QString appDir = QCoreApplication::applicationDirPath();
    QString projectRoot = QDir(appDir).absoluteFilePath("../../../..");
    projectRoot = QDir::cleanPath(projectRoot);
    QString dllPath = projectRoot + "/dll";

    if (QDir(dllPath).exists()) {
        QString currentPath = QString::fromLocal8Bit(qgetenv("PATH"));
        currentPath = dllPath + ";" + currentPath;
        qputenv("PATH", currentPath.toLocal8Bit());
    }

    // --- ПАРСЕР КОМАНДНОЙ СТРОКИ ---
    QCommandLineParser parser;
    parser.setApplicationDescription("ИС Военкомат: Сервер + NoSQL Конвертер + Генератор");
    parser.addHelpOption();

    QCommandLineOption convertOption("convert", "Запустить NoSQL конвертер (ЛР 3) и выйти.");
    QCommandLineOption generateOption("generate", "Запустить генератор тестовых данных и выйти.");

    parser.addOption(convertOption);
    parser.addOption(generateOption);
    parser.process(a);

    // --- ПОДКЛЮЧЕНИЕ К БАЗЕ ДАННЫХ ---
    if (!DatabaseManager::instance().connectToDatabase()) {
        std::cout << "CRITICAL: Database connection failed! Check config.ini\n";
        return -1;
    }

    // 1. Режим конвертера (аналог converter.py)
    if (parser.isSet(convertOption)) {
        std::cout << "\n>>> STARTING NOSQL CONVERSION (LAB 3) <<<\n";
        Converter conv;
        conv.run();
        std::cout << ">>> CONVERSION FINISHED <<<\n";
        return 0;
    }

    // 2. Режим генератора (аналог generate_data.py)
    if (parser.isSet(generateOption)) {
        std::cout << "\n>>> STARTING DATA GENERATION <<<\n";
        DataGenerator gen;
        gen.run();
        std::cout << ">>> GENERATION FINISHED <<<\n";
        return 0;
    }

    // 3. Режим HTTP сервера (ЛР 1)
    std::cout << "\n>>> STARTING VOENKOMAT REST SERVER (PORT 8000) <<<\n";
    RestServer server;
    if (!server.start(8000)) {
        std::cout << "ERROR: Failed to start server\n";
        return -1;
    }

    return a.exec();
}
