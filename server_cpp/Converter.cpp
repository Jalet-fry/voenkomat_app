#include "Converter.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlRecord>
#include <QFile>
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>
#include <QDebug>

Converter::Converter(QObject *parent) : QObject(parent) {}

void Converter::run()
{
    qInfo() << "--- ЛАБОРАТОРНАЯ РАБОТА №3: NoSQL КОНВЕРТЕР (C++) ---";

    QSqlDatabase db = QSqlDatabase::database(QSqlDatabase::connectionNames().first());
    if (!db.isOpen()) {
        qCritical() << "Database not open!";
        return;
    }

    QString nosqlDir = "nosql_db_cpp";
    QDir().mkpath(nosqlDir);

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

        QFile dbFile(QDir(nosqlDir).filePath(tableName + ".db.txt"));
        if (dbFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&dbFile);

            while (dataQuery.next()) {
                QJsonObject valObj;
                QString keyStr;

                // Формируем ключ (поддержка составных ключей как в Python)
                if (pkCols.size() > 1) {
                    QStringList keyParts;
                    for (const auto &pk : pkCols) keyParts << dataQuery.value(pk).toString();
                    keyStr = keyParts.join("_");
                } else if (pkCols.size() == 1) {
                    keyStr = dataQuery.value(pkCols[0]).toString();
                }

                // Формируем значение (JSON всех остальных полей)
                for (int i = 0; i < rec.count(); ++i) {
                    QString colName = rec.fieldName(i);
                    // Если PK один, исключаем его из JSON (как в ТЗ)
                    if (pkCols.size() == 1 && colName == pkCols[0]) continue;
                    valObj[colName] = QJsonValue::fromVariant(dataQuery.value(i));
                }

                if (keyStr.isEmpty()) keyStr = "row_" + QString::number(dataQuery.at());
                out << keyStr << " ||| " << QJsonDocument(valObj).toJson(QJsonDocument::Compact) << "\n";
            }
            dbFile.close();
            qInfo() << "  OK:" << tableName << "конвертирована.";
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
