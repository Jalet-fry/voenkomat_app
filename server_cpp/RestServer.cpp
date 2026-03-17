#include "RestServer.h"
#include "DatabaseManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QUrlQuery>
#include <QDebug>
#include <QHostAddress>
#include <QTcpServer>

const QString SUPERUSER_PASSWORD = "admin";
const QStringList LOOKUP_TABLES = {"fitness_categories", "commissioners"};

QJsonObject cleanInputData(const QJsonObject &data) {
    QJsonObject cleaned;
    QStringList skipValues = {"", "АВТО", "--- Не выбрано (ПУСТО) ---"};
    for (auto it = data.begin(); it != data.end(); ++it) {
        if (it.value().isNull()) continue;

        // Исправлено: Проверяем skipValues только если это строка.
        // Иначе (числа, булевы значения) - оставляем как есть.
        if (it.value().isString()) {
            QString valStr = it.value().toString();
            if (valStr == "Не выбрано (NULL)") {
                cleaned.insert(it.key(), QJsonValue::Null);
                continue;
            }
            if (skipValues.contains(valStr)) {
                continue;
            }
        }
        cleaned.insert(it.key(), it.value());
    }
    return cleaned;
}

void syncBidirectionalLinks(const QString &tableName, const QVariant &recordId, const QJsonObject &data) {
    if (tableName == "conscripts") {
        QVariant militaryTicketId = data.value("military_ticket_id").toVariant();
        if (!militaryTicketId.isNull() && militaryTicketId.isValid() &&
            militaryTicketId.toString() != "NULL") {
            DatabaseManager::instance().executeModify(
                    "UPDATE public.military_id_cards SET conscript_id = ? WHERE ticket_id = ?",
                    {recordId, militaryTicketId}
            );
        }
        QVariant regCardId = data.value("registration_card_id").toVariant();
        if (!regCardId.isNull() && regCardId.isValid() &&
            regCardId.toString() != "NULL") {
            DatabaseManager::instance().executeModify(
                    "UPDATE public.service_record_cards SET conscript_id = ? WHERE card_id = ?",
                    {recordId, regCardId}
            );
        }
    }
}

RestServer::RestServer(QObject *parent) : QObject(parent) {
    setupRoutes();
}

bool RestServer::start(quint16 port) {
    // Правильный способ запуска в Qt 6.10 TP
    QTcpServer *tcpServer = new QTcpServer(this);
    if (!tcpServer->listen(QHostAddress::Any, port)) {
        qCritical() << "TCP Server could not listen on port" << port;
        return false;
    }

    m_server.bind(tcpServer);
    qInfo() << "Server started on port" << port;
    return true;
}

void RestServer::setupRoutes() {
    // 1. Root
    m_server.route("/", []() {
        return QHttpServerResponse(QJsonObject{
                {"status", "ok"},
                {"message", "Voenkomat C++ API is running"}
        });
    });

    // 2. Custom Query
    m_server.route("/api/execute-query", QHttpServerRequest::Method::Post,
                   [](const QHttpServerRequest &req) {
                       if (req.value("x-auth-token") != SUPERUSER_PASSWORD) {
                           return QHttpServerResponse(QHttpServerResponse::StatusCode::Forbidden);
                       }

                       QJsonObject body = QJsonDocument::fromJson(req.body()).object();
                       QString sql = body.value("sql").toString();
                       QJsonObject result;

                       if (sql.trimmed().toUpper().startsWith("SELECT")) {
                           result = QJsonObject{
                                   {"status", "success"},
                                   {"data", DatabaseManager::instance().executeSelect(sql)}
                           };
                       } else {
                           QJsonObject modifyRes = DatabaseManager::instance().executeModify(sql);
                           result["status"] = modifyRes["status"];
                           // Python возвращает список даже для UPDATE/DELETE (rows_affected в первом элементе)
                           result["data"] = modifyRes["data"];
                       }
                       return QHttpServerResponse(result);
                   }
    );

    // 3. Backup
    m_server.route("/api/backup", QHttpServerRequest::Method::Post,
                   [](const QHttpServerRequest &req) {
                       if (req.value("x-auth-token") != SUPERUSER_PASSWORD) {
                           return QHttpServerResponse(QHttpServerResponse::StatusCode::Forbidden);
                       }

                       QJsonArray tablesRes = DatabaseManager::instance().executeSelect(
                               "SELECT table_name FROM information_schema.tables WHERE table_schema = 'public' AND table_type = 'BASE TABLE'"
                       );

                       QJsonObject backupData;
                       for (const auto &t : tablesRes) {
                           QString name = t.toObject()["table_name"].toString();
                           backupData[name] = DatabaseManager::instance().executeSelect(
                                   QString("SELECT * FROM %1").arg(name)
                           );
                       }

                       QString filename = QString("backup_%1.json")
                               .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));

                       QFile file(filename);
                       if (file.open(QIODevice::WriteOnly)) {
                           file.write(QJsonDocument(backupData).toJson());
                           file.close();
                           return QHttpServerResponse(QJsonObject{
                                   {"status", "success"},
                                   {"file", filename}
                           });
                       }
                       return QHttpServerResponse(QJsonObject{
                               {"status", "error"},
                               {"message", "Could not write file"}
                       });
                   }
    );

    // 4. All tables list
    m_server.route("/api/all-tables", []() {
        QString sql = "SELECT table_name FROM information_schema.tables "
                      "WHERE table_schema = 'public' AND table_type = 'BASE TABLE' "
                      "ORDER BY table_name;";
        QJsonArray res = DatabaseManager::instance().executeSelect(sql);
        QJsonArray tables;
        for (const auto &v : res) tables.append(v.toObject()["table_name"]);
        return QHttpServerResponse(QJsonObject{{"tables", tables}});
    });

    // 5. Columns
    m_server.route("/api/columns/<arg>", [](const QString &tableName) {
        QString sql = "SELECT column_name FROM information_schema.columns "
                      "WHERE table_name = ? AND table_schema = 'public' "
                      "ORDER BY ordinal_position";
        QJsonArray res = DatabaseManager::instance().executeSelect(sql, {tableName});
        QJsonArray cols;
        for (const auto &v : res) cols.append(v.toObject()["column_name"]);
        return QHttpServerResponse(QJsonObject{{"columns", cols}});
    });

    // 6. Column details
    m_server.route("/api/column-details/<arg>", [](const QString &tableName) {
        QString sql = "SELECT column_name as name, data_type as type, "
                      "is_nullable = 'YES' as nullable, column_default as default "
                      "FROM information_schema.columns WHERE table_name = ? "
                      "AND table_schema = 'public' ORDER BY ordinal_position";
        return QHttpServerResponse(QJsonObject{
                {"details", DatabaseManager::instance().executeSelect(sql, {tableName})}
        });
    });

    // 7. Foreign keys
    m_server.route("/api/foreign-keys/<arg>", [](const QString &tableName) {
        QString sql = "SELECT kcu.column_name as column, ccu.table_name AS referenced_table, "
                      "ccu.column_name AS referenced_column "
                      "FROM information_schema.table_constraints AS tc "
                      "JOIN information_schema.key_column_usage AS kcu "
                      "ON tc.constraint_name = kcu.constraint_name "
                      "JOIN information_schema.constraint_column_usage AS ccu "
                      "ON ccu.constraint_name = tc.constraint_name "
                      "WHERE tc.constraint_type = 'FOREIGN KEY' AND tc.table_name = ?";
        return QHttpServerResponse(QJsonObject{
                {"foreign_keys", DatabaseManager::instance().executeSelect(sql, {tableName})}
        });
    });

    // 8. Metadata (PK)
    m_server.route("/api/metadata/<arg>", [](const QString &tableName) {
        return QHttpServerResponse(QJsonObject{
                {"pk", DatabaseManager::instance().getPrimaryKeyColumn(tableName)}
        });
    });

    // 9. Get data with filters
    m_server.route("/api/<arg>", QHttpServerRequest::Method::Get,
                   [](const QString &tableName, const QHttpServerRequest &req) {
                       QUrlQuery query(req.url());
                       QString filters = query.queryItemValue("filters");
                       QString sql = QString("SELECT * FROM public.%1").arg(tableName);
                       if (!filters.isEmpty()) {
                           sql += " WHERE " + filters;
                       }
                       sql += " ORDER BY 1 LIMIT 1000";
                       return QHttpServerResponse(QJsonObject{
                               {"data", DatabaseManager::instance().executeSelect(sql)}
                       });
                   }
    );

    // 10. Create record
    m_server.route("/api/<arg>", QHttpServerRequest::Method::Post,
                   [](const QString &tableName, const QHttpServerRequest &req) {
                       if (LOOKUP_TABLES.contains(tableName) &&
                           req.value("x-auth-token") != SUPERUSER_PASSWORD) {
                           return QHttpServerResponse(QHttpServerResponse::StatusCode::Forbidden);
                       }

                       QJsonDocument doc = QJsonDocument::fromJson(req.body());
                       if (doc.isNull() || !doc.isObject()) {
                           return QHttpServerResponse(QHttpServerResponse::StatusCode::BadRequest);
                       }

                       QJsonObject data = cleanInputData(doc.object());
                       QStringList cols;
                       QVariantList vals;
                       QStringList placeholders;

                       for (auto it = data.begin(); it != data.end(); ++it) {
                           cols << it.key();
                           vals << it.value().toVariant();
                           placeholders << "?";
                       }

                       QString sql = QString("INSERT INTO public.%1 (%2) VALUES (%3) RETURNING *")
                               .arg(tableName, cols.join(", "), placeholders.join(", "));

                       QJsonObject res = DatabaseManager::instance().executeModify(sql, vals);

                       if (res["status"] == "success" && res.contains("data")) {
                           QJsonArray dataArray = res["data"].toArray();
                           if (!dataArray.isEmpty()) {
                               QJsonObject newRec = dataArray.first().toObject();
                               QString pk = DatabaseManager::instance().getPrimaryKeyColumn(tableName);
                               syncBidirectionalLinks(tableName, newRec[pk].toVariant(), data);

                               // Исправлено: возвращаем объект в поле data, а не массив (как в Python)
                               return QHttpServerResponse(QJsonObject{
                                   {"status", "success"},
                                   {"data", newRec}
                               });
                           }
                       }
                       return QHttpServerResponse(res);
                   }
    );

    // 11. Update record
    m_server.route("/api/<arg>/<arg>", QHttpServerRequest::Method::Put,
                   [](const QString &tableName, const QString &id, const QHttpServerRequest &req) {
                       if (LOOKUP_TABLES.contains(tableName) &&
                           req.value("x-auth-token") != SUPERUSER_PASSWORD) {
                           return QHttpServerResponse(QHttpServerResponse::StatusCode::Forbidden);
                       }

                       QJsonDocument doc = QJsonDocument::fromJson(req.body());
                       if (doc.isNull() || !doc.isObject()) {
                           return QHttpServerResponse(QHttpServerResponse::StatusCode::BadRequest);
                       }

                       QJsonObject data = cleanInputData(doc.object());
                       QString pk = DatabaseManager::instance().getPrimaryKeyColumn(tableName);

                       QStringList sets;
                       QVariantList vals;
                       for (auto it = data.begin(); it != data.end(); ++it) {
                           if (it.key() == pk) continue;
                           sets << QString("%1 = ?").arg(it.key());
                           vals << it.value().toVariant();
                       }
                       vals << id;

                       QString sql = QString("UPDATE public.%1 SET %2 WHERE %3 = ? RETURNING *")
                               .arg(tableName, sets.join(", "), pk);

                       QJsonObject res = DatabaseManager::instance().executeModify(sql, vals);
                       if (res["status"] == "success") {
                           syncBidirectionalLinks(tableName, id, data);

                           QJsonArray dataArray = res["data"].toArray();
                           if (!dataArray.isEmpty()) {
                               return QHttpServerResponse(QJsonObject{
                                   {"status", "success"},
                                   {"data", dataArray.first().toObject()}
                               });
                           }
                       }
                       return QHttpServerResponse(res);
                   }
    );

    // 12. Delete record
    m_server.route("/api/<arg>/<arg>", QHttpServerRequest::Method::Delete,
                   [](const QString &tableName, const QString &id, const QHttpServerRequest &req) {
                       if (LOOKUP_TABLES.contains(tableName) &&
                           req.value("x-auth-token") != SUPERUSER_PASSWORD) {
                           return QHttpServerResponse(QHttpServerResponse::StatusCode::Forbidden);
                       }

                       QString pk = DatabaseManager::instance().getPrimaryKeyColumn(tableName);
                       QString sql = QString("DELETE FROM public.%1 WHERE %2 = ?")
                               .arg(tableName, pk);

                       QJsonObject res = DatabaseManager::instance().executeModify(sql, {id});
                       if (res["status"] == "success") {
                           // Python возвращает просто статус
                           return QHttpServerResponse(QJsonObject{{"status", "success"}});
                       }
                       return QHttpServerResponse(res);
                   }
    );
}
