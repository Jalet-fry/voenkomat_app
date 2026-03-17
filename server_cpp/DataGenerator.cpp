#include "DataGenerator.h"
#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDate>
#include <QDateTime>
#include <random>

DataGenerator::DataGenerator(QObject *parent) : QObject(parent) {}

void DataGenerator::run()
{
    qInfo() << "--- ГЕНЕРАЦИЯ ДАННЫХ (C++) ---";

    QStringList tables = {
        "conscripts_events", "conscripts_commissioners", "medical_examinations",
        "military_id_cards", "service_record_cards", "callup_events",
        "conscripts", "commissioners", "fitness_categories"
    };

    foreach(const QString &t, tables) {
        DatabaseManager::instance().executeModify(QString("TRUNCATE public.%1 RESTART IDENTITY CASCADE").arg(t));
    }
    qInfo() << "Таблицы очищены.";

    // 1. Категории
    DatabaseManager::instance().executeModify("INSERT INTO fitness_categories (category_name, restriction_description, category_index) VALUES "
                                              "('А', 'Годен', 1), ('Б', 'Годен с огр.', 2), ('В', 'Огр. годен', 3), "
                                              "('Г', 'Временно не годен', 4), ('Д', 'Не годен', 5)");

    QJsonArray catRes = DatabaseManager::instance().executeSelect("SELECT category_id FROM fitness_categories");
    QVariantList catIds;
    for(const auto &v : catRes) catIds << v.toObject()["category_id"].toVariant();

    // 2. Комиссары
    QStringList names = {"Иванов И.И.", "Петров П.П.", "Сидоров С.С.", "Кузнецов А.В.", "Смирнов Б.Н."};
    QVariantList commIds;
    foreach(const QString &name, names) {
        QJsonObject res = DatabaseManager::instance().executeModify("INSERT INTO commissioners (full_name, position, years_of_service) VALUES (?, ?, ?) RETURNING commissioner_id",
                                                                    {name, "Полковник", 15 + (rand() % 15)});
        commIds << res["data"].toArray().first().toObject()["commissioner_id"].toVariant();
    }

    // 3. Призывники (200 человек)
    qInfo() << "Генерация 200 призывников...";
    QStringList firstNames = {"Александр", "Иван", "Сергей", "Дмитрий", "Андрей", "Михаил", "Николай"};
    QStringList lastNames = {"Иванов", "Смирнов", "Кузнецов", "Попов", "Васильев", "Петров", "Соколов"};

    for(int i = 0; i < 200; ++i) {
        QString fullName = lastNames[rand() % lastNames.size()] + " " + firstNames[rand() % firstNames.size()];
        QDate birth = QDate::currentDate().addYears(-18 - (rand() % 9)).addDays(rand() % 365);
        QString passport = QString("%1 %2").arg(1000 + rand() % 9000).arg(100000 + rand() % 900000);

        QJsonObject res = DatabaseManager::instance().executeModify("INSERT INTO conscripts (full_name, birth_date, residence_address, passport_number) VALUES (?, ?, ?, ?) RETURNING conscript_id",
                                                                    {fullName, birth, "г. Москва, ул. Ленина, д. " + QString::number(i+1), passport});

        int cid = res["data"].toArray().first().toObject()["conscript_id"].toInt();

        // Медосмотр
        DatabaseManager::instance().executeModify("INSERT INTO medical_examinations (examination_date, conscript_id, category_id, conclusion) VALUES (?, ?, ?, ?)",
                                                  {QDate::currentDate().addDays(-(rand() % 100)), cid, catIds[rand() % catIds.size()], "Годен"});
    }

    qInfo() << "--- ГЕНЕРАЦИЯ ЗАВЕРШЕНА ---";
}
