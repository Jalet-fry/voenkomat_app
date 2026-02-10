#include "RecordDialog.h"
#include "DbConstants.h"
#include <QPushButton>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QDate>
#include <QDateTime>
#include <QRegExp>

RecordDialog::RecordDialog(DatabaseManager *dbManager, const QString &tableName, QWidget *parent, int recordId)
    : QDialog(parent)
    , m_dbManager(dbManager)
    , m_tableName(tableName)
    , m_recordId(recordId)
{
    setWindowTitle(recordId < 0 ? "Добавить запись" : "Изменить запись");
    setMinimumSize(400, 300);

    // Получаем список столбцов
    m_columns = m_dbManager->getColumnList(tableName);
    
    // Получаем детальную информацию о колонках
    m_columnDetails = m_dbManager->getColumnDetails(tableName);
    
    // Получаем информацию о внешних ключах
    m_foreignKeys = m_dbManager->getForeignKeyInfo(tableName);

    using namespace Db;

    // Инициализация русских названий полей
    // Conscripts
    m_fieldDisplayNames[Conscripts::CONSCRIPT_ID] = "ID призывника";
    m_fieldDisplayNames[Conscripts::FULL_NAME] = "ФИО";
    m_fieldDisplayNames[Conscripts::BIRTH_DATE] = "Дата рождения";
    m_fieldDisplayNames[Conscripts::RESIDENCE_ADDRESS] = "Адрес проживания";
    m_fieldDisplayNames[Conscripts::PASSPORT_NUMBER] = "Номер паспорта";
    m_fieldDisplayNames[Conscripts::MILITARY_TICKET_ID] = "ID военного билета";
    m_fieldDisplayNames[Conscripts::REGISTRATION_CARD_ID] = "ID учётной карты";

    // Commissioners
    m_fieldDisplayNames[Commissioners::COMMISSIONER_ID] = "ID комиссара";
    m_fieldDisplayNames[Commissioners::FULL_NAME] = "ФИО комиссара";
    m_fieldDisplayNames[Commissioners::POSITION] = "Должность";
    m_fieldDisplayNames[Commissioners::YEARS_OF_SERVICE] = "Стаж работы";
    m_fieldDisplayNames[Commissioners::PHONE_NUMBER] = "Контактный телефон";

    // Fitness Categories
    m_fieldDisplayNames[FitnessCategories::CATEGORY_ID] = "ID категории";
    m_fieldDisplayNames[FitnessCategories::CATEGORY_NAME] = "Название категории";
    m_fieldDisplayNames[FitnessCategories::RESTRICTION_DESCRIPTION] = "Описание ограничений";
    m_fieldDisplayNames[FitnessCategories::CATEGORY_INDEX] = "Индекс категории";
    m_fieldDisplayNames[FitnessCategories::CATEGORY_BASIS] = "Основание для категории";

    // Medical Examinations
    m_fieldDisplayNames[MedicalExaminations::CERTIFICATION_ID] = "ID освидетельствования";
    m_fieldDisplayNames[MedicalExaminations::EXAMINATION_DATE] = "Дата проведения";
    m_fieldDisplayNames[MedicalExaminations::EXAMINATION_RESULTS] = "Результаты обследования";
    m_fieldDisplayNames[MedicalExaminations::DOCTOR_FULL_NAME] = "ФИО врача";
    m_fieldDisplayNames[MedicalExaminations::CONCLUSION] = "Заключение";

    // Military Id Cards
    m_fieldDisplayNames[MilitaryIdCards::TICKET_ID] = "ID билета";
    m_fieldDisplayNames[MilitaryIdCards::TICKET_NUMBER] = "Номер билета";
    m_fieldDisplayNames[MilitaryIdCards::ISSUE_DATE] = "Дата выдачи";
    m_fieldDisplayNames[MilitaryIdCards::MILITARY_RANK] = "Воинское звание";
    m_fieldDisplayNames[MilitaryIdCards::CATEGORY] = "Категория";

    // Service Record Cards
    m_fieldDisplayNames[ServiceRecordCards::CARD_ID] = "ID карты";
    m_fieldDisplayNames[ServiceRecordCards::CARD_NUMBER] = "Номер карты";
    m_fieldDisplayNames[ServiceRecordCards::REGISTRATION_DATE] = "Дата постановки на учёт";
    m_fieldDisplayNames[ServiceRecordCards::DEFERMENT_HISTORY] = "История отсрочек";
    m_fieldDisplayNames[ServiceRecordCards::MILITARY_SPECIALTY] = "Военно-учётная специальность";

    // Callup Events
    m_fieldDisplayNames[CallupEvents::EVENT_ID] = "ID мероприятия";
    m_fieldDisplayNames[CallupEvents::EVENT_TYPE] = "Тип мероприятия";
    m_fieldDisplayNames[CallupEvents::EVENT_DATETIME] = "Дата и время";
    m_fieldDisplayNames[CallupEvents::EVENT_LOCATION] = "Место проведения";
    m_fieldDisplayNames[CallupEvents::COMMISSIONER_FULL_NAME] = "ФИО комиссара";

    // Common/Relations
    m_fieldDisplayNames["id_prizivnika"] = "ID призывника";
    m_fieldDisplayNames[ConscriptsCommissioners::INTERACTION_DATE] = "Дата взаимодействия";
    m_fieldDisplayNames[ConscriptsCommissioners::OFFICE_NUMBER] = "Номер кабинета";

    setupUI();
    if (m_recordId >= 0) {
        loadRecordData();
    }
}

RecordDialog::~RecordDialog()
{
}

void RecordDialog::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(10);
    layout->setContentsMargins(20, 20, 20, 20);

    QGridLayout *formLayout = new QGridLayout();

    int row = 0;
    foreach (const QString &col, m_columns) {
        QString displayName = getDisplayName(col);
        QLabel *label = new QLabel(displayName + ":", this);
        formLayout->addWidget(label, row, 0);

        QLineEdit *field = new QLineEdit(this);
        field->setStyleSheet("QLineEdit { padding: 5px; border: 2px solid #FFB6C1; border-radius: 5px; }");
        
        // Находим детальную информацию о колонке
        DatabaseManager::ColumnDetail colDetail;
        foreach (const DatabaseManager::ColumnDetail &detail, m_columnDetails) {
            if (detail.columnName == col) {
                colDetail = detail;
                break;
            }
        }
        
        // Проверяем, является ли колонка auto-increment (SERIAL)
        QString defaultStr = colDetail.defaultValue.toString();
        bool isAutoIncrement = !defaultStr.isEmpty() && defaultStr.contains("nextval", Qt::CaseInsensitive);
        
        // Если это первичный ключ с auto-increment, делаем поле только для чтения
        QString primaryKeyColumn = m_dbManager->getPrimaryKeyColumn(m_tableName);
        if (col == primaryKeyColumn) {
            if (isAutoIncrement) {
                field->setReadOnly(true);
                if (m_recordId < 0) {
                    field->setPlaceholderText("Автоматически");
                } else {
                    field->setPlaceholderText("ID нельзя изменить");
                }
            } else {
                if (m_recordId < 0) {
                    field->setPlaceholderText("Введите значение");
                } else {
                    field->setReadOnly(true);
                    field->setPlaceholderText("ID нельзя изменить");
                }
            }
        } else if (isAutoIncrement && m_recordId < 0) {
            field->setReadOnly(true);
            field->setPlaceholderText("Автоматически");
        }
        
        if (colDetail.dataType == "date") {
            field->setPlaceholderText("YYYY-MM-DD");
        } else if (colDetail.dataType == "timestamp" || colDetail.dataType == "timestamp without time zone") {
            field->setPlaceholderText("YYYY-MM-DD HH:MM:SS");
        }
        
        m_fields[col] = field;
        formLayout->addWidget(field, row, 1);
        row++;
    }

    layout->addLayout(formLayout);
    layout->addStretch();

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &RecordDialog::saveRecord);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox);
}

QString RecordDialog::getDisplayName(const QString &fieldName) const
{
    return m_fieldDisplayNames.value(fieldName, fieldName);
}

void RecordDialog::loadRecordData()
{
    if (m_recordId < 0) return;

    QString idColumn = m_dbManager->getPrimaryKeyColumn(m_tableName);
    if (idColumn.isEmpty()) return;

    QSqlQuery query = m_dbManager->prepareQuery(
        QString("SELECT * FROM %1 WHERE %2 = :id_value")
        .arg(DatabaseManager::escapeIdentifier(m_tableName))
        .arg(DatabaseManager::escapeIdentifier(idColumn))
    );
    query.bindValue(":id_value", m_recordId);
    
    if (m_dbManager->executePreparedQuery(query) && query.next()) {
        QStringList columns = m_dbManager->getColumnList(m_tableName);
        for (int i = 0; i < columns.size(); ++i) {
            QString col = columns[i];
            QVariant value = query.value(i);
            if (m_fields.contains(col)) {
                if (value.isNull()) {
                    m_fields[col]->setText("");
                } else {
                    DatabaseManager::ColumnDetail colDetail;
                    foreach (const DatabaseManager::ColumnDetail &detail, m_columnDetails) {
                        if (detail.columnName == col) {
                            colDetail = detail;
                            break;
                        }
                    }
                    if (colDetail.dataType == "date") {
                        m_fields[col]->setText(value.toDate().toString("yyyy-MM-dd"));
                    } else if (colDetail.dataType.contains("timestamp")) {
                        m_fields[col]->setText(value.toDateTime().toString("yyyy-MM-dd hh:mm:ss"));
                    } else {
                        m_fields[col]->setText(value.toString());
                    }
                }
            }
        }
    }
}

bool RecordDialog::isValidDate(const QString &dateStr)
{
    if (dateStr.isEmpty()) return true;
    QDate date = QDate::fromString(dateStr, "yyyy-MM-dd");
    return date.isValid();
}

bool RecordDialog::isValidTimestamp(const QString &timestampStr)
{
    if (timestampStr.isEmpty()) return true;
    QDateTime datetime = QDateTime::fromString(timestampStr, "yyyy-MM-dd hh:mm:ss");
    return datetime.isValid();
}

bool RecordDialog::isValidInteger(const QString &value)
{
    if (value.isEmpty()) return true;
    bool ok;
    value.toInt(&ok);
    return ok;
}

QVariant RecordDialog::formatValueForSQL(const QString &columnName, const QString &value, const QString &dataType)
{
    Q_UNUSED(columnName);
    QString trimmedValue = value.trimmed();
    if (trimmedValue.isEmpty()) return QVariant();
    
    if (dataType.contains("int")) {
        return trimmedValue.toInt();
    } else if (dataType.contains("numeric") || dataType.contains("real") || dataType.contains("double")) {
        return trimmedValue.toDouble();
    } else if (dataType == "boolean") {
        return trimmedValue.toLower() == "true" || trimmedValue == "1";
    } else if (dataType == "date") {
        return QDate::fromString(trimmedValue, "yyyy-MM-dd");
    } else if (dataType.contains("timestamp")) {
        return QDateTime::fromString(trimmedValue, "yyyy-MM-dd hh:mm:ss");
    }
    return trimmedValue;
}

bool RecordDialog::validateRecord(QString &errorMessage)
{
    foreach (const DatabaseManager::ColumnDetail &detail, m_columnDetails) {
        if (!m_fields.contains(detail.columnName)) continue;
        QString value = m_fields[detail.columnName]->text().trimmed();
        
        if (!detail.isNullable && value.isEmpty() && detail.defaultValue.isNull()) {
            errorMessage = QString("Поле '%1' обязательно").arg(getDisplayName(detail.columnName));
            return false;
        }
        
        if (!value.isEmpty()) {
            if (detail.dataType == "date" && !isValidDate(value)) {
                errorMessage = QString("Ошибка в поле '%1' (YYYY-MM-DD)").arg(getDisplayName(detail.columnName));
                return false;
            }
            if (detail.dataType.contains("timestamp") && !isValidTimestamp(value)) {
                errorMessage = QString("Ошибка в поле '%1' (YYYY-MM-DD HH:MM:SS)").arg(getDisplayName(detail.columnName));
                return false;
            }
            if (detail.dataType.contains("int") && !isValidInteger(value)) {
                errorMessage = QString("Ошибка в поле '%1' (целое число)").arg(getDisplayName(detail.columnName));
                return false;
            }
        }
    }
    return true;
}

void RecordDialog::saveRecord()
{
    QString error;
    if (!validateRecord(error)) {
        QMessageBox::warning(this, "Ошибка", error);
        return;
    }
    
    QString idColumn = m_dbManager->getPrimaryKeyColumn(m_tableName);
    if (!m_dbManager->beginTransaction()) return;
    
    bool success = false;
    if (m_recordId < 0) {
        QStringList cols, placeholders;
        foreach (const auto &detail, m_columnDetails) {
            QString def = detail.defaultValue.toString();
            if (detail.columnName == idColumn && def.contains("nextval")) continue;
            if (def.contains("nextval")) continue;
            cols << DatabaseManager::escapeIdentifier(detail.columnName);
            placeholders << QString(":%1").arg(detail.columnName);
        }

        QSqlQuery query = m_dbManager->prepareQuery(
            QString("INSERT INTO %1 (%2) VALUES (%3)")
            .arg(DatabaseManager::escapeIdentifier(m_tableName))
            .arg(cols.join(","))
            .arg(placeholders.join(","))
        );
        
        foreach (const auto &detail, m_columnDetails) {
            QString def = detail.defaultValue.toString();
            if (detail.columnName == idColumn && def.contains("nextval")) continue;
            if (def.contains("nextval")) continue;
            query.bindValue(":" + detail.columnName, formatValueForSQL(detail.columnName, m_fields[detail.columnName]->text(), detail.dataType));
        }
        success = m_dbManager->executePreparedQuery(query);
    } else {
        QStringList set;
        foreach (const auto &detail, m_columnDetails) {
            if (detail.columnName == idColumn) continue;
            set << QString("%1 = :%2").arg(DatabaseManager::escapeIdentifier(detail.columnName)).arg(detail.columnName);
        }

        QSqlQuery query = m_dbManager->prepareQuery(
            QString("UPDATE %1 SET %2 WHERE %3 = :id_val")
            .arg(DatabaseManager::escapeIdentifier(m_tableName))
            .arg(set.join(","))
            .arg(DatabaseManager::escapeIdentifier(idColumn))
        );
        
        foreach (const auto &detail, m_columnDetails) {
            if (detail.columnName == idColumn) continue;
            query.bindValue(":" + detail.columnName, formatValueForSQL(detail.columnName, m_fields[detail.columnName]->text(), detail.dataType));
        }
        query.bindValue(":id_val", m_recordId);
        success = m_dbManager->executePreparedQuery(query);
    }
    
    if (success && m_dbManager->commitTransaction()) accept();
    else m_dbManager->rollbackTransaction();
}

