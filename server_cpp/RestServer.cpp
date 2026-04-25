#include "RestServer.h"
#include "NoSQLManager.h"
#include "DatabaseManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QDebug>
#include <QHostAddress>
#include <QTcpServer>
#include <QHttpHeaders>

QHttpHeaders commonHeaders() {
    QHttpHeaders h;
    h.append("Access-Control-Allow-Origin", "*");
    h.append("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    h.append("Access-Control-Allow-Headers", "Content-Type, x-auth-token");
    h.append("Server", "Voenkomat-NoSQL-Server");
    return h;
}

QHttpServerResponse makeResponse(const QJsonObject &obj) {
    QHttpServerResponse resp(obj);
    resp.setHeaders(commonHeaders());
    return resp;
}

QHttpServerResponse makeResponse(const QJsonArray &arr) {
    QHttpServerResponse resp(arr);
    resp.setHeaders(commonHeaders());
    return resp;
}

QHttpServerResponse makeResponse(const QByteArray &data) {
    QHttpServerResponse resp(data);
    resp.setHeaders(commonHeaders());
    return resp;
}

RestServer::RestServer(QObject *parent) : QObject(parent) {
    setupRoutes();
}

bool RestServer::start(quint16 port) {
    QTcpServer *tcpServer = new QTcpServer(this);
    if (!tcpServer->listen(QHostAddress::Any, port)) {
        qCritical() << "!!! FAILED TO LISTEN ON PORT" << port;
        return false;
    }
    m_server.bind(tcpServer);
    qInfo() << ">>> REST SERVER IS READY ON PORT" << port << "<<<";
    return true;
}

void RestServer::setupRoutes() {

    // 1. Root
    m_server.route("/", [this](const QHttpServerRequest &req) {
        qInfo() << "[HTTP] Ping from" << req.remoteAddress().toString();
        return makeResponse(QByteArray("OK"));
    });

    // 2. All tables
    m_server.route("/api/all-tables", [this]() {
        qInfo() << "[HTTP] Table list requested";
        QStringList tables;
        if (DatabaseManager::instance().isNoSqlMode()) {
            tables = NoSQLManager::instance().getAllTables();
        } else {
            tables = {"conscripts", "medical_examinations", "fitness_categories",
                      "military_id_cards", "service_record_cards", "commissioners", "callup_events"};
        }
        QJsonObject res;
        res["tables"] = QJsonArray::fromStringList(tables);
        return makeResponse(res);
    });

    // 3. Metadata (Primary Key)
    m_server.route("/api/metadata/<arg>", [this](const QString &tableName) {
        QJsonObject res;
        if (DatabaseManager::instance().isNoSqlMode()) {
            res["pk"] = NoSQLManager::instance().getPrimaryKey(tableName);
        } else {
            res["pk"] = DatabaseManager::instance().getPrimaryKeyColumn(tableName);
        }
        return makeResponse(res);
    });

    // 4. Columns (Simple list)
    m_server.route("/api/columns/<arg>", [this](const QString &tableName) {
        qInfo() << "[HTTP] Column list for:" << tableName;
        QStringList columns;
        if (DatabaseManager::instance().isNoSqlMode()) {
            columns = NoSQLManager::instance().getColumns(tableName);
        } else {
            columns = NoSQLManager::instance().getColumns(tableName);
        }
        QJsonObject res;
        res["columns"] = QJsonArray::fromStringList(columns);
        return makeResponse(res);
    });

    // 5. Column details (Expected by client)
    m_server.route("/api/column-details/<arg>", [this](const QString &tableName) {
        qInfo() << "[HTTP] Column details for:" << tableName;
        QJsonArray details;
        QStringList columns;

        if (DatabaseManager::instance().isNoSqlMode()) {
            columns = NoSQLManager::instance().getColumns(tableName);
        } else {
            // В режиме SQL можно вытащить реальные типы, но для NoSQL упростим
            columns = NoSQLManager::instance().getColumns(tableName);
        }

        for (const QString &col : columns) {
            QJsonObject d;
            d["name"] = col;
            d["type"] = "text"; // В NoSQL всё хранится как текст/json
            d["nullable"] = true;
            d["default"] = "";
            details.append(d);
        }

        QJsonObject res;
        res["details"] = details;
        return makeResponse(res);
    });

    // 5. Foreign Keys (Expected by client)
    m_server.route("/api/foreign-keys/<arg>", [this](const QString &tableName) {
        QJsonObject res;
        res["foreign_keys"] = QJsonArray(); // NoSQL обычно не поддерживает FK на уровне БД
        return makeResponse(res);
    });

    // 6. Data (Get table rows)
    m_server.route("/api/<arg>", QHttpServerRequest::Method::Get,
                   [this](const QString &tableName, const QHttpServerRequest &req) {
                       qInfo() << "[HTTP] Fetching data for:" << tableName;
                       QUrlQuery query(req.url());
                       QString filters = query.queryItemValue("filters");

                       QJsonArray data;
                       if (DatabaseManager::instance().isNoSqlMode()) {
                           data = NoSQLManager::instance().getData(tableName, filters);
                       } else {
                           QString sql = QString("SELECT * FROM %1").arg(tableName);
                           if (!filters.isEmpty()) sql += " WHERE " + filters;
                           data = DatabaseManager::instance().executeSelect(sql);
                       }

                       QJsonObject res;
                       res["data"] = data; // Оборачиваем в "data", как хочет клиент
                       return makeResponse(res);
                   }
    );

    // 7. Execute Query (SQL via HTTP)
    m_server.route("/api/execute-query", QHttpServerRequest::Method::Post,
                   [this](const QHttpServerRequest &req) {
                       QJsonDocument doc = QJsonDocument::fromJson(req.body());
                       QString sql = doc.object()["sql"].toString();
                       qInfo() << "[HTTP] Execute query:" << sql;

                       QJsonArray data;
                       // Для NoSQL мы не можем выполнять чистый SQL,
                       // но можем имитировать SELECT * для простых случаев
                       if (sql.toUpper().startsWith("SELECT")) {
                           // Парсинг таблицы из SELECT * FROM table
                           QStringList parts = sql.split(" ", Qt::SkipEmptyParts);
                           int fromIdx = parts.indexOf("FROM");
                           if (fromIdx != -1 && fromIdx + 1 < parts.size()) {
                               QString table = parts[fromIdx + 1];
                               data = NoSQLManager::instance().getData(table, "");
                           }
                       }

                       QJsonObject res;
                       res["data"] = data;
                       return makeResponse(res);
                   }
    );

    // 8. Delete Record
    m_server.route("/api/<arg>/<arg>", QHttpServerRequest::Method::Delete,
                   [this](const QString &tableName, const QString &id) {
                       qInfo() << "[HTTP] Delete ID" << id << "from" << tableName;
                       bool ok = false;
                       if (DatabaseManager::instance().isNoSqlMode()) {
                           ok = NoSQLManager::instance().deleteData(tableName, id);
                       }
                       return makeResponse(ok ? QByteArray("OK") : QByteArray("Error"));
                   }
    );
}
