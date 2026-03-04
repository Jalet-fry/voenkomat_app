#include "RecordDialog.h"
#include "DbConstants.h"
#include "ConfigManager.h"
#include <QPushButton>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDate>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonValue>
#include <QKeyEvent>

RecordDialog::RecordDialog(DatabaseManager *dbManager, const QString &tableName, QWidget *parent, int recordId)
    : QDialog(parent)
    , m_dbManager(dbManager)
    , m_tableName(tableName)
    , m_recordId(recordId)
{
    ConfigManager config("config.ini");
    m_isClassicUI = config.isClassicUI();

    setWindowTitle(recordId < 0 ? "Добавление: " + tableName : "Правка: " + tableName);

    if (m_isClassicUI) {
        setFixedSize(500, 500);
    } else {
        setMinimumSize(500, 450);
    }

    m_columns = m_dbManager->getColumnList(tableName);
    m_foreignKeys = m_dbManager->getForeignKeyInfo(tableName);

    // Локализация имен полей
    m_fieldDisplayNames["conscript_id"] = "ID призывника";
    m_fieldDisplayNames["full_name"] = "ФИО";
    m_fieldDisplayNames["passport_number"] = "Паспорт";
    m_fieldDisplayNames["birth_date"] = "Дата рождения";
    m_fieldDisplayNames["residence_address"] = "Адрес";

    setupUI();
    setupStyles();

    if (m_recordId >= 0) {
        loadRecordData();
    }
}

RecordDialog::~RecordDialog() {}

void RecordDialog::setupUI()
{
    m_layout = new QVBoxLayout(this);
    if (m_isClassicUI) setupClassicUI(); else setupModernUI();

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &RecordDialog::saveRecord);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    m_layout->addWidget(m_buttonBox);
}

void RecordDialog::setupModernUI()
{
    m_layout->setSpacing(15);
    m_layout->setContentsMargins(25, 25, 25, 25);

    QGridLayout *form = new QGridLayout();
    QString pk = m_dbManager->getPrimaryKeyColumn(m_tableName);

    int row = 0;
    foreach (const QString &col, m_columns) {
        QLabel *label = new QLabel(getDisplayName(col) + ":", this);
        label->setStyleSheet("font-weight: bold; color: #2c3e50;");

        QLineEdit *field = new QLineEdit(this);
        field->setStyleSheet("QLineEdit { padding: 8px; border: 1px solid #bdc3c7; border-radius: 4px; background: white; color: black; }");
        
        if (col == pk) {
            if (m_recordId >= 0) {
                field->setReadOnly(true);
                field->setStyleSheet("background-color: #f1f2f6; color: #7f8c8d; border: 1px solid #bdc3c7; padding: 8px;");
            } else {
                field->setPlaceholderText("Автоматически");
            }
        }

        m_fields[col] = field;
        form->addWidget(label, row, 0);
        form->addWidget(field, row, 1);
        row++;
    }
    m_layout->addLayout(form);
    m_layout->addStretch();
}

void RecordDialog::setupClassicUI()
{
    m_layout->setContentsMargins(10, 10, 10, 10);
    m_layout->setSpacing(5);

    QGridLayout *form = new QGridLayout();
    QString pk = m_dbManager->getPrimaryKeyColumn(m_tableName);

    int row = 0;
    foreach (const QString &col, m_columns) {
        QLabel *label = new QLabel(col + ":", this);
        QLineEdit *field = new QLineEdit(this);

        if (col == pk && m_recordId >= 0) {
            field->setReadOnly(true);
            field->setStyleSheet("background-color: #d0d0d0;");
        }

        m_fields[col] = field;
        form->addWidget(label, row, 0);
        form->addWidget(field, row, 1);
        row++;
    }
    m_layout->addLayout(form);

    QLabel *hint = new QLabel(" [Enter] Сохранить | [Esc] Отмена ", this);
    hint->setStyleSheet("background-color: #000080; color: white; font-family: 'Consolas'; font-size: 11px;");
    m_layout->addWidget(hint);
}

void RecordDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) { reject(); return; }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        saveRecord();
        return;
    }
    QDialog::keyPressEvent(event);
}

void RecordDialog::loadRecordData()
{
    QString pk = m_dbManager->getPrimaryKeyColumn(m_tableName);
    QJsonObject data;
    if (m_dbManager->isHttpMode()) {
        QJsonArray arr = m_dbManager->fetchTableDataHttp(m_tableName, QString("%1 = %2").arg(pk).arg(m_recordId));
        if (!arr.isEmpty()) data = arr[0].toObject();
    } else {
        QSqlQuery q = m_dbManager->executeQuery(QString("SELECT * FROM %1 WHERE %2 = %3").arg(m_tableName).arg(pk).arg(m_recordId));
        if (q.next()) {
            QSqlRecord rec = q.record();
            for(int i=0; i<rec.count(); ++i) data[rec.fieldName(i)] = QJsonValue::fromVariant(q.value(i));
        }
    }

    foreach (const QString &col, m_columns) {
        if (m_fields.contains(col)) m_fields[col]->setText(data[col].toVariant().toString());
    }
}

void RecordDialog::saveRecord()
{
    QJsonObject json;
    QString pk = m_dbManager->getPrimaryKeyColumn(m_tableName);

    foreach (const QString &col, m_columns) {
        QString val = m_fields[col]->text().trimmed();
        if (m_recordId < 0 && col == pk && (val.isEmpty() || val == "0")) continue;
        json[col] = val;
    }

    bool ok = false;
    if (m_dbManager->isHttpMode()) {
        ok = (m_recordId < 0) ? m_dbManager->addRecordHttp(m_tableName, json) : m_dbManager->updateRecordHttp(m_tableName, m_recordId, json);
    } else {
        QStringList ks = json.keys();
        QStringList vs;
        foreach(const QString &k, ks) vs << QString("'%1'").arg(json[k].toString().replace("'", "''"));
        QString sql;
        if (m_recordId < 0) sql = QString("INSERT INTO %1 (%2) VALUES (%3)").arg(m_tableName).arg(ks.join(",")).arg(vs.join(","));
        else {
            QStringList set;
            foreach(const QString &k, ks) if(k != pk) set << QString("%1 = '%2'").arg(k).arg(json[k].toString().replace("'", "''"));
            sql = QString("UPDATE %1 SET %2 WHERE %3 = %4").arg(m_tableName).arg(set.join(",")).arg(pk).arg(m_recordId);
        }
        m_dbManager->executeQuery(sql, &ok);
    }

    if (ok) accept();
    else QMessageBox::critical(this, "Ошибка", m_dbManager->lastError().isEmpty() ? "Не удалось сохранить запись" : m_dbManager->lastError());
}

void RecordDialog::setupStyles()
{
    if (m_isClassicUI) {
        setStyleSheet("QDialog { background-color: #c0c0c0; color: black; }"
                      "QLineEdit { background-color: white; border: 2px inset gray; color: black; font-family: 'Consolas'; }"
                      "QLabel { color: black; }");
    } else {
        setStyleSheet("QDialog { background-color: #f0f3f5; }"
                      "QLabel { color: #2c3e50; }"
                      "QPushButton { background-color: #2c3e50; color: white; border-radius: 4px; padding: 6px 12px; }"
                      "QPushButton:hover { background-color: #34495e; }");
    }
}

QString RecordDialog::getDisplayName(const QString &f) const { return m_fieldDisplayNames.value(f, f); }
