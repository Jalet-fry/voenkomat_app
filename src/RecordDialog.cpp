#include "RecordDialog.h"
#include "DbConstants.h"
#include "ConfigManager.h"
#include <QPushButton>
#include <QMessageBox>
#include <QDate>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonValue>
#include <QKeyEvent>
#include <QCompleter>
#include <QJsonArray>
#include <QHBoxLayout>

#include <QIntValidator>
#include <QDoubleValidator>

RecordDialog::RecordDialog(DatabaseManager *dbManager, const QString &tableName, QWidget *parent, const QString &recordId)
    : QDialog(parent)
    , m_dbManager(dbManager)
    , m_tableName(tableName)
    , m_recordId(recordId)
{
    ConfigManager config("config.ini");
    m_isClassicUI = config.isClassicUI();

    setWindowTitle(recordId.isEmpty() ? "Добавление записи" : "Редактирование записи");
    setMinimumSize(600, 650);

    m_columns = m_dbManager->getColumnList(tableName);

    // ПОЛНЫЙ РУССКИЙ МАППИНГ ДЛЯ ВСЕХ ТАБЛИЦ ИЗ ТВОЕГО ДАМПА
    m_fieldDisplayNames["id_prizivnik"] = "Призывник (ID)";
    m_fieldDisplayNames["conscript_id"] = "Призывник (ID)";
    m_fieldDisplayNames["fio"] = "ФИО";
    m_fieldDisplayNames["full_name"] = "ФИО";
    m_fieldDisplayNames["data_rozhdeniya"] = "Дата рождения";
    m_fieldDisplayNames["birth_date"] = "Дата рождения";
    m_fieldDisplayNames["adres_prozhivaniya"] = "Адрес прописки";
    m_fieldDisplayNames["residence_address"] = "Адрес прописки";
    m_fieldDisplayNames["nomer_pasporta"] = "Серия/Номер паспорта";
    m_fieldDisplayNames["passport_number"] = "Серия/Номер паспорта";

    m_fieldDisplayNames["id_voennogo_bileta"] = "Связанный военный билет";
    m_fieldDisplayNames["military_ticket_id"] = "Связанный военный билет";
    m_fieldDisplayNames["id_voenno_uchetnoi_karty"] = "Связанная учетная карта";
    m_fieldDisplayNames["registration_card_id"] = "Связанная учетная карта";

    m_fieldDisplayNames["id_bileta"] = "ID Билета";
    m_fieldDisplayNames["ticket_id"] = "ID Билета";
    m_fieldDisplayNames["nomer_bileta"] = "Номер билета";
    m_fieldDisplayNames["ticket_number"] = "Номер билета";
    m_fieldDisplayNames["data_vydachi"] = "Дата выдачи";
    m_fieldDisplayNames["issue_date"] = "Дата выдачи";
    m_fieldDisplayNames["voinskoe_zvanie"] = "Воинское звание";
    m_fieldDisplayNames["military_rank"] = "Воинское звание";
    m_fieldDisplayNames["kategoria"] = "Категория годности (литера)";
    m_fieldDisplayNames["category"] = "Категория годности (литера)";

    m_fieldDisplayNames["id_karty"] = "ID Учетной карты";
    m_fieldDisplayNames["card_id"] = "ID Учетной карты";
    m_fieldDisplayNames["nomer_karty"] = "Номер карты";
    m_fieldDisplayNames["card_number"] = "Номер карты";
    m_fieldDisplayNames["data_postanovki_na_uchet"] = "Дата постановки на учет";
    m_fieldDisplayNames["registration_date"] = "Дата постановки на учет";
    m_fieldDisplayNames["voenno_uchetnaya_specialnost"] = "ВУС (Специальность)";
    m_fieldDisplayNames["military_specialty"] = "ВУС (Специальность)";

    m_fieldDisplayNames["id_comissar"] = "Комиссар (ID)";
    m_fieldDisplayNames["commissioner_id"] = "Комиссар (ID)";
    m_fieldDisplayNames["dolzhnost"] = "Должность";
    m_fieldDisplayNames["position"] = "Должность";
    m_fieldDisplayNames["stazh_raboty"] = "Стаж работы (лет)";
    m_fieldDisplayNames["years_of_service"] = "Стаж работы (лет)";
    m_fieldDisplayNames["kontaktnyi_telefon"] = "Контактный телефон";
    m_fieldDisplayNames["phone_number"] = "Контактный телефон";

    setupUI();
    setupStyles();

    if (!m_recordId.isEmpty()) {
        loadRecordData();
    }
    setupAutocomplete();
}

void RecordDialog::setupUI()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(20, 20, 20, 20);
    m_layout->setSpacing(10);

    QGridLayout *form = new QGridLayout();
    QString pk = m_dbManager->getPrimaryKeyColumn(m_tableName);

    QList<DatabaseManager::ForeignKeyInfo> fks = m_dbManager->getForeignKeyInfo(m_tableName);
    QMap<QString, DatabaseManager::ForeignKeyInfo> fkMap;
    foreach(const auto &fk, fks) fkMap[fk.columnName] = fk;

    int row = 0;
    foreach (const QString &col, m_columns) {
        // УБИРАЕМ ТОЛЬКО ТЕХНИЧЕСКИЙ "id", если он дублирует PK
        if (col.toLower() == "id" && pk.toLower() != "id") {
            continue;
        }

        QLabel *label = new QLabel(getDisplayName(col) + ":", this);

        if (fkMap.contains(col)) {
            QWidget *container = new QWidget(this);
            QHBoxLayout *hL = new QHBoxLayout(container);
            hL->setContentsMargins(0,0,0,0);

            QComboBox *combo = new QComboBox(this);
            combo->addItem("--- Не выбрано (ПУСТО) ---", QVariant("")); // Явное пустое значение

            QString refTable = fkMap[col].referencedTable;
            QString refCol = fkMap[col].referencedColumn;

            auto reloadCombo = [this, combo, refTable, refCol]() {
                QVariant current = combo->currentData();
                combo->clear();
                combo->addItem("--- Не выбрано (ПУСТО) ---", QVariant(""));
                QJsonArray refData = m_dbManager->fetchTableDataHttp(refTable);
                for(int j=0; j<refData.size(); ++j) {
                    QJsonObject obj = refData[j].toObject();
                    QString id = obj[refCol].toVariant().toString();
                    QString info = id;
                    if (obj.contains("fio")) info += " [" + obj["fio"].toString() + "]";
                    else if (obj.contains("full_name")) info += " [" + obj["full_name"].toString() + "]";
                    else if (obj.contains("nomer_bileta")) info += " [" + obj["nomer_bileta"].toString() + "]";
                    else if (obj.contains("ticket_number")) info += " [" + obj["ticket_number"].toString() + "]";
                    combo->addItem(info, id);
                }
                if (current.isValid()) combo->setCurrentIndex(combo->findData(current));
            };
            reloadCombo();

            QPushButton *addBtn = new QPushButton("+", this);
            addBtn->setFixedWidth(35);
            addBtn->setToolTip("Создать новую связанную запись");
            connect(addBtn, &QPushButton::clicked, this, [this, refTable, reloadCombo]() {
                RecordDialog d(m_dbManager, refTable, this);
                if (d.exec() == QDialog::Accepted) reloadCombo();
            });

            hL->addWidget(combo);
            hL->addWidget(addBtn);
            m_fieldWidgets[col] = combo;
            form->addWidget(label, row, 0);
            form->addWidget(container, row, 1);
        } else {
            QLineEdit *field = new QLineEdit(this);

            // УТОЧНЕННАЯ ВАЛИДАЦИЯ: Блокируем буквы только там, где это 100% число
            QString lowCol = col.toLower();
            if (lowCol.contains("date")) {
                field->setInputMask("0000-00-00;_");
            } else if (lowCol == "years_of_service" || lowCol == "stazh_raboty" || lowCol == "category_index") {
                // Стаж и индексы - строго цифры
                QIntValidator *validator = new QIntValidator(0, 100, this);
                field->setValidator(validator);
                field->setPlaceholderText("0-100");
            } else if (lowCol.endsWith("_id") || lowCol == "id") {
                // ID - тоже только цифры
                QIntValidator *validator = new QIntValidator(0, 2147483647, this);
                field->setValidator(validator);
            }
            // Поля типа "phone_number", "passport_number", "address" ОСТАЮТСЯ СВОБОДНЫМИ
            // В них можно вводить и +, и пробелы, и буквы.

            // ЖЕСТКАЯ БЛОКИРОВКА ID: Чтобы пользователь не мог менять его вручную (ЛР требование)
            if (col.compare(pk, Qt::CaseInsensitive) == 0 || col.toLower() == "id") {
                field->setReadOnly(true);
                field->setEnabled(false);
                field->setText(!m_recordId.isEmpty() ? m_recordId : "АВТО-ID");
                field->setStyleSheet("background-color: #e0e0e0; border: 1px solid #999; color: #555; font-weight: bold;");
            }
            m_fieldWidgets[col] = field;
            form->addWidget(label, row, 0);
            form->addWidget(field, row, 1);
        }
        row++;
    }

    m_layout->addLayout(form);
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &RecordDialog::saveRecord);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    m_layout->addWidget(m_buttonBox);
}

void RecordDialog::saveRecord()
{
    QJsonObject json;
    QString pk = m_dbManager->getPrimaryKeyColumn(m_tableName);

    // Дополнительная проверка перед сохранением
    foreach (const QString &col, m_columns) {
        if (!m_fieldWidgets.contains(col)) continue;

        if (QLineEdit *edit = qobject_cast<QLineEdit*>(m_fieldWidgets[col])) {
            QString val = edit->text().trimmed();
            QString lowCol = col.toLower();

            // Если поле должно быть числовым, а там пусто (и это не PK) - можно выдать предупреждение
            if (edit->validator() && val.isEmpty() && !edit->isReadOnly()) {
                QMessageBox::warning(this, "Валидация", QString("Поле '%1' должно содержать число!").arg(getDisplayName(col)));
                edit->setFocus();
                return;
            }
        }
    }

    foreach (const QString &col, m_columns) {
        // ID берем из m_recordId или если пусто - ставим "АВТО-ID"
        if (col.compare(pk, Qt::CaseInsensitive) == 0 || col.toLower() == "id") {
            json[col] = !m_recordId.isEmpty() ? m_recordId : "АВТО-ID";
            continue;
        }

        if (QComboBox *combo = qobject_cast<QComboBox*>(m_fieldWidgets[col])) {
            json[col] = combo->currentData().toString();
        } else if (QLineEdit *edit = qobject_cast<QLineEdit*>(m_fieldWidgets[col])) {
            QString val = edit->text().trimmed();
            // Не отправляем пустые даты
            if (col.toLower().contains("date") && (val == "0000-00-00" || val.isEmpty())) continue;

            // Если мы редактируем, и это поле PK - обязательно включаем его в JSON
            if (!m_recordId.isEmpty() && col.compare(pk, Qt::CaseInsensitive) == 0) {
                json[col] = m_recordId;
            } else {
                json[col] = val;
            }
        }
    }

    bool ok = (m_recordId.isEmpty()) ? !m_dbManager->addRecordHttp(m_tableName, json).isEmpty()
                              : m_dbManager->updateRecordHttp(m_tableName, m_recordId, json);

    if (ok) accept();
    else {
        QString err = m_dbManager->lastError();
        if (err.isEmpty()) err = "Сервер не ответил или вернул ошибку.";
        QMessageBox::critical(this, "Ошибка сохранения", err);
    }
}

RecordDialog::~RecordDialog() {}
void RecordDialog::setupAutocomplete() { /* ... аналогично ... */ }
void RecordDialog::loadRecordData() {
    QString pk = m_dbManager->getPrimaryKeyColumn(m_tableName);

    // 1. Пытаемся получить данные по точному ID
    // Используем фильтр, который гарантированно понимает сервер (pk = 'id')
    QJsonArray arr = m_dbManager->fetchTableDataHttp(m_tableName, QString("%1 = '%2'").arg(pk, m_recordId));

    // 2. Если не нашли, пробуем по универсальному "id" (иногда в NoSQL ключи плавают)
    if (arr.isEmpty() && pk.toLower() != "id") {
        arr = m_dbManager->fetchTableDataHttp(m_tableName, QString("id = '%1'").arg(m_recordId));
    }

    // 3. Крайний случай: если совсем пусто, пробуем запросить всё и найти вручную в массиве
    if (arr.isEmpty()) {
        QJsonArray all = m_dbManager->fetchTableDataHttp(m_tableName, "");
        for(int i=0; i<all.size(); ++i) {
            QJsonObject o = all[i].toObject();
            if (o[pk].toVariant().toString() == m_recordId || o["id"].toVariant().toString() == m_recordId) {
                arr.append(o);
                break;
            }
        }
    }

    if (arr.isEmpty()) {
        qWarning() << "[RecordDialog] Record not found:" << m_tableName << "ID:" << m_recordId;
        QMessageBox::warning(this, "Предупреждение",
            "Не удалось загрузить данные записи. Поля будут пустыми.\nВозможно, запись была удалена или сервер недоступен.");
        return;
    }

    QJsonObject data = arr[0].toObject();

    // Создаем карту существующих ключей в нижнем регистре для сопоставления с колонками
    QMap<QString, QString> dataKeysMap;
    for (const QString &key : data.keys()) {
        dataKeysMap[key.toLower()] = key;
    }

    foreach (const QString &col, m_columns) {
        if (!m_fieldWidgets.contains(col)) continue;

        // Ищем значение в данных (Case-Insensitive match)
        QString actualKey = dataKeysMap.value(col.toLower());
        QString val;
        if (!actualKey.isEmpty()) {
            val = data[actualKey].toVariant().toString();
        }

        if (QComboBox *combo = qobject_cast<QComboBox*>(m_fieldWidgets[col])) {
            int idx = combo->findData(val);
            if (idx >= 0) combo->setCurrentIndex(idx);
            else if (!val.isEmpty()) {
                // Если значения нет в списке, добавляем его временно
                combo->addItem(val + " (Текущее)", val);
                combo->setCurrentIndex(combo->count() - 1);
            }
        } else if (QLineEdit *edit = qobject_cast<QLineEdit*>(m_fieldWidgets[col])) {
            edit->setText(val);
        }
    }
}
void RecordDialog::setupStyles() { setStyleSheet("QDialog { background-color: #ffffff; } QLabel { color: #000000; font-weight: bold; font-size: 13px; } QLineEdit, QComboBox { background-color: #ffffff; color: #000000; border: 2px solid #2f3640; padding: 6px; } QPushButton { background-color: #2f3640; color: #ffffff; font-weight: bold; padding: 8px; border-radius: 4px; }"); }
void RecordDialog::keyPressEvent(QKeyEvent *e) { if(e->key()==Qt::Key_Escape) reject(); else if(e->key()==Qt::Key_Return || e->key()==Qt::Key_Enter) { if(qobject_cast<QComboBox*>(focusWidget())) return; saveRecord(); } else QDialog::keyPressEvent(e); }
QString RecordDialog::getDisplayName(const QString &f) const { return m_fieldDisplayNames.value(f, f); }
