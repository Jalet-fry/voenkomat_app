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
#include <QScrollArea>

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
        setFixedSize(550, 550);
    } else {
        setMinimumSize(500, 450);
    }

    m_columns = m_dbManager->getColumnList(tableName);

    // Локализация имен полей
    m_fieldDisplayNames["conscript_id"] = "ID призывника";
    m_fieldDisplayNames["id_prizivnik"] = "ID призывника";
    m_fieldDisplayNames["full_name"] = "ФИО";
    m_fieldDisplayNames["fio"] = "ФИО";
    m_fieldDisplayNames["passport_number"] = "Паспорт";
    m_fieldDisplayNames["nomer_pasporta"] = "Паспорт";
    m_fieldDisplayNames["birth_date"] = "Дата рождения";
    m_fieldDisplayNames["data_rozhdeniya"] = "Дата рождения";
    m_fieldDisplayNames["residence_address"] = "Адрес";
    m_fieldDisplayNames["adres_prozhivaniya"] = "Адрес";
    m_fieldDisplayNames["category_name"] = "Категория";
    m_fieldDisplayNames["nazvanie_kategorii"] = "Категория";
    m_fieldDisplayNames["restriction_description"] = "Ограничения";
    m_fieldDisplayNames["opisanie_ogranichenii"] = "Ограничения";

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
    m_layout->setSpacing(10);
    m_layout->setContentsMargins(20, 20, 20, 20);

    QWidget *container = new QWidget(this);
    QGridLayout *form = new QGridLayout(container);
    QString pk = m_dbManager->getPrimaryKeyColumn(m_tableName);

    int row = 0;
    foreach (const QString &col, m_columns) {
        QLabel *label = new QLabel(getDisplayName(col) + ":", this);
        label->setStyleSheet("font-weight: bold; color: #2c3e50;");

        QLineEdit *field = new QLineEdit(this);
        field->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #ced4da; border-radius: 4px; background: white; color: black; }");
        
        if (col.toLower().contains("id") || col == pk) {
            if (m_recordId >= 0) {
                field->setReadOnly(true);
                field->setStyleSheet("background-color: #e9ecef; color: #495057; border: 1px solid #ced4da; padding: 6px;");
            } else {
                field->setPlaceholderText("Авто");
            }
        }

        m_fields[col] = field;
        form->addWidget(label, row, 0);
        form->addWidget(field, row, 1);
        row++;
    }

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(container);
    scrollArea->setFrameShape(QFrame::NoFrame);
    m_layout->addWidget(scrollArea);
}

void RecordDialog::setupClassicUI()
{
    m_layout->setContentsMargins(10, 10, 10, 10);
    m_layout->setSpacing(5);

    QWidget *container = new QWidget(this);
    QGridLayout *form = new QGridLayout(container);
    QString pk = m_dbManager->getPrimaryKeyColumn(m_tableName);

    int row = 0;
    foreach (const QString &col, m_columns) {
        QLabel *label = new QLabel(getDisplayName(col) + ":", this);
        QLineEdit *field = new QLineEdit(this);

        if (col.toLower().contains("id") || col == pk) {
            if (m_recordId >= 0) {
                field->setReadOnly(true);
                field->setStyleSheet("background-color: #d0d0d0; color: #505050;");
            }
        }

        m_fields[col] = field;
        form->addWidget(label, row, 0);
        form->addWidget(field, row, 1);
        row++;
    }

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(container);
    m_layout->addWidget(scrollArea);

    QLabel *hint = new QLabel(" [Enter] Сохранить | [Esc] Отмена ", this);
    hint->setStyleSheet("background-color: #000080; color: white; font-family: 'Consolas'; font-size: 11px; padding: 2px;");
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
        QString filter = QString("%1 = %2").arg(pk).arg(m_recordId);
        QJsonArray arr = m_dbManager->fetchTableDataHttp(m_tableName, filter);
        if (!arr.isEmpty()) data = arr[0].toObject();
    } else {
        QSqlQuery q = m_dbManager->executeQuery(QString("SELECT * FROM public.%1 WHERE %2 = %3").arg(m_tableName).arg(pk).arg(m_recordId));
        if (q.next()) {
            QSqlRecord rec = q.record();
            for(int i=0; i<rec.count(); ++i) data[rec.fieldName(i)] = QJsonValue::fromVariant(q.value(i));
        }
    }

    foreach (const QString &col, m_columns) {
        if (m_fields.contains(col)) {
            QVariant val = data[col].toVariant();
            if (val.userType() == QMetaType::QDate) {
                m_fields[col]->setText(val.toDate().toString("yyyy-MM-dd"));
            } else if (val.userType() == QMetaType::QDateTime) {
                m_fields[col]->setText(val.toDateTime().toString("yyyy-MM-dd HH:mm:ss"));
            } else {
                m_fields[col]->setText(val.toString());
            }
        }
    }
}

void RecordDialog::saveRecord()
{
    QJsonObject json;
    QString pk = m_dbManager->getPrimaryKeyColumn(m_tableName);

    foreach (const QString &col, m_columns) {
        QString val = m_fields[col]->text().trimmed();
        // При добавлении новой записи пропускаем пустой ID, если он автоинкрементный
        if (m_recordId < 0 && col == pk && val.isEmpty()) continue;
        json[col] = val;
    }

    bool ok = false;
    if (m_dbManager->isHttpMode()) {
        ok = (m_recordId < 0) ? m_dbManager->addRecordHttp(m_tableName, json) : m_dbManager->updateRecordHttp(m_tableName, m_recordId, json);
    } else {
        QStringList ks = json.keys();
        QStringList vs;
        foreach(const QString &k, ks) {
            QString v = json[k].toString().replace("'", "''");
            if (v.isEmpty()) vs << "NULL";
            else vs << QString("'%1'").arg(v);
        }

        QString sql;
        if (m_recordId < 0) {
            sql = QString("INSERT INTO public.%1 (%2) VALUES (%3)").arg(m_tableName).arg(ks.join(",")).arg(vs.join(","));
        } else {
            QStringList set;
            foreach(const QString &k, ks) {
                if(k != pk) {
                    QString v = json[k].toString().replace("'", "''");
                    set << QString("%1 = %2").arg(k).arg(v.isEmpty() ? "NULL" : QString("'%1'").arg(v));
                }
            }
            sql = QString("UPDATE public.%1 SET %2 WHERE %3 = %4").arg(m_tableName).arg(set.join(",")).arg(pk).arg(m_recordId);
        }
        m_dbManager->executeQuery(sql, &ok);
    }

    if (ok) accept();
    else QMessageBox::critical(this, "Ошибка", "Не удалось сохранить запись. " + m_dbManager->lastError());
}

void RecordDialog::setupStyles()
{
    if (m_isClassicUI) {
        setStyleSheet("QDialog { background-color: #c0c0c0; color: black; }"
                      "QLineEdit { background-color: white; border: 2px inset gray; color: black; font-family: 'Consolas'; }"
                      "QLabel { color: black; font-family: 'Consolas'; }");
    } else {
        setStyleSheet("QDialog { background-color: #f8f9fa; }"
                      "QPushButton { background-color: #2c3e50; color: white; border-radius: 4px; padding: 6px 15px; }");
    }
}

QString RecordDialog::getDisplayName(const QString &f) const { return m_fieldDisplayNames.value(f, f); }
