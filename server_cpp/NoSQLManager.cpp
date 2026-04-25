#include "NoSQLManager.h"
#include "DatabaseManager.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QDir>
#include <QRegularExpression>

NoSQLManager::NoSQLManager(QObject *parent) : QObject(parent) {}

NoSQLManager& NoSQLManager::instance() {
    static NoSQLManager inst;
    return inst;
}

QStringList NoSQLManager::getAllTables() {
    QString path = DatabaseManager::instance().noSqlPath();
    QDir dir(path);
    QStringList files = dir.entryList({"*.db"}, QDir::Files);
    QStringList tables;
    for (const QString &f : files) tables << f.section('.', 0, 0);
    return tables;
}

QString NoSQLManager::getPrimaryKey(const QString &) {
    // В NoSQL режиме (BerkeleyDB) ключом всегда является 'id' (виртуальное поле для 'key' из KV)
    return "id";
}

QStringList NoSQLManager::getColumns(const QString &tableName) {
    QJsonArray data = getData(tableName, "", 1);
    QStringList cols;
    cols << "id"; // Всегда первый

    if (!data.isEmpty()) {
        QStringList keys = data.first().toObject().keys();
        keys.removeAll("id");
        keys.sort(Qt::CaseInsensitive);
        cols << keys;
    }
    return cols;
}

QJsonArray NoSQLManager::getData(const QString &tableName, const QString &filters, int limit) {
    QJsonArray result;
    QString dbPath = QString("%1/%2.db").arg(DatabaseManager::instance().noSqlPath(), tableName);
    if (!QFile::exists(dbPath)) return result;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "nosql_read_" + tableName);
        db.setDatabaseName(dbPath);
        if (!db.open()) return result;

        QString sql = "SELECT key, value FROM kv";
        QString whereClause = "";

        if (!filters.isEmpty()) {
            QRegularExpression re("(\\w+)\\s*(=|>|<|>=|<=|ILIKE)\\s*(.*)");
            QRegularExpressionMatch match = re.match(filters.trimmed());
            if (match.hasMatch()) {
                QString col = match.captured(1);
                QString op = match.captured(2);
                QString val = match.captured(3).trimmed().remove("'").remove("%");

                if (col == "id") {
                    if (op == "=" || op == "LIKE" || op == "ILIKE") {
                        whereClause = QString(" WHERE key %1 '%2'").arg(op == "ILIKE" ? "LIKE" : op, val);
                    } else {
                        // Умное сравнение для BerkeleyDB ключей: сначала по длине, потом по тексту
                        // Это исключит попадание "10_10" в фильтр "< 5_5"
                        whereClause = QString(" WHERE (length(key), key) %1 (length('%2'), '%2')").arg(op, val);
                    }
                } else {
                    if (op == "ILIKE") {
                        whereClause = QString(" WHERE json_extract(value, '$.%1') LIKE '%%2%'").arg(col, val);
                    } else {
                        bool isNum; val.toDouble(&isNum);
                        if (isNum) whereClause = QString(" WHERE CAST(json_extract(value, '$.%1') AS REAL) %2 %3").arg(col, op, val);
                        else whereClause = QString(" WHERE json_extract(value, '$.%1') %2 '%3'").arg(col, op, val);
                    }
                }
            }
        }

        sql += whereClause;
        // Умная сортировка для BerkeleyDB ключей (сначала по длине, потом по алфавиту)
        sql += " ORDER BY length(key) ASC, key ASC";
        if (limit > 0) sql += QString(" LIMIT %1").arg(limit);

        QSqlQuery query(db);
        if (query.exec(sql)) {
            while (query.next()) {
                QJsonObject obj = QJsonDocument::fromJson(query.value(1).toByteArray()).object();
                obj["id"] = query.value(0).toString(); // Внедряем реальный ключ NoSQL в поле 'id'
                result.append(obj);
            }
        }
        db.close();
    }
    QSqlDatabase::removeDatabase("nosql_read_" + tableName);
    return result;
}

bool NoSQLManager::insertData(const QString &tableName, const QJsonObject &data) {
    QString dbPath = QString("%1/%2.db").arg(DatabaseManager::instance().noSqlPath(), tableName);
    QString id = data.value("id").toString();
    if (id.isEmpty()) {
        // Если ID не задан, генерируем (только для простых таблиц)
        id = QString::number(QDateTime::currentMSecsSinceEpoch());
    }

    QJsonObject cleanData = data;
    cleanData.remove("id");

    bool ok = false;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "nosql_ins_" + tableName);
        db.setDatabaseName(dbPath);
        if (db.open()) {
            QSqlQuery q(db);
            q.prepare("INSERT OR REPLACE INTO kv (key, value) VALUES (?, ?)");
            q.addBindValue(id);
            q.addBindValue(QJsonDocument(cleanData).toJson(QJsonDocument::Compact));
            ok = q.exec();
            db.close();
        }
    }
    QSqlDatabase::removeDatabase("nosql_ins_" + tableName);
    return ok;
}

bool NoSQLManager::updateData(const QString &tableName, const QString &id, const QJsonObject &data) {
    return insertData(tableName, data); // В KV-хранилище это одно и то же
}

bool NoSQLManager::deleteData(const QString &tableName, const QString &id) {
    QString dbPath = QString("%1/%2.db").arg(DatabaseManager::instance().noSqlPath(), tableName);
    bool ok = false;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "nosql_del_" + tableName);
        db.setDatabaseName(dbPath);
        if (db.open()) {
            QSqlQuery q(db);
            q.prepare("DELETE FROM kv WHERE key = ?");
            q.addBindValue(id);
            ok = q.exec();
            db.close();
        }
    }
    QSqlDatabase::removeDatabase("nosql_del_" + tableName);
    return ok;
}
