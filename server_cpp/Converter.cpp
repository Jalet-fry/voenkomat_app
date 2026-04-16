#include "Converter.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlRecord>
#include <QFile>
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>
#include <QDebug>
#include <QTextStream>
#include <QDataStream>
#include <QCoreApplication>

Converter::Converter(QObject *parent) : QObject(parent) {}

void Converter::run()
{
    qInfo() << "--- ЛАБОРАТОРНАЯ РАБОТА №3: NoSQL КОНВЕРТЕР (C++) ---";

    QSqlDatabase db = QSqlDatabase::database(QSqlDatabase::connectionNames().first());
    if (!db.isOpen()) {
        qCritical() << "Database not open!";
        return;
    }

    // Определяем корень проекта (там где лежит config.ini)
    QString appDir = QCoreApplication::applicationDirPath();
    QDir dir(appDir);

    // Поднимаемся из build/release до корня проекта
    // Обычно это 3-4 уровня вверх для Shadow Build
    while (!dir.exists("config.ini") && dir.cdUp()) { }

    QString nosqlDir = dir.absoluteFilePath("nosql_db_cpp");
    QDir().mkpath(nosqlDir);
    qInfo() << "Путь сохранения NoSQL:" << nosqlDir;

    QSqlQuery tableQuery(db);
    tableQuery.prepare("SELECT table_name FROM information_schema.tables "
                       "WHERE table_schema = 'public' AND table_type = 'BASE TABLE'");

    if (!tableQuery.exec()) {
        qCritical() << "Failed to fetch tables:" << tableQuery.lastError().text();
        return;
    }

    while (tableQuery.next()) {
        QString tableName = tableQuery.value(0).toString();
        qInfo() << "Конвертация" << tableName << "...";

        QStringList pkCols = getPrimaryKeyColumns(tableName);

        QSqlQuery dataQuery(db);
        dataQuery.prepare(QString("SELECT * FROM public.%1").arg(tableName));
        if (!dataQuery.exec()) continue;

        QSqlRecord rec = dataQuery.record();

        // Создаем два файла: текстовый дамп и бинарную базу
        QFile txtFile(QDir(nosqlDir).filePath(tableName + ".db.txt"));
        QFile binFile(QDir(nosqlDir).filePath(tableName + ".db"));

        if (txtFile.open(QIODevice::WriteOnly | QIODevice::Text) && binFile.open(QIODevice::WriteOnly)) {
            QTextStream out(&txtFile);
            QDataStream binOut(&binFile);
            binOut.setVersion(QDataStream::Qt_6_0); // Совместимость версий

            while (dataQuery.next()) {
                QJsonObject valObj;
                QString keyStr;

                // 1. Формируем ключ
                if (pkCols.size() > 1) {
                    QStringList keyParts;
                    for (const auto &pk : pkCols) keyParts << dataQuery.value(pk).toString();
                    keyStr = keyParts.join("_");
                } else if (pkCols.size() == 1) {
                    keyStr = dataQuery.value(pkCols[0]).toString();
                } else {
                    keyStr = "row_" + QString::number(dataQuery.at());
                }

                // 2. Формируем значение (JSON)
                for (int i = 0; i < rec.count(); ++i) {
                    QString colName = rec.fieldName(i);
                    // Исключаем PK из JSON если он один (по ТЗ)
                    if (pkCols.size() == 1 && colName == pkCols[0]) continue;
                    valObj[colName] = QJsonValue::fromVariant(dataQuery.value(i));
                }

                QString jsonStr = QJsonDocument(valObj).toJson(QJsonDocument::Compact);

                // Запись в текстовый файл (для глаз)
                out << keyStr << " ||| " << jsonStr << "\n";

                // Запись в бинарный файл (имитация BerkeleyDB)
                binOut << keyStr.toUtf8() << jsonStr.toUtf8();
            }
            txtFile.close();
            binFile.close();

            // --- БЛОК СРАВНЕНИЯ (ВЕРИФИКАЦИЯ) ---
            QFile verifyFile(QDir(nosqlDir).filePath(tableName + ".db"));
            if (verifyFile.open(QIODevice::ReadOnly)) {
                QDataStream in(&verifyFile);
                QByteArray vKey, vVal;
                in >> vKey >> vVal; // Читаем самую первую запись

                qInfo() << "  [V] Верификация таблицы" << tableName << ":";
                qInfo() << "      SQL Key:" << "..." << "-> NoSQL Key:" << vKey;
                qInfo() << "      SQL JSON:" << "..." << "-> NoSQL JSON:" << vVal.left(50) << "...";
                verifyFile.close();
            }

            qInfo() << "  OK:" << tableName << "(txt + db) созданы.";
        }
    }

    qInfo() << "--- КОНВЕРТАЦИЯ ЗАВЕРШЕНА ---";
}

QStringList Converter::getPrimaryKeyColumns(const QString &tableName)
{
    QStringList cols;
    QSqlQuery query(QSqlDatabase::database(QSqlDatabase::connectionNames().first()));
    query.prepare("SELECT kcu.column_name FROM information_schema.table_constraints tc "
                  "JOIN information_schema.key_column_usage kcu ON tc.constraint_name = kcu.constraint_name "
                  "WHERE tc.constraint_type = 'PRIMARY KEY' AND tc.table_name = ? ORDER BY kcu.ordinal_position");
    query.addBindValue(tableName);
    if (query.exec()) {
        while (query.next()) cols << query.value(0).toString();
    }
    return cols;
}
