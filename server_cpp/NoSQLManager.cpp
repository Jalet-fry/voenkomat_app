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
#include <QDateTime>
#include <QDate>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QMap>
#include <algorithm>

// Статический кэш соединений для ускорения (NoSQL Connection Pool)
static QMap<QString, QString> g_activeConnections;

NoSQLManager::NoSQLManager(QObject *parent) : QObject(parent) {}

NoSQLManager& NoSQLManager::instance() {
    static NoSQLManager inst;
    return inst;
}

// Вспомогательный метод для получения/создания соединения
QSqlDatabase NoSQLManager::getDatabase(const QString &tableName) {
    QString dbPath = QString("%1/%2.db").arg(DatabaseManager::instance().noSqlPath(), tableName);
    QString connName = "nosql_conn_" + tableName;

    if (QSqlDatabase::contains(connName)) {
        QSqlDatabase db = QSqlDatabase::database(connName);
        if (db.isOpen()) return db;
        if (db.open()) return db;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
    db.setDatabaseName(dbPath);
    if (!db.open()) {
        qWarning() << "Failed to open NoSQL DB:" << dbPath << db.lastError().text();
    }
    return db;
}

QStringList NoSQLManager::getAllTables() {
    QString path = DatabaseManager::instance().noSqlPath();
    QDir dir(path);
    QStringList files = dir.entryList({"*.db"}, QDir::Files);
    QStringList tables;
    for (const QString &f : files) tables << f.section('.', 0, 0);
    return tables;
}

QString NoSQLManager::getPrimaryKey(const QString &tableName) {
    // Делаем поиск нечувствительным к регистру
    QString t = tableName.toLower();
    static QMap<QString, QString> pkMap = {
        {"conscripts", "conscript_id"},
        {"prizivnik", "id_prizivnik"},
        {"commissioners", "commissioner_id"},
        {"comissar", "id_comissar"},
        {"fitness_categories", "category_id"},
        {"medical_examinations", "certification_id"},
        {"military_id_cards", "ticket_id"},
        {"service_record_cards", "card_id"},
        {"callup_events", "event_id"}
    };
    return pkMap.value(t, "id");
}

QStringList NoSQLManager::getColumns(const QString &tableName) {
    QJsonArray data = getData(tableName, "", 1);
    QStringList cols;
    QString pk = getPrimaryKey(tableName);
    cols << pk;

    if (!data.isEmpty()) {
        QStringList keys = data.first().toObject().keys();
        keys.removeAll(pk);
        keys.sort(Qt::CaseInsensitive);
        cols << keys;
    }
    return cols;
}

QJsonArray NoSQLManager::getData(const QString &tableName, const QString &filters, int limit) {
    QJsonArray result;
    QSqlDatabase db = getDatabase(tableName);
    if (!db.isOpen()) return result;

    QString sql = "SELECT key, value FROM kv";
    QString whereClause = "";

    if (!filters.isEmpty()) {
        // Улучшенная регулярка: поддерживает кавычки и пробелы более надежно
        QRegularExpression re("(\\w+)\\s*(=|>|<|>=|<=|ILIKE|LIKE)\\s*['\"]?(.*?)['\"]?\\s*$", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch match = re.match(filters.trimmed());

        if (match.hasMatch()) {
            QString col = match.captured(1).trimmed();
            QString op = match.captured(2).toUpper();
            QString val = match.captured(3).trimmed();

            QString pk = getPrimaryKey(tableName);
            // Если запрос идет по PK или по универсальному "id"
            if (col.compare(pk, Qt::CaseInsensitive) == 0 || col.compare("id", Qt::CaseInsensitive) == 0) {
                if (op == "=" || op == "LIKE" || op == "ILIKE") {
                    whereClause = QString(" WHERE key %1 '%2'").arg(op == "ILIKE" ? "LIKE" : op, val);
                } else {
                    // Для NoSQL (SQLite) сравнение строк-чисел: длина, затем значение
                    whereClause = QString(" WHERE (length(key), key) %1 (length('%2'), '%2')").arg(op, val);
                }
            } else {
                // Поиск по JSON полям (для остальных колонок)
                if (op == "ILIKE" || op == "LIKE") {
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
    sql += " ORDER BY length(key) ASC, key ASC";
    if (limit > 0) sql += QString(" LIMIT %1").arg(limit);

    QSqlQuery query(db);
    if (query.exec(sql)) {
        QString pk = getPrimaryKey(tableName);
        while (query.next()) {
            QJsonObject obj = QJsonDocument::fromJson(query.value(1).toByteArray()).object();
            // Всегда подставляем актуальный ключ в поле PK
            obj[pk] = query.value(0).toString();
            // Также добавляем поле "id" для универсальности загрузки в UI
            obj["id"] = query.value(0).toString();

            // ЛОГ ДЛЯ ОТЛАДКИ (чтобы видеть, что находит сервер)
            qDebug() << "[NoSQL] Found record for" << tableName << ":" << query.value(0).toString();

            result.append(obj);
        }
    } else {
        qWarning() << "NoSQL Select Error:" << query.lastError().text() << "SQL:" << sql;
    }
    return result;
}

bool NoSQLManager::insertData(const QString &tableName, QJsonObject &data) {
    QSqlDatabase db = getDatabase(tableName);
    if (!db.isOpen()) return false;

    QString pk = getPrimaryKey(tableName);
    QString id;

    if (data.contains(pk)) id = data.value(pk).toVariant().toString();
    else if (data.contains("id")) id = data.value("id").toVariant().toString();

    // Автогенерация ID если пусто
    if (id.isEmpty()) {
        qint64 maxId = 0;
        QSqlQuery q(db);
        if (q.exec("SELECT key FROM kv")) {
            while (q.next()) {
                bool isNum;
                qint64 current = q.value(0).toLongLong(&isNum);
                if (isNum && current > maxId) maxId = current;
            }
        }
        id = QString::number(maxId + 1);
        data[pk] = id;
        data["id"] = id;
    }

    QSqlQuery q(db);
    q.prepare("INSERT OR REPLACE INTO kv (key, value) VALUES (?, ?)");
    q.addBindValue(id);
    q.addBindValue(QJsonDocument(data).toJson(QJsonDocument::Compact));
    bool ok = q.exec();
    if (!ok) qWarning() << "NoSQL Insert Error:" << q.lastError().text();
    return ok;
}

bool NoSQLManager::updateData(const QString &tableName, const QString &oldId, QJsonObject &data) {
    QSqlDatabase db = getDatabase(tableName);
    if (!db.isOpen()) return false;

    // 1. Загружаем текущие данные из базы
    QJsonObject existingData;
    QSqlQuery q(db);
    q.prepare("SELECT value FROM kv WHERE key = ?");
    q.addBindValue(oldId);

    QString pk = getPrimaryKey(tableName);

    if (q.exec() && q.next()) {
        QJsonDocument doc = QJsonDocument::fromJson(q.value(0).toByteArray());
        if (doc.isObject()) {
            existingData = doc.object();
        }
    } else {
        qWarning() << "NoSQL Update Error: Record not found" << oldId << "in" << tableName;
        return false;
    }

    // 2. Определяем новый ID (если он меняется)
    QString newId = oldId;
    // Ищем PK в присланных данных (с учетом регистра и без)
    for (const QString &key : data.keys()) {
        if (key.compare(pk, Qt::CaseInsensitive) == 0 || key.compare("id", Qt::CaseInsensitive) == 0) {
            QString val = data[key].toVariant().toString();
            if (!val.isEmpty()) {
                newId = val;
                break;
            }
        }
    }

    // 3. Умное слияние (Case-Insensitive Smart Merge)
    QMap<QString, QString> existingKeysMap;
    for (const QString &key : existingData.keys()) {
        existingKeysMap[key.toLower()] = key;
    }

    for (auto it = data.begin(); it != data.end(); ++it) {
        QString newKey = it.key();
        QJsonValue newVal = it.value();

        // Пропускаем обновление ID внутри JSON, если он пустой или совпадает с текущим PK
        if (newKey.compare(pk, Qt::CaseInsensitive) == 0 || newKey.compare("id", Qt::CaseInsensitive) == 0) {
            continue;
        }

        // Ищем, есть ли уже такой ключ в базе (игнорируя регистр)
        QString keyToUpdate = existingKeysMap.value(newKey.toLower(), newKey);

        // Защита: не затираем непустое значение в базе пустой строкой из формы
        if (newVal.isString() && newVal.toString().trimmed().isEmpty()) {
            if (existingData.contains(keyToUpdate) && !existingData[keyToUpdate].toVariant().toString().trimmed().isEmpty()) {
                continue;
            }
        }

        existingData[keyToUpdate] = newVal;
    }

    // Гарантируем синхронизацию PK и "id" внутри объекта
    existingData[pk] = newId.contains("_") ? newId : (newId.toLongLong() > 0 ? QJsonValue(newId.toLongLong()) : QJsonValue(newId));
    existingData["id"] = existingData[pk];

    // 4. Сохранение
    // Сначала пробуем вставить/обновить по новому ключу
    if (insertData(tableName, existingData)) {
        // Если ID изменился и новая запись успешно создана, удаляем старую
        if (newId != oldId) {
            qInfo() << "[NoSQL] Key changed from" << oldId << "to" << newId << ". Deleting old record.";
            deleteData(tableName, oldId);
        }
        data = existingData; // Возвращаем полный объект клиенту
        return true;
    }

    return false;
}

bool NoSQLManager::deleteData(const QString &tableName, const QString &id) {
    QSqlDatabase db = getDatabase(tableName);
    if (!db.isOpen()) return false;

    QSqlQuery q(db);
    q.prepare("DELETE FROM kv WHERE key = ?");
    q.addBindValue(id);
    return q.exec();
}

static int calculateAge(const QDate &birthDate) {
    QDate now = QDate::currentDate();
    int age = now.year() - birthDate.year();
    if (now.month() < birthDate.month() || (now.month() == birthDate.month() && now.day() < birthDate.day())) age--;
    return age;
}

static QString parseCity(QString addr) {
    if (addr.isEmpty()) return "Не указан";
    // Убираем "г.", "г " и пробелы
    QString city = addr.split(',').first().trimmed();
    city.remove("г.").remove("г ").trimmed();
    return city;
}

QJsonArray NoSQLManager::executeSpecialQuery(const QString &lab, const QString &queryNumber) {
    qInfo() << "[NoSQL] Special Query:" << lab << queryNumber;

    // --- ЛАБ 5 ---
    if (lab == "Lab5") {
        if (queryNumber == "5.1") {
            QJsonArray data = getData("conscripts");
            QJsonArray res;
            for (const QJsonValue &v : data) {
                QJsonObject p = v.toObject();
                p["age"] = calculateAge(p["birth_date"].toVariant().toDate());
                res.append(p);
            }
            QList<QJsonValue> list; for(auto v : res) list << v;
            std::sort(list.begin(), list.end(), [](const QJsonValue &a, const QJsonValue &b){
                return a.toObject()["age"].toInt() > b.toObject()["age"].toInt();
            });
            QJsonArray sorted; for(auto v : list) sorted << v;
            return sorted;
        }
        if (queryNumber == "5.2") {
            QJsonArray data = getData("conscripts");
            QMap<QString, int> counts;
            for (const QJsonValue &v : data) {
                QString city = parseCity(v.toObject()["residence_address"].toString());
                counts[city]++;
            }
            QJsonArray res;
            for (auto it = counts.begin(); it != counts.end(); ++it) {
                QJsonObject row; row["city"] = it.key(); row["count"] = it.value();
                res.append(row);
            }
            return res;
        }
        if (queryNumber == "5.3") {
            QJsonArray data = getData("conscripts");
            QJsonArray res;
            for (const QJsonValue &v : data) {
                QJsonObject p = v.toObject();
                int age = calculateAge(p["birth_date"].toVariant().toDate());
                if (age > 20) { p["age"] = age; res.append(p); }
            }
            return res;
        }
        if (queryNumber == "5.4") {
            QJsonArray ps = getData("conscripts"), vbs = getData("military_id_cards");
            QJsonArray res;
            for (const QJsonValue &pv : ps) {
                QJsonObject p = pv.toObject(); QString id = p["conscript_id"].toString();
                QJsonObject row; row["full_name"] = p["full_name"];
                for (const QJsonValue &vv : vbs) {
                    QJsonObject vb = vv.toObject();
                    if (vb["conscript_id"].toString() == id) {
                        row["ticket_number"] = vb["ticket_number"];
                        row["military_rank"] = vb["military_rank"];
                        break;
                    }
                }
                res.append(row);
            }
            return res;
        }
        if (queryNumber == "5.5") {
            QJsonArray data = getData("military_id_cards");
            QMap<QString, int> counts;
            for (const QJsonValue &v : data) counts[v.toObject()["military_rank"].toString()]++;
            QJsonArray res;
            for (auto it = counts.begin(); it != counts.end(); ++it) {
                QJsonObject row; row["military_rank"] = it.key(); row["count"] = it.value();
                res.append(row);
            }
            return res;
        }
        if (queryNumber == "5.6") {
            QJsonArray ps = getData("conscripts"), vbs = getData("military_id_cards"), fcs = getData("fitness_categories");
            QJsonArray res;
            for (const QJsonValue &vv : vbs) {
                QJsonObject vb = vv.toObject();
                if (vb["category"].toString() == "А") {
                    QJsonObject row;
                    for(auto pv : ps) if(pv.toObject()["conscript_id"].toString() == vb["conscript_id"].toString()) row["full_name"] = pv.toObject()["full_name"];
                    row["category_name"] = "А";
                    res.append(row);
                }
            }
            return res;
        }
        if (queryNumber == "5.7") {
            QJsonArray cs = getData("commissioners"), links = getData("conscripts_commissioners");
            QJsonArray res;
            for (const QJsonValue &cv : cs) {
                QJsonObject c = cv.toObject(); QString id = c["commissioner_id"].toString();
                int count = 0;
                for (auto lv : links) if (lv.toObject()["commissioner_id"].toString() == id) count++;
                QJsonObject row; row["full_name"] = c["full_name"]; row["count"] = count;
                res.append(row);
            }
            return res;
        }
        if (queryNumber == "5.8") {
            QJsonArray data = getData("commissioners");
            QList<QJsonValue> list; for(auto v : data) list << v;
            std::sort(list.begin(), list.end(), [](const QJsonValue &a, const QJsonValue &b){
                return a.toObject()["years_of_service"].toInt() > b.toObject()["years_of_service"].toInt();
            });
            QJsonArray sorted; for(int i=0; i<qMin(5, (int)list.size()); ++i) sorted << list[i];
            return sorted;
        }
        if (queryNumber == "5.9") {
            QJsonArray data = getData("commissioners");
            QJsonArray res;
            for (const QJsonValue &v : data) {
                QJsonObject c = v.toObject();
                if (c["position"].toString() == "Военный комиссар") res.append(c);
            }
            return res;
        }
        if (queryNumber == "5.10") {
            QJsonArray cs = getData("commissioners"), links = getData("conscripts_commissioners");
            QJsonArray res;
            for (const QJsonValue &cv : cs) {
                QJsonObject c = cv.toObject(); QString id = c["commissioner_id"].toString();
                int count = 0;
                for (auto lv : links) if (lv.toObject()["commissioner_id"].toString() == id) count++;
                QJsonObject row; row["commissioner_id"] = id; row["full_name"] = c["full_name"]; row["position"] = c["position"]; row["prizivniki_count"] = count;
                res.append(row);
            }
            return res;
        }
        if (queryNumber == "5.11") {
            QJsonArray cs = getData("commissioners"), ps = getData("conscripts"), links = getData("conscripts_commissioners");
            QJsonArray res;
            for (const QJsonValue &cv : cs) {
                QJsonObject c = cv.toObject(); QString id = c["commissioner_id"].toString();
                QStringList names;
                for (auto lv : links) {
                    if (lv.toObject()["commissioner_id"].toString() == id) {
                        QString pid = lv.toObject()["conscript_id"].toString();
                        for(auto pv : ps) if(pv.toObject()["conscript_id"].toString() == pid) names << pv.toObject()["full_name"].toString();
                    }
                }
                names.sort();
                QJsonObject row; row["commissioner_id"] = id; row["full_name"] = c["full_name"]; row["conscripts_list"] = names.join(", ");
                res.append(row);
            }
            return res;
        }
        if (queryNumber == "5.12") {
            QJsonArray data = getData("commissioners");
            QJsonArray res;
            for (const QJsonValue &v : data) {
                QJsonObject c = v.toObject();
                if (c["years_of_service"].toInt() > 10) res.append(c);
            }
            return res;
        }
        if (queryNumber == "5.13") {
            QJsonArray vbs = getData("military_id_cards"), ps = getData("conscripts");
            QJsonArray res;
            for (const QJsonValue &vv : vbs) {
                QJsonObject vb = vv.toObject(); QString pid = vb["conscript_id"].toString();
                QJsonObject row; row["ticket_id"] = vb["ticket_id"]; row["ticket_number"] = vb["ticket_number"]; row["military_rank"] = vb["military_rank"];
                for(auto pv : ps) {
                    QJsonObject p = pv.toObject();
                    if(p["conscript_id"].toString() == pid) {
                        row["conscript_name"] = p["full_name"];
                        row["conscript_age"] = calculateAge(p["birth_date"].toVariant().toDate());
                        break;
                    }
                }
                res.append(row);
            }
            return res;
        }
        if (queryNumber == "5.14") {
            QJsonArray data = getData("military_id_cards");
            QJsonArray res; QDate limit = QDate::fromString("2023-01-01", "yyyy-MM-dd");
            for (const QJsonValue &v : data) {
                QJsonObject vb = v.toObject();
                if (vb["issue_date"].toVariant().toDate() > limit) res.append(vb);
            }
            return res;
        }
        if (queryNumber == "5.15") {
            QJsonArray data = getData("military_id_cards");
            QMap<QString, int> counts;
            for (const QJsonValue &v : data) counts[v.toObject()["category"].toString()]++;
            QJsonArray res;
            for (auto it = counts.begin(); it != counts.end(); ++it) {
                QJsonObject row; row["category"] = it.key(); row["count"] = it.value();
                res.append(row);
            }
            return res;
        }
        if (queryNumber == "5.16") {
            QJsonArray data = getData("callup_events");
            QList<QJsonValue> list; for(auto v : data) list << v;
            std::sort(list.begin(), list.end(), [](const QJsonValue &a, const QJsonValue &b){
                return a.toObject()["event_datetime"].toString() > b.toObject()["event_datetime"].toString();
            });
            QJsonArray sorted; for(int i=0; i<qMin(5, (int)list.size()); ++i) sorted << list[i];
            return sorted;
        }
        if (queryNumber == "5.17") {
            QJsonArray cs = getData("commissioners"), evs = getData("callup_events");
            QJsonArray res;
            for (const QJsonValue &cv : cs) {
                QJsonObject c = cv.toObject(); QString id = c["commissioner_id"].toString();
                QStringList types; int count = 0;
                for (auto ev : evs) {
                    if (ev.toObject()["commissioner_id"].toString() == id) {
                        count++; QString t = ev.toObject()["event_type"].toString();
                        if (!types.contains(t)) types << t;
                    }
                }
                QJsonObject row; row["commissioner_name"] = c["full_name"]; row["events_count"] = count; row["event_types"] = types.join(", ");
                res.append(row);
            }
            return res;
        }
        if (queryNumber == "5.18") {
            QJsonArray evs = getData("callup_events"), ps = getData("conscripts"), links = getData("conscripts_events");
            QJsonArray res;
            for (const QJsonValue &ev : evs) {
                QJsonObject e = ev.toObject(); QString eid = e["event_id"].toString();
                for (auto lv : links) {
                    if (lv.toObject()["event_id"].toString() == eid) {
                        QString pid = lv.toObject()["conscript_id"].toString();
                        QJsonObject row; row["event_id"] = eid; row["event_type"] = e["event_type"]; row["event_datetime"] = e["event_datetime"]; row["event_location"] = e["event_location"];
                        for(auto pv : ps) if(pv.toObject()["conscript_id"].toString() == pid) { row["conscript_name"] = pv.toObject()["full_name"]; break; }
                        res.append(row);
                    }
                }
            }
            return res;
        }
        if (queryNumber == "5.19") {
            QJsonArray mes = getData("medical_examinations"), ps = getData("conscripts"), fcs = getData("fitness_categories");
            QJsonArray res;
            for (const QJsonValue &mv : mes) {
                QJsonObject m = mv.toObject();
                QJsonObject row; row["certification_id"] = m["certification_id"]; row["examination_date"] = m["examination_date"]; row["doctor_full_name"] = m["doctor_full_name"]; row["conclusion"] = m["conclusion"];
                for(auto pv : ps) if(pv.toObject()["conscript_id"].toString() == m["conscript_id"].toString()) row["conscript_name"] = pv.toObject()["full_name"];
                for(auto fv : fcs) if(fv.toObject()["category_id"].toString() == m["category_id"].toString()) row["category_name"] = fv.toObject()["category_name"];
                res.append(row);
            }
            return res;
        }
        if (queryNumber == "5.20") {
            QJsonArray data = getData("medical_examinations");
            QMap<QString, int> counts;
            for (const QJsonValue &v : data) counts[v.toObject()["doctor_full_name"].toString()]++;
            QJsonArray res;
            for (auto it = counts.begin(); it != counts.end(); ++it) {
                QJsonObject row; row["doctor_full_name"] = it.key(); row["examinations_count"] = it.value();
                res.append(row);
            }
            return res;
        }
        if (queryNumber == "5.21") {
            QJsonArray fcs = getData("fitness_categories"), ps = getData("conscripts"), vbs = getData("military_id_cards");
            QJsonArray res;
            for (const QJsonValue &fv : fcs) {
                QJsonObject fc = fv.toObject(); QString fid = fc["category_id"].toString();
                int count = 0; double sumAge = 0;
                for (auto vv : vbs) {
                    if (vv.toObject()["category_id"].toString() == fid) {
                        QString pid = vv.toObject()["conscript_id"].toString();
                        for(auto pv : ps) if(pv.toObject()["conscript_id"].toString() == pid) {
                            count++; sumAge += calculateAge(pv.toObject()["birth_date"].toVariant().toDate());
                        }
                    }
                }
                QJsonObject row; row["category_name"] = fc["category_name"]; row["category_index"] = fc["category_index"];
                row["full_category"] = fc["category_name"].toString() + fc["category_index"].toVariant().toString();
                row["prizivniki_count"] = count; row["avg_age"] = count > 0 ? qRound(sumAge/count * 100.0)/100.0 : 0;
                res.append(row);
            }
            return res;
        }
        if (queryNumber == "5.22") {
            QJsonArray ps = getData("conscripts"), vbs = getData("military_id_cards"), fcs = getData("fitness_categories");
            QJsonArray res;
            for (const QJsonValue &pv : ps) {
                QJsonObject p = pv.toObject(); QString pid = p["conscript_id"].toString();
                for (auto vv : vbs) {
                    QJsonObject vb = vv.toObject();
                    if (vb["conscript_id"].toString() == pid) {
                        QJsonObject row; row["conscript_name"] = p["full_name"]; row["ticket_category"] = vb["category"];
                        for(auto fv : fcs) if(fv.toObject()["category_id"].toString() == vb["category_id"].toString()) {
                            QJsonObject fc = fv.toObject();
                            row["category_name"] = fc["category_name"]; row["category_index"] = fc["category_index"];
                            row["full_category"] = fc["category_name"].toString() + fc["category_index"].toVariant().toString();
                            row["restriction_description"] = fc["restriction_description"];
                        }
                        res.append(row);
                    }
                }
            }
            return res;
        }
        if (queryNumber == "5.23") {
            QJsonArray ps = getData("conscripts"), vbs = getData("military_id_cards"), fcs = getData("fitness_categories");
            QJsonArray res;
            for (const QJsonValue &pv : ps) {
                QJsonObject p = pv.toObject(); QString pid = p["conscript_id"].toString();
                QStringList cats; int count = 0;
                for (auto vv : vbs) {
                    if (vv.toObject()["conscript_id"].toString() == pid) {
                        count++; QString fid = vv.toObject()["category_id"].toString();
                        for(auto fv : fcs) if(fv.toObject()["category_id"].toString() == fid)
                            cats << fv.toObject()["category_name"].toString() + fv.toObject()["category_index"].toVariant().toString();
                    }
                }
                QJsonObject row; row["conscript_name"] = p["full_name"]; row["categories_list"] = cats.join(", "); row["categories_count"] = count;
                res.append(row);
            }
            return res;
        }
        if (queryNumber == "5.24" || queryNumber == "5.25") {
            QJsonArray fcs = getData("fitness_categories"), ps = getData("conscripts"), vbs = getData("military_id_cards");
            QJsonArray res;
            for (const QJsonValue &fv : fcs) {
                QJsonObject fc = fv.toObject(); QString fid = fc["category_id"].toString();
                QSet<QString> uniqueP;
                for (auto vv : vbs) if (vv.toObject()["category_id"].toString() == fid) uniqueP.insert(vv.toObject()["conscript_id"].toString());
                QJsonObject row; row["category_id"] = fid; row["category_name"] = fc["category_name"]; row["category_index"] = fc["category_index"];
                row["full_category"] = fc["category_name"].toString() + fc["category_index"].toVariant().toString();
                row["prizivniki_count"] = uniqueP.size(); row["total_prizivniki"] = uniqueP.size();
                res.append(row);
            }
            return res;
        }
        if (queryNumber == "5.26") {
            QJsonArray fcs = getData("fitness_categories"), vbs = getData("military_id_cards");
            QJsonArray res;
            for (const QJsonValue &fv : fcs) {
                QJsonObject fc = fv.toObject(); QString fid = fc["category_id"].toString();
                bool found = false;
                for (auto vv : vbs) if (vv.toObject()["category_id"].toString() == fid) { found = true; break; }
                if (!found) {
                    QJsonObject row; row["category_id"] = fid; row["category_name"] = fc["category_name"]; row["category_index"] = fc["category_index"];
                    row["full_category"] = fc["category_name"].toString() + fc["category_index"].toVariant().toString();
                    res.append(row);
                }
            }
            return res;
        }
        if (queryNumber == "5.27") {
            QJsonArray vbs = getData("military_id_cards"), fcs = getData("fitness_categories");
            QJsonArray res;
            for (const QJsonValue &vv : vbs) {
                QJsonObject vb = vv.toObject(); QJsonObject row = vb;
                for(auto fv : fcs) if(fv.toObject()["category_id"].toString() == vb["category_id"].toString()) {
                    QJsonObject fc = fv.toObject();
                    row["category_name"] = fc["category_name"]; row["category_index"] = fc["category_index"];
                    row["full_category"] = fc["category_name"].toString() + fc["category_index"].toVariant().toString();
                    row["restriction_description"] = fc["restriction_description"]; row["category_basis"] = fc["category_basis"];
                }
                res.append(row);
            }
            return res;
        }
        if (queryNumber == "5.28") {
            QJsonArray fcs = getData("fitness_categories");
            QJsonArray res;
            for (const QJsonValue &fv : fcs) {
                QJsonObject fc = fv.toObject();
                QJsonObject row; row["category_id"] = fc["category_id"];
                row["full_category"] = fc["category_name"].toString() + fc["category_index"].toVariant().toString();
                row["description"] = fc["restriction_description"]; row["basis"] = fc["category_basis"];
                res.append(row);
            }
            return res;
        }
        if (queryNumber == "5.29") {
            QJsonArray fcs = getData("fitness_categories"), vbs = getData("military_id_cards");
            QJsonArray res;
            for (const QJsonValue &fv : fcs) {
                QJsonObject fc = fv.toObject(); QString fid = fc["category_id"].toString();
                int total = 0; QSet<QString> unique;
                for (auto vv : vbs) if (vv.toObject()["category_id"].toString() == fid) { total++; unique.insert(vv.toObject()["conscript_id"].toString()); }
                QJsonObject row; row["category_name"] = fc["category_name"]; row["total_conscripts"] = total; row["unique_conscripts"] = unique.size();
                res.append(row);
            }
            return res;
        }
        if (queryNumber == "5.30") {
            QJsonArray fcs = getData("fitness_categories"), ps = getData("conscripts"), vbs = getData("military_id_cards");
            QJsonArray res; int totalP = ps.size();
            for (const QJsonValue &fv : fcs) {
                QJsonObject fc = fv.toObject(); QString fid = fc["category_id"].toString();
                QSet<QString> unique;
                for (auto vv : vbs) if (vv.toObject()["category_id"].toString() == fid) unique.insert(vv.toObject()["conscript_id"].toString());
                QJsonObject row; row["category_id"] = fid; row["category_name"] = fc["category_name"]; row["category_index"] = fc["category_index"];
                row["full_category"] = fc["category_name"].toString() + fc["category_index"].toVariant().toString();
                row["conscripts_count"] = unique.size();
                row["percentage"] = totalP > 0 ? qRound(unique.size() * 100.0 / totalP * 100.0)/100.0 : 0;
                res.append(row);
            }
            return res;
        }
    }

    // --- ЛАБ 6 ---
    if (lab == "Lab6") {
        if (queryNumber == "6.1") {
            QJsonArray ps = getData("conscripts"), vbs = getData("military_id_cards"), fcs = getData("fitness_categories");
            QJsonArray res;
            for (const QJsonValue &pv : ps) {
                QJsonObject p = pv.toObject(); QString pid = p["conscript_id"].toString();
                for (auto vv : vbs) {
                    QJsonObject vb = vv.toObject();
                    if (vb["conscript_id"].toString() == pid) {
                        QJsonObject row = p;
                        for(auto fv : fcs) if(fv.toObject()["category_id"].toString() == vb["category_id"].toString()) row["kategoria_godnosti"] = fv.toObject()["category_name"];
                        row["age"] = calculateAge(p["birth_date"].toVariant().toDate());
                        res.append(row); break;
                    }
                }
            }
            return res;
        }
        if (queryNumber == "6.2") {
            QJsonArray data = getData("conscripts");
            QMap<QString, int> counts;
            for (const QJsonValue &v : data) counts[parseCity(v.toObject()["residence_address"].toString())]++;
            QJsonArray res;
            for (auto it = counts.begin(); it != counts.end(); ++it) if(it.value() == 1) {
                QJsonObject row; row["city"] = it.key(); row["conscripts_count"] = it.value(); res.append(row);
            }
            return res;
        }
        if (queryNumber == "6.3") {
            QJsonArray ps = getData("conscripts"), mes = getData("medical_examinations");
            QJsonArray res;
            for (const QJsonValue &pv : ps) {
                QJsonObject p = pv.toObject(); QString pid = p["conscript_id"].toString();
                int count = 0;
                for(auto mv : mes) if(mv.toObject()["conscript_id"].toString() == pid) count++;
                p["examinations_count"] = count; res.append(p);
            }
            return res;
        }
        if (queryNumber == "6.4") {
            QJsonArray ps = getData("conscripts"), vbs = getData("military_id_cards");
            QJsonArray res;
            for (const QJsonValue &pv : ps) {
                QJsonObject p = pv.toObject(); QString pid = p["conscript_id"].toString();
                QJsonObject row = p;
                for(auto vv : vbs) if(vv.toObject()["conscript_id"].toString() == pid) {
                    QJsonObject vb = vv.toObject();
                    row["ticket_number"] = vb["ticket_number"]; row["military_rank"] = vb["military_rank"]; row["category"] = vb["category"]; row["issue_date"] = vb["issue_date"];
                }
                res.append(row);
            }
            return res;
        }
        if (queryNumber == "6.5") {
            QJsonArray data = getData("conscripts");
            QMap<QString, int> counts;
            for (const QJsonValue &v : data) counts[parseCity(v.toObject()["residence_address"].toString())]++;
            QJsonArray res;
            for (auto it = counts.begin(); it != counts.end(); ++it) {
                QJsonObject row; row["city"] = it.key(); row["conscripts_count"] = it.value(); res.append(row);
            }
            return res;
        }
        if (queryNumber == "6.6") {
            QJsonArray ps = getData("conscripts"), vbs = getData("military_id_cards");
            QJsonArray res;
            for (const QJsonValue &pv : ps) {
                QJsonObject p = pv.toObject(); QString pid = p["conscript_id"].toString();
                for(auto vv : vbs) if(vv.toObject()["conscript_id"].toString() == pid) {
                    p["age"] = calculateAge(p["birth_date"].toVariant().toDate());
                    res.append(p); break;
                }
            }
            return res;
        }
        if (queryNumber == "6.7") {
            QJsonArray cs = getData("commissioners"), links = getData("conscripts_commissioners");
            QJsonArray res;
            for (const QJsonValue &cv : cs) {
                QJsonObject c = cv.toObject(); QString id = c["commissioner_id"].toString();
                int count = 0;
                for (auto lv : links) if (lv.toObject()["commissioner_id"].toString() == id) count++;
                QJsonObject row; row["commissioner_id"] = id; row["full_name"] = c["full_name"]; row["position"] = c["position"]; row["conscripts_count"] = count;
                res.append(row);
            }
            return res;
        }
        if (queryNumber == "6.8") {
            QJsonArray ps = getData("conscripts"), links = getData("conscripts_events");
            QJsonArray res;
            for (const QJsonValue &pv : ps) {
                QJsonObject p = pv.toObject(); QString pid = p["conscript_id"].toString();
                int count = 0;
                for (auto lv : links) if (lv.toObject()["conscript_id"].toString() == pid) count++;
                if (count > 0) { p["events_count"] = count; res.append(p); }
            }
            QList<QJsonValue> list; for(auto v : res) list << v;
            std::sort(list.begin(), list.end(), [](const QJsonValue &a, const QJsonValue &b){ return a.toObject()["events_count"].toInt() > b.toObject()["events_count"].toInt(); });
            QJsonArray sorted; for(int i=0; i<qMin(5, (int)list.size()); ++i) sorted << list[i];
            return sorted;
        }
        if (queryNumber == "6.9") {
            QJsonArray ps = getData("conscripts"), mes = getData("medical_examinations");
            QJsonArray res;
            for (const QJsonValue &pv : ps) {
                QJsonObject p = pv.toObject(); QString pid = p["conscript_id"].toString();
                bool found = false;
                for (auto mv : mes) if (mv.toObject()["conscript_id"].toString() == pid) { found = true; break; }
                if (!found) res.append(p);
            }
            return res;
        }
        if (queryNumber == "6.10") {
            QJsonArray cs = getData("commissioners"), links = getData("conscripts_commissioners");
            QJsonArray res;
            for (const QJsonValue &cv : cs) {
                QJsonObject c = cv.toObject(); QString id = c["commissioner_id"].toString();
                int count = 0;
                for (auto lv : links) if (lv.toObject()["commissioner_id"].toString() == id) count++;
                QJsonObject row = c; row["conscripts_count"] = count; res.append(row);
            }
            return res;
        }
        if (queryNumber == "6.11") {
            QJsonArray data = getData("commissioners");
            QJsonArray res;
            for (const QJsonValue &v : data) {
                QJsonObject c = v.toObject();
                if (c["position"].toString() == "Военный комиссар") res.append(c);
            }
            return res;
        }
        if (queryNumber == "6.12") {
            QJsonArray data = getData("commissioners");
            int maxS = 0; for(auto v : data) maxS = qMax(maxS, v.toObject()["years_of_service"].toInt());
            QJsonArray res;
            for (const QJsonValue &v : data) if(v.toObject()["years_of_service"].toInt() == maxS) res.append(v.toObject());
            return res;
        }
        if (queryNumber == "6.13") return executeSpecialQuery("Lab5", "5.13");
        if (queryNumber == "6.14") return executeSpecialQuery("Lab5", "5.15");
        if (queryNumber == "6.15") {
            QJsonArray vbs = getData("military_id_cards"), ps = getData("conscripts");
            QJsonArray res; QDate limit = QDate::fromString("2000-01-01", "yyyy-MM-dd");
            for (const QJsonValue &vv : vbs) {
                QJsonObject vb = vv.toObject(); QString pid = vb["conscript_id"].toString();
                for(auto pv : ps) {
                    QJsonObject p = pv.toObject();
                    if(p["conscript_id"].toString() == pid && p["birth_date"].toVariant().toDate() < limit) { res.append(vb); break; }
                }
            }
            return res;
        }
        if (queryNumber == "6.16") return executeSpecialQuery("Lab5", "5.16");
        if (queryNumber == "6.17") return executeSpecialQuery("Lab5", "5.17");
        if (queryNumber == "6.18") return executeSpecialQuery("Lab5", "5.18");
        if (queryNumber == "6.19") {
            QJsonArray mes = getData("medical_examinations"), ps = getData("conscripts"), fcs = getData("fitness_categories");
            QJsonArray res;
            for (const QJsonValue &mv : mes) {
                QJsonObject m = mv.toObject();
                QJsonObject row = m;
                for(auto pv : ps) if(pv.toObject()["conscript_id"].toString() == m["conscript_id"].toString()) row["conscript_name"] = pv.toObject()["full_name"];
                for(auto fv : fcs) if(fv.toObject()["category_id"].toString() == m["category_id"].toString()) {
                    QJsonObject fc = fv.toObject(); row["category_name"] = fc["category_name"]; row["category_index"] = fc["category_index"];
                    row["full_category"] = fc["category_name"].toString() + fc["category_index"].toVariant().toString();
                }
                res.append(row);
            }
            return res;
        }
        if (queryNumber == "6.20") return executeSpecialQuery("Lab5", "5.20");
        if (queryNumber == "6.21") {
            QJsonArray ps = getData("conscripts"), vbs = getData("military_id_cards"), fcs = getData("fitness_categories");
            QJsonArray res;
            for (const QJsonValue &vv : vbs) {
                QJsonObject vb = vv.toObject();
                if (vb["category"].toString() == "А") {
                    QJsonObject row; row["conscript_id"] = vb["conscript_id"]; row["category"] = "А";
                    for(auto pv : ps) if(pv.toObject()["conscript_id"].toString() == vb["conscript_id"].toString()) row["full_name"] = pv.toObject()["full_name"];
                    for(auto fv : fcs) if(fv.toObject()["category_id"].toString() == vb["category_id"].toString()) {
                        QJsonObject fc = fv.toObject(); row["category_name"] = fc["category_name"]; row["category_index"] = fc["category_index"];
                        row["full_category"] = fc["category_name"].toString() + fc["category_index"].toVariant().toString();
                    }
                    res.append(row);
                }
            }
            return res;
        }
        if (queryNumber == "6.22") return executeSpecialQuery("Lab5", "5.22");
        if (queryNumber == "6.23") return executeSpecialQuery("Lab5", "5.23");
        if (queryNumber == "6.24") return executeSpecialQuery("Lab5", "5.24");
        if (queryNumber == "6.25") {
            QJsonArray fcs = getData("fitness_categories"), ps = getData("conscripts"), vbs = getData("military_id_cards");
            QJsonArray res;
            for (const QJsonValue &fv : fcs) {
                QJsonObject fc = fv.toObject(); QString fid = fc["category_id"].toString();
                QSet<QString> unique; int bCount = 0;
                for (auto vv : vbs) if (vv.toObject()["category_id"].toString() == fid) { bCount++; unique.insert(vv.toObject()["conscript_id"].toString()); }
                QJsonObject row; row["category_name"] = fc["category_name"]; row["total_conscripts"] = unique.size(); row["biletov_count"] = bCount;
                res.append(row);
            }
            return res;
        }
        if (queryNumber == "6.26") return executeSpecialQuery("Lab5", "5.26");
        if (queryNumber == "6.27") {
            QJsonArray ps = getData("conscripts"), vbs = getData("military_id_cards"), fcs = getData("fitness_categories");
            QJsonArray res;
            for (const QJsonValue &vv : vbs) {
                QJsonObject vb = vv.toObject(); QString fid = vb["category_id"].toString();
                for(auto fv : fcs) {
                    QJsonObject fc = fv.toObject();
                    if(fc["category_id"].toString() == fid && fc["category_index"].toInt() > 1) {
                        QJsonObject row; row["conscript_id"] = vb["conscript_id"]; row["category_name"] = fc["category_name"]; row["category_index"] = fc["category_index"];
                        row["full_category"] = fc["category_name"].toString() + fc["category_index"].toVariant().toString();
                        for(auto pv : ps) if(pv.toObject()["conscript_id"].toString() == vb["conscript_id"].toString()) row["full_name"] = pv.toObject()["full_name"];
                        res.append(row); break;
                    }
                }
            }
            return res;
        }
        if (queryNumber == "6.28") return executeSpecialQuery("Lab5", "5.28");
        if (queryNumber == "6.29") {
            QJsonArray fcs = getData("fitness_categories"), vbs = getData("military_id_cards");
            QJsonArray res;
            for (const QJsonValue &fv : fcs) {
                QJsonObject fc = fv.toObject(); QString fid = fc["category_id"].toString();
                int count = 0;
                for (auto vv : vbs) if (vv.toObject()["category_id"].toString() == fid) count++;
                if (count > 0) {
                    QJsonObject row = fc; row["usage_count"] = count;
                    row["full_category"] = fc["category_name"].toString() + fc["category_index"].toVariant().toString();
                    res.append(row);
                }
            }
            return res;
        }
        if (queryNumber == "6.30") {
            QJsonArray ps = getData("conscripts"), vbs = getData("military_id_cards"), fcs = getData("fitness_categories");
            QJsonArray res; int totalB = vbs.size();
            for (const QJsonValue &fv : fcs) {
                QJsonObject fc = fv.toObject(); QString fid = fc["category_id"].toString();
                QSet<QString> unique;
                for (auto vv : vbs) if (vv.toObject()["category_id"].toString() == fid) unique.insert(vv.toObject()["conscript_id"].toString());
                QJsonObject row; row["category_name"] = fc["category_name"]; row["conscripts_count"] = unique.size();
                row["percentage"] = totalB > 0 ? qRound(unique.size() * 100.0 / totalB * 100.0)/100.0 : 0;
                res.append(row);
            }
            return res;
        }
        if (queryNumber == "6.31") {
            QJsonArray ps = getData("conscripts"), cs = getData("commissioners");
            QSet<QString> names;
            for(auto v : ps) names.insert(v.toObject()["full_name"].toString());
            for(auto v : cs) names.insert(v.toObject()["full_name"].toString());
            QStringList sorted = names.values(); sorted.sort();
            QJsonArray res; for(int i=0; i<qMin(10, (int)sorted.size()); ++i) { QJsonObject row; row["participant_name"] = sorted[i]; res.append(row); }
            return res;
        }
        if (queryNumber == "6.32") {
            QJsonArray ps = getData("conscripts"), vbs = getData("military_id_cards");
            QSet<QString> hasVb;
            for(auto v : vbs) hasVb.insert(v.toObject()["conscript_id"].toString());
            QJsonArray res;
            for(auto v : ps) if(!hasVb.contains(v.toObject()["conscript_id"].toString())) { QJsonObject row; row["full_name"] = v.toObject()["full_name"]; res.append(row); }
            return res;
        }
        if (queryNumber == "6.33") {
            QJsonArray ps = getData("conscripts"), evs = getData("callup_events");
            QSet<QString> pCities, eCities;
            for(auto v : ps) pCities.insert(parseCity(v.toObject()["residence_address"].toString()));
            for(auto v : evs) eCities.insert(parseCity(v.toObject()["event_location"].toString()));
            QJsonArray res; QStringList intersect = (pCities & eCities).values(); intersect.sort();
            for(auto c : intersect) { QJsonObject row; row["city"] = c; res.append(row); }
            return res;
        }
        if (queryNumber == "6.34") {
            QJsonArray fcs = getData("fitness_categories"), vbs = getData("military_id_cards"), mes = getData("medical_examinations");
            QSet<QString> vbIds, meIds;
            for(auto v : vbs) vbIds.insert(v.toObject()["category_id"].toString());
            for(auto v : mes) meIds.insert(v.toObject()["category_id"].toString());
            QSet<QString> common = vbIds & meIds;
            QJsonArray res;
            for(auto fv : fcs) {
                QJsonObject fc = fv.toObject();
                if(common.contains(fc["category_id"].toString())) {
                    QJsonObject row = fc; row["full_category"] = fc["category_name"].toString() + fc["category_index"].toVariant().toString();
                    res.append(row);
                }
            }
            return res;
        }
    }

    // Универсальный фолбэк: пробуем найти таблицу по SQL файлу
    QString projectRoot = DatabaseManager::instance().projectRoot();
    QString sqlPath = QDir::cleanPath(QString("%1/resources_old/queries/%2/%3.sql").arg(projectRoot, lab, queryNumber));
    QFile file(sqlPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString content = QTextStream(&file).readAll().toUpper();
        QRegularExpression re("FROM\\s+(?:PUBLIC\\.)?(\\w+)");
        QRegularExpressionMatch match = re.match(content);
        if (match.hasMatch()) return getData(match.captured(1).toLower());
    }

    return QJsonArray();
}

QJsonObject NoSQLManager::getSpecialQueriesInfo() {
    QJsonObject res;
    QString projectRoot = DatabaseManager::instance().projectRoot();
    QString queriesPath = QDir::cleanPath(projectRoot + "/resources_old/queries");

    for (const QString lab : {"Lab5", "Lab6"}) {
        QJsonArray list;
        QDir dir(queriesPath + "/" + lab);
        QStringList files = dir.entryList({"*.sql"}, QDir::Files, QDir::Name);
        for (const QString &f : files) {
            QJsonObject q;
            q["id"] = f.section('.', 0, 0);
            q["filename"] = f;

            // Читаем первую строку комментария как описание
            QFile file(dir.absoluteFilePath(f));
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString firstLine = file.readLine().trimmed();
                if (firstLine.startsWith("--")) q["description"] = firstLine.mid(2).trimmed();
                else q["description"] = f;
            }
            list.append(q);
        }
        res[lab] = list;
    }
    return res;
}
