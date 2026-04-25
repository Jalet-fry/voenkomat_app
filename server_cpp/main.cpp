#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <iostream>
#include "DatabaseManager.h"
#include "RestServer.h"
#include "Converter.h"

int main(int argc, char *argv[])
{
    // 1. Отключаем SSL-спам (для ЛР это лишний шум)
    qputenv("QT_LOGGING_RULES", "qt.network.ssl.warning=false");
    setvbuf(stdout, NULL, _IONBF, 0);

    QCoreApplication a(argc, argv);
    a.setApplicationName("VoenkomatServer");

    // --- УМНЫЙ ПОИСК ПУТЕЙ ---
    QString appDir = QCoreApplication::applicationDirPath();
    QString nosqlPath;
    QDir searchDir(appDir);

    // Ищем папку nosql_db_cpp, поднимаясь вверх от EXE (до 6 уровней)
    for (int i = 0; i < 6; ++i) {
        QString candidate = searchDir.absoluteFilePath("nosql_db_cpp");
        if (QDir(candidate).exists() && !QDir(candidate).entryList({"*.db"}).isEmpty()) {
            nosqlPath = candidate;
            break;
        }
        if (!searchDir.cdUp()) break;
    }

    // --- ПАРСЕР АРГУМЕНТОВ ---
    QCommandLineParser parser;
    parser.addHelpOption();
    QCommandLineOption convertOption("convert", "Конвертировать SQL -> NoSQL");
    parser.addOption(convertOption);
    parser.process(a);

    // --- ЗАПУСК В РЕЖИМЕ КОНВЕРТЕРА ---
    if (parser.isSet(convertOption)) {
        std::cout << ">>> CONVERTER MODE <<<" << std::endl;
        if (!DatabaseManager::instance().connectToDatabase()) {
            std::cerr << "ERROR: No connection to PostgreSQL!" << std::endl;
            return -1;
        }
        Converter conv;
        conv.run();
        return 0;
    }

    // --- ЗАПУСК СЕРВЕРА ---
    if (!nosqlPath.isEmpty()) {
        std::cout << ">>> SERVER MODE: NoSQL (BerkeleyDB Style) <<<" << std::endl;
        std::cout << "[INFO] Database: " << QDir::toNativeSeparators(nosqlPath).toStdString() << std::endl;
        DatabaseManager::instance().setNoSqlMode(true);
        DatabaseManager::instance().setNoSqlPath(nosqlPath);
    } else {
        std::cout << ">>> SERVER MODE: Classic SQL (PostgreSQL) <<<" << std::endl;
        std::cout << "[WARN] NoSQL files not found. Using Postgres." << std::endl;
        DatabaseManager::instance().setNoSqlMode(false);
        DatabaseManager::instance().connectToDatabase();
    }

    RestServer server;
    if (server.start(8000)) {
        std::cout << "Server started on port 8000" << std::endl;
        return a.exec();
    }
    return -1;
}
