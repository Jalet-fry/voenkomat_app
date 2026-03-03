#include "RecordDialog.h"
#include "DbConstants.h"
#include <QPushButton>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDate>
#include <QDateTime>
#include <QRegularExpression>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>

RecordDialog::RecordDialog(DatabaseManager *dbManager, const QString &tableName, QWidget *parent, int recordId)
    : QDialog(parent)
    , m_dbManager(dbManager)
    , m_tableName(tableName)
    , m_recordId(recordId)
{
    setWindowTitle(recordId < 0 ? "Добавление записи" : "Редактирование записи");
    setMinimumSize(500, 450);

    m_columns = m_dbManager->getColumnList(tableName);
    m_columnDetails = m_dbManager->getColumnDetails(tableName);
    m_foreignKeys = m_dbManager->getForeignKeyInfo(tableName);

    // Маппинг для подсказок
    m_fieldDisplayNames["conscript_id"] = "ID призывника";
    m_fieldDisplayNames["full_name"] = "ФИО";
    m_fieldDisplayNames["passport_number"] = "Серия и номер паспорта";
    m_fieldDisplayNames["military_ticket_id"] = "ID военного билета";
    m_fieldDisplayNames["registration_card_id"] = "ID учетной карты";

    setupUI();
    if (m_recordId >= 0) {
        loadRecordData();
    }
}

RecordDialog::~RecordDialog() {}

void RecordDialog::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(15);
    layout->setContentsMargins(25, 25, 25, 25);

    QGridLayout *formLayout = new QGridLayout();
    QString pkCol = m_dbManager->getPrimaryKeyColumn(m_tableName);

    int row = 0;
    foreach (const QString &col, m_columns) {
        QString displayName = m_fieldDisplayNames.value(col, col);
        QLabel *label = new QLabel(displayName + ":", this);
        label->setStyleSheet("font-weight: bold; color: #2c3e50;");

        QLineEdit *field = new QLineEdit(this);
        // ЖЕСТКИЙ СТИЛЬ ДЛЯ ВИДИМОСТИ ТЕКСТА
        field->setStyleSheet("QLineEdit { padding: 8px; border: 1px solid #bdc3c7; border-radius: 4px; background: white; color: black; font-size: 13px; }");
        
        // 1. ПРОВЕРКА НА ПЕРВИЧНЫЙ КЛЮЧ
        if (col == pkCol) {
            if (m_recordId >= 0) {
                field->setReadOnly(true);
                field->setStyleSheet("background-color: #f1f2f6; color: #7f8c8d; border: 1px solid #dcdde1;");
                label->setText(label->text() + " [Заблокировано]");
            } else {
                field->setPlaceholderText("Автоматически (Serial)");
            }
        }

        // 2. ПРОВЕРКА НА ВНЕШНИЙ КЛЮЧ (Foreign Key)
        bool isFK = false;
        QString refTable;
        foreach(const auto &fk, m_foreignKeys) {
            if (fk.columnName == col) {
                isFK = true; refTable = fk.referencedTable; break;
            }
        }
        if (isFK) {
            label->setStyleSheet("font-weight: bold; color: #0984e3;"); // Синий цвет для связей
            field->setToolTip("Это поле связано с таблицей " + refTable + ". Значение должно там существовать.");
            field->setPlaceholderText("Введите существующий ID из " + refTable);
        }

        // 3. ПРОВЕРКА НА УНИКАЛЬНОСТЬ (Эвристика)
        if (col.contains("passport") || col.contains("ticket_number") || col.contains("phone")) {
            label->setStyleSheet("font-weight: bold; color: #d35400;"); // Оранжевый для уникальных
            field->setToolTip("Данные в этом поле не должны повторяться в базе.");
        }

        m_fields[col] = field;
        formLayout->addWidget(label, row, 0);
        formLayout->addWidget(field, row, 1);
        row++;
    }

    layout->addLayout(formLayout);

    // Легенда внизу
    QLabel *legend = new QLabel(
        "<span style='color: #0984e3;'>■</span> — Связь с другой таблицей (FK)<br>"
        "<span style='color: #d35400;'>■</span> — Уникальное поле (Unique)", this);
    legend->setStyleSheet("font-size: 11px; color: #636e72;");
    layout->addWidget(legend);

    layout->addStretch();

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    buttonBox->setStyleSheet("QPushButton { min-width: 100px; padding: 8px; }");
    connect(buttonBox, &QDialogButtonBox::accepted, this, &RecordDialog::saveRecord);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox);

    setStyleSheet("QDialog { background-color: #dbffff; }");
}

void RecordDialog::loadRecordData()
{
    if (m_recordId < 0) return;
    QString pkCol = m_dbManager->getPrimaryKeyColumn(m_tableName);

    QJsonObject data;
    if (m_dbManager->isHttpMode()) {
        QJsonArray arr = m_dbManager->fetchTableDataHttp(m_tableName, QString("%1 = %2").arg(pkCol).arg(m_recordId));
        if (!arr.isEmpty()) data = arr[0].toObject();
    } else {
        QSqlQuery query = m_dbManager->executeQuery(QString("SELECT * FROM %1 WHERE %2 = %3").arg(m_tableName).arg(pkCol).arg(m_recordId));
        if (query.next()) {
            QSqlRecord rec = query.record();
            for(int i=0; i<rec.count(); ++i) data[rec.fieldName(i)] = QJsonValue::fromVariant(query.value(i));
        }
    }

    foreach (const QString &col, m_columns) {
        if (m_fields.contains(col)) {
            m_fields[col]->setText(data[col].toVariant().toString());
        }
    }
}

void RecordDialog::saveRecord()
{
    QJsonObject jsonObj;
    QString pkCol = m_dbManager->getPrimaryKeyColumn(m_tableName);

    foreach (const QString &col, m_columns) {
        QString val = m_fields[col]->text().trimmed();
        // Пропускаем PK только при создании, если он пустой
        if (m_recordId < 0 && col == pkCol && val.isEmpty()) continue;
        jsonObj[col] = val;
    }

    bool success = false;
    if (m_dbManager->isHttpMode()) {
        success = (m_recordId < 0)
            ? m_dbManager->addRecordHttp(m_tableName, jsonObj)
            : m_dbManager->updateRecordHttp(m_tableName, m_recordId, jsonObj);
    } else {
        QStringList cols = jsonObj.keys();
        QStringList vals;
        foreach(const QString &k, cols) vals << QString("'%1'").arg(jsonObj[k].toString().replace("'", "''"));

        QString sql;
        if (m_recordId < 0) {
            sql = QString("INSERT INTO %1 (%2) VALUES (%3)").arg(m_tableName).arg(cols.join(",")).arg(vals.join(","));
        } else {
            QStringList set;
            foreach(const QString &k, jsonObj.keys()) if(k != pkCol) set << QString("%1 = '%2'").arg(k).arg(jsonObj[k].toString().replace("'", "''"));
            sql = QString("UPDATE %1 SET %2 WHERE %3 = %4").arg(m_tableName).arg(set.join(",")).arg(pkCol).arg(m_recordId);
        }
        m_dbManager->executeQuery(sql, &success);
    }

    if (success) accept();
    else {
        QString err = m_dbManager->lastError();
        if (err.contains("foreign key", Qt::CaseInsensitive)) {
            err = "Ошибка: Вы указали ID, которого не существует в связанной таблице.\nПроверьте синие поля.";
        } else if (err.contains("duplicate key", Qt::CaseInsensitive)) {
            err = "Ошибка: Запись с такими уникальными данными уже существует.\nПроверьте оранжевые поля.";
        }
        QMessageBox::critical(this, "Ошибка сохранения", err);
    }
}

QString RecordDialog::getDisplayName(const QString &f) const { return m_fieldDisplayNames.value(f, f); }
bool RecordDialog::validateRecord(QString &) { return true; }
QVariant RecordDialog::formatValueForSQL(const QString &, const QString &, const QString &) { return QVariant(); }
bool RecordDialog::isValidDate(const QString &) { return true; }
bool RecordDialog::isValidInteger(const QString &) { return true; }
bool RecordDialog::isValidTimestamp(const QString &) { return true; }
