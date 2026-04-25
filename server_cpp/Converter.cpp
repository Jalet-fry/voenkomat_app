#include "Converter.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlRecord>
#include <QFile>
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>
#include <QDebug>
#include <QCoreApplication>

Converter::Converter(QObject *parent) : QObject(parent) {}

void Converter::run()
{
    qInfo() << "--- ЛАБОРАТОРНАЯ РАБОТА №4: NoSQL КОНВЕРТЕР (C++ / SQLite-KV) ---";

    QSqlDatabase pgDb = QSqlDatabase::database(QSqlDatabase::connectionNames().first());
    if (!pgDb.isOpen()) {
        qCritical() << "Postgres database not open!";
        return;
    }

    // Определяем папку для NoSQL (C++)
    QString projectRoot = QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("../../../..");
    QString nosqlDir = QDir(projectRoot).absoluteFilePath("nosql_db_cpp");
    QDir().mkpath(nosqlDir);
    qInfo() << "Сохранение NoSQL в:" << nosqlDir;

    QSqlQuery tableQuery(pgDb);
    tableQuery.prepare("SELECT table_name FROM information_schema.tables "
                       "WHERE table_schema = 'public' AND table_type = 'BASE TABLE'");

    if (!tableQuery.exec()) return;

    while (tableQuery.next()) {
        QString tableName = tableQuery.value(0).toString();
        qInfo() << "Конвертация" << tableName << "...";

        QStringList pkCols = getPrimaryKeyColumns(tableName, pgDb);

        // 1. Создаем SQLite файл для этой таблицы
        QString dbPath = QDir(nosqlDir).filePath(tableName + ".db");
        if (QFile::exists(dbPath)) QFile::remove(dbPath);

        QSqlDatabase liteDb = QSqlDatabase::addDatabase("QSQLITE", "lite_" + tableName);
        liteDb.setDatabaseName(dbPath);
        if (!liteDb.open()) {
            qWarning() << "Could not create SQLite file:" << dbPath;
            continue;
        }

        liteDb.exec("CREATE TABLE kv (key TEXT PRIMARY KEY, value TEXT)");

        // 2. Читаем данные из Postgres
        QSqlQuery dataQuery(pgDb);
        dataQuery.prepare(QString("SELECT * FROM public.%1").arg(tableName));
        if (!dataQuery.exec()) continue;

        QSqlRecord rec = dataQuery.record();
        liteDb.transaction();

        QSqlQuery insertQuery(liteDb);
        insertQuery.prepare("INSERT INTO kv (key, value) VALUES (?, ?)");

        int count = 0;
        int rowIdx = 1;
        while (dataQuery.next()) {
            QJsonObject valObj;

            // ФОРМИРУЕМ СОСТАВНОЙ КЛЮЧ (для ЛР4)
            QString keyStr;
            if (!pkCols.isEmpty()) {
                QStringList keyParts;
                for (const QString &pk : pkCols) {
                    keyParts << dataQuery.value(pk).toString();
                }
                keyStr = keyParts.join("_");
            } else {
                // Если нет PK, используем индекс строки
                keyStr = QString::number(rowIdx++);
            }

            // Собираем все поля в JSON
            for (int i = 0; i < rec.count(); ++i) {
                valObj[rec.fieldName(i)] = QJsonValue::fromVariant(dataQuery.value(i));
            }

            insertQuery.addBindValue(keyStr);
            insertQuery.addBindValue(QString(QJsonDocument(valObj).toJson(QJsonDocument::Compact)));
            insertQuery.exec();
            count++;
        }

        liteDb.commit();
        liteDb.close();
        QSqlDatabase::removeDatabase("lite_" + tableName);

        qInfo() << "  Успешно:" << tableName << "(" << count << "записей )";
    }
    qInfo() << "--- КОНВЕРТАЦИЯ ЗАВЕРШЕНА ---";
}

QStringList Converter::getPrimaryKeyColumns(const QString &tableName, QSqlDatabase &db)
{
    QStringList cols;
    QSqlQuery query(db);
    query.prepare("SELECT kcu.column_name FROM information_schema.table_constraints tc "
                  "JOIN information_schema.key_column_usage kcu ON tc.constraint_name = kcu.constraint_name "
                  "WHERE tc.constraint_type = 'PRIMARY KEY' AND tc.table_name = ? ORDER BY kcu.ordinal_position");
    query.addBindValue(tableName);
    if (query.exec()) {
        while (query.next()) cols << query.value(0).toString();
    }
    return cols;
}
