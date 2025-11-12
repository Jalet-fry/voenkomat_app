#include "RecordDialog.h"
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

    // Инициализация русских названий полей
    m_fieldDisplayNames["id_prizivnik"] = "ID призывника";
    m_fieldDisplayNames["fio"] = "ФИО";
    m_fieldDisplayNames["data_rozhdeniya"] = "Дата рождения";
    m_fieldDisplayNames["adres_prozhivaniya"] = "Адрес проживания";
    m_fieldDisplayNames["nomer_pasporta"] = "Номер паспорта";
    m_fieldDisplayNames["id_comissar"] = "ID комиссара";
    m_fieldDisplayNames["dolzhnost"] = "Должность";
    m_fieldDisplayNames["stazh_raboty"] = "Стаж работы";
    m_fieldDisplayNames["kontaktnyi_telefon"] = "Контактный телефон";
    m_fieldDisplayNames["id_kategorii"] = "ID категории";
    m_fieldDisplayNames["nazvanie_kategorii"] = "Название категории";
    m_fieldDisplayNames["opisanie_ogranichenii"] = "Описание ограничений";
    m_fieldDisplayNames["index_kategorii"] = "Индекс категории";
    m_fieldDisplayNames["osnovanie_dlya_kategorii"] = "Основание для категории";
    m_fieldDisplayNames["id_osvidetelstvovania"] = "ID освидетельствования";
    m_fieldDisplayNames["data_provedeniya"] = "Дата проведения";
    m_fieldDisplayNames["rezultaty_obsledovania"] = "Результаты обследования";
    m_fieldDisplayNames["fio_vracha"] = "ФИО врача";
    m_fieldDisplayNames["zaklyuchenie"] = "Заключение";
    m_fieldDisplayNames["id_prizivnika"] = "ID призывника";
    m_fieldDisplayNames["id_bileta"] = "ID билета";
    m_fieldDisplayNames["nomer_bileta"] = "Номер билета";
    m_fieldDisplayNames["data_vydachi"] = "Дата выдачи";
    m_fieldDisplayNames["voinskoe_zvanie"] = "Воинское звание";
    m_fieldDisplayNames["kategoria"] = "Категория";
    m_fieldDisplayNames["id_karty"] = "ID карты";
    m_fieldDisplayNames["nomer_karty"] = "Номер карты";
    m_fieldDisplayNames["data_postanovki_na_uchet"] = "Дата постановки на учёт";
    m_fieldDisplayNames["istoriya_otsrochek"] = "История отсрочек";
    m_fieldDisplayNames["voenno_uchetnaya_specialnost"] = "Военно-учётная специальность";
    m_fieldDisplayNames["id_meropriyatiya"] = "ID мероприятия";
    m_fieldDisplayNames["tip_meropriyatiya"] = "Тип мероприятия";
    m_fieldDisplayNames["mesto_provedeniya"] = "Место проведения";
    m_fieldDisplayNames["fio_comissara"] = "ФИО комиссара";
    m_fieldDisplayNames["data_vzaimodeistviya"] = "Дата взаимодействия";
    m_fieldDisplayNames["nomer_kabineta"] = "Номер кабинета";

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
        
        // Если это первичный ключ и это добавление, делаем поле только для чтения
        QString primaryKeyColumn = m_dbManager->getPrimaryKeyColumn(m_tableName);
        if (col == primaryKeyColumn && m_recordId < 0) {
            field->setReadOnly(true);
            field->setPlaceholderText("Автоматически");
        }
        
        // Добавляем подсказку для дат
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
    if (m_recordId < 0) {
        return;
    }

    if (!m_dbManager || !m_dbManager->isConnected()) {
        QMessageBox::critical(this, "Ошибка", "База данных не подключена");
        return;
    }

    QString idColumn = m_dbManager->getPrimaryKeyColumn(m_tableName);
    if (idColumn.isEmpty()) {
        QMessageBox::critical(this, "Ошибка", "Не удалось определить первичный ключ таблицы");
        return;
    }

    // Используем prepared statement для безопасности
    QSqlQuery query = m_dbManager->prepareQuery(
        QString("SELECT * FROM %1 WHERE %2 = :id_value").arg(m_tableName).arg(idColumn)
    );
    query.bindValue(":id_value", m_recordId);
    
    bool ok = m_dbManager->executePreparedQuery(query);

    if (ok && query.next()) {
        QStringList columns = m_dbManager->getColumnList(m_tableName);
        for (int i = 0; i < columns.size(); ++i) {
            QString col = columns[i];
            QVariant value = query.value(i);
            if (m_fields.contains(col)) {
                // Форматируем значение в зависимости от типа
                if (value.isNull()) {
                    m_fields[col]->setText("");
                } else {
                    // Для дат и времени используем специальное форматирование
                    DatabaseManager::ColumnDetail colDetail;
                    foreach (const DatabaseManager::ColumnDetail &detail, m_columnDetails) {
                        if (detail.columnName == col) {
                            colDetail = detail;
                            break;
                        }
                    }
                    
                    if (colDetail.dataType == "date") {
                        QDate date = value.toDate();
                        if (date.isValid()) {
                            m_fields[col]->setText(date.toString("yyyy-MM-dd"));
                        } else {
                            m_fields[col]->setText(value.toString());
                        }
                    } else if (colDetail.dataType == "timestamp" || colDetail.dataType == "timestamp without time zone") {
                        QDateTime datetime = value.toDateTime();
                        if (datetime.isValid()) {
                            m_fields[col]->setText(datetime.toString("yyyy-MM-dd hh:mm:ss"));
                        } else {
                            m_fields[col]->setText(value.toString());
                        }
                    } else {
                        m_fields[col]->setText(value.toString());
                    }
                }
            }
        }
    } else {
        QMessageBox::critical(this, "Ошибка", "Не удалось загрузить запись:\n" + m_dbManager->lastError());
    }
}

bool RecordDialog::isValidDate(const QString &dateStr)
{
    if (dateStr.isEmpty()) {
        return true; // Пустая дата - это NULL
    }
    
    QRegExp rx("^\\d{4}-\\d{2}-\\d{2}$");
    if (!rx.exactMatch(dateStr)) {
        return false;
    }
    
    QDate date = QDate::fromString(dateStr, "yyyy-MM-dd");
    return date.isValid();
}

bool RecordDialog::isValidTimestamp(const QString &timestampStr)
{
    if (timestampStr.isEmpty()) {
        return true; // Пустой timestamp - это NULL
    }
    
    QRegExp rx("^\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2}$");
    if (!rx.exactMatch(timestampStr)) {
        return false;
    }
    
    QDateTime datetime = QDateTime::fromString(timestampStr, "yyyy-MM-dd hh:mm:ss");
    return datetime.isValid();
}

bool RecordDialog::isValidInteger(const QString &value)
{
    if (value.isEmpty()) {
        return true; // Пустое значение - это NULL
    }
    
    bool ok;
    value.toInt(&ok);
    return ok;
}

QVariant RecordDialog::formatValueForSQL(const QString &columnName, const QString &value, const QString &dataType)
{
    Q_UNUSED(columnName); // Параметр зарезервирован для будущего использования
    QString trimmedValue = value.trimmed();
    
    // Если значение пустое, возвращаем NULL
    if (trimmedValue.isEmpty()) {
        return QVariant();
    }
    
    // Обрабатываем в зависимости от типа данных
    if (dataType == "integer" || dataType == "bigint" || dataType == "smallint") {
        bool ok;
        int intValue = trimmedValue.toInt(&ok);
        if (ok) {
            return QVariant(intValue);
        }
        return QVariant(); // Возвращаем NULL при ошибке
    } else if (dataType == "numeric" || dataType == "decimal" || dataType == "real" || dataType == "double precision") {
        bool ok;
        double doubleValue = trimmedValue.toDouble(&ok);
        if (ok) {
            return QVariant(doubleValue);
        }
        return QVariant(); // Возвращаем NULL при ошибке
    } else if (dataType == "boolean") {
        QString lowerValue = trimmedValue.toLower();
        if (lowerValue == "true" || lowerValue == "1" || lowerValue == "yes" || lowerValue == "t") {
            return QVariant(true);
        } else if (lowerValue == "false" || lowerValue == "0" || lowerValue == "no" || lowerValue == "f") {
            return QVariant(false);
        }
        return QVariant(); // Возвращаем NULL при ошибке
    } else if (dataType == "date") {
        QDate date = QDate::fromString(trimmedValue, "yyyy-MM-dd");
        if (date.isValid()) {
            return QVariant(date);
        }
        return QVariant(); // Возвращаем NULL при ошибке
    } else if (dataType == "timestamp" || dataType == "timestamp without time zone") {
        QDateTime datetime = QDateTime::fromString(trimmedValue, "yyyy-MM-dd hh:mm:ss");
        if (datetime.isValid()) {
            return QVariant(datetime);
        }
        return QVariant(); // Возвращаем NULL при ошибке
    } else {
        // Для строковых типов возвращаем как есть
        return QVariant(trimmedValue);
    }
}

bool RecordDialog::validateRecord(QString &errorMessage)
{
    errorMessage.clear();
    
    // Проверяем обязательные поля (NOT NULL)
    foreach (const DatabaseManager::ColumnDetail &detail, m_columnDetails) {
        // Пропускаем первичный ключ при добавлении (автогенерируется)
        QString primaryKeyColumn = m_dbManager->getPrimaryKeyColumn(m_tableName);
        if (detail.columnName == primaryKeyColumn && m_recordId < 0) {
            continue;
        }
        
        // Проверяем обязательные поля
        if (!detail.isNullable) {
            if (!m_fields.contains(detail.columnName)) {
                errorMessage = QString("Поле '%1' не найдено").arg(getDisplayName(detail.columnName));
                return false;
            }
            
            QString value = m_fields[detail.columnName]->text().trimmed();
            if (value.isEmpty() && detail.defaultValue.isNull()) {
                errorMessage = QString("Поле '%1' обязательно для заполнения").arg(getDisplayName(detail.columnName));
                return false;
            }
        }
        
        // Валидация форматов данных
        if (m_fields.contains(detail.columnName)) {
            QString value = m_fields[detail.columnName]->text().trimmed();
            
            if (!value.isEmpty()) {
                if (detail.dataType == "date") {
                    if (!isValidDate(value)) {
                        errorMessage = QString("Неверный формат даты в поле '%1'. Используйте формат YYYY-MM-DD")
                                      .arg(getDisplayName(detail.columnName));
                        return false;
                    }
                } else if (detail.dataType == "timestamp" || detail.dataType == "timestamp without time zone") {
                    if (!isValidTimestamp(value)) {
                        errorMessage = QString("Неверный формат времени в поле '%1'. Используйте формат YYYY-MM-DD HH:MM:SS")
                                      .arg(getDisplayName(detail.columnName));
                        return false;
                    }
                } else if (detail.dataType == "integer" || detail.dataType == "bigint" || detail.dataType == "smallint") {
                    if (!isValidInteger(value)) {
                        errorMessage = QString("Неверный формат числа в поле '%1'").arg(getDisplayName(detail.columnName));
                        return false;
                    }
                }
                
                // Проверка максимальной длины для строковых типов
                if (detail.characterMaxLength > 0 && (detail.dataType == "character varying" || detail.dataType == "varchar" || detail.dataType == "text")) {
                    if (value.length() > detail.characterMaxLength) {
                        errorMessage = QString("Поле '%1' превышает максимальную длину (%2 символов)")
                                      .arg(getDisplayName(detail.columnName))
                                      .arg(detail.characterMaxLength);
                        return false;
                    }
                }
            }
        }
    }
    
    // Проверка внешних ключей
    foreach (const DatabaseManager::ForeignKeyInfo &fkInfo, m_foreignKeys) {
        if (m_fields.contains(fkInfo.columnName)) {
            QString fkValue = m_fields[fkInfo.columnName]->text().trimmed();
            if (!fkValue.isEmpty()) {
                // Находим тип данных для этого поля
                QString dataType;
                foreach (const DatabaseManager::ColumnDetail &colDetail, m_columnDetails) {
                    if (colDetail.columnName == fkInfo.columnName) {
                        dataType = colDetail.dataType;
                        break;
                    }
                }
                
                // Преобразуем значение в нужный тип для проверки
                QVariant checkValue;
                if (dataType == "integer" || dataType == "bigint" || dataType == "smallint") {
                    bool ok;
                    int intValue = fkValue.toInt(&ok);
                    if (ok) {
                        checkValue = intValue;
                    } else {
                        errorMessage = QString("Поле '%1' должно быть числом")
                                      .arg(getDisplayName(fkInfo.columnName));
                        return false;
                    }
                } else {
                    checkValue = fkValue;
                }
                
                // Проверяем существование связанной записи
                if (!m_dbManager->recordExists(fkInfo.referencedTable, fkInfo.referencedColumn, checkValue)) {
                    errorMessage = QString("Запись с указанным значением в поле '%1' не существует в таблице '%2'")
                                  .arg(getDisplayName(fkInfo.columnName))
                                  .arg(fkInfo.referencedTable);
                    return false;
                }
            }
        }
    }
    
    return true;
}

void RecordDialog::saveRecord()
{
    // Проверяем подключение к БД
    if (!m_dbManager || !m_dbManager->isConnected()) {
        QMessageBox::critical(this, "Ошибка", "База данных не подключена");
        return;
    }
    
    // Валидация данных
    QString validationError;
    if (!validateRecord(validationError)) {
        QMessageBox::warning(this, "Ошибка валидации", validationError);
        return;
    }
    
    // Получаем первичный ключ
    QString idColumn = m_dbManager->getPrimaryKeyColumn(m_tableName);
    if (idColumn.isEmpty()) {
        QMessageBox::critical(this, "Ошибка", "Не удалось определить первичный ключ таблицы");
        return;
    }
    
    // Начинаем транзакцию
    if (!m_dbManager->beginTransaction()) {
        QMessageBox::critical(this, "Ошибка", "Не удалось начать транзакцию");
        return;
    }
    
    bool success = false;
    
    if (m_recordId < 0) {
        // Добавление новой записи
        QStringList columnNames;
        QStringList placeholders;
        
        foreach (const DatabaseManager::ColumnDetail &detail, m_columnDetails) {
            // Пропускаем первичный ключ при добавлении (автогенерируется)
            if (detail.columnName == idColumn) {
                continue;
            }
            
            columnNames << detail.columnName;
            placeholders << QString(":%1").arg(detail.columnName);
        }
        
        if (columnNames.isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Нет полей для вставки");
            m_dbManager->rollbackTransaction();
            return;
        }
        
        QString sql = QString("INSERT INTO %1 (%2) VALUES (%3)")
            .arg(m_tableName)
            .arg(columnNames.join(", "))
            .arg(placeholders.join(", "));
        
        QSqlQuery query = m_dbManager->prepareQuery(sql);
        
        // Привязываем значения
        foreach (const DatabaseManager::ColumnDetail &detail, m_columnDetails) {
            if (detail.columnName == idColumn) {
                continue;
            }
            
            QString value = m_fields[detail.columnName]->text().trimmed();
            QVariant formattedValue = formatValueForSQL(detail.columnName, value, detail.dataType);
            query.bindValue(QString(":%1").arg(detail.columnName), formattedValue);
        }
        
        success = m_dbManager->executePreparedQuery(query);
        
    } else {
        // Обновление существующей записи
        QStringList setClause;
        
        foreach (const DatabaseManager::ColumnDetail &detail, m_columnDetails) {
            // Пропускаем первичный ключ при обновлении
            if (detail.columnName == idColumn) {
                continue;
            }
            
            setClause << QString("%1 = :%2").arg(detail.columnName).arg(detail.columnName);
        }
        
        if (setClause.isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Нет полей для обновления");
            m_dbManager->rollbackTransaction();
            return;
        }
        
        QString sql = QString("UPDATE %1 SET %2 WHERE %3 = :id_value")
            .arg(m_tableName)
            .arg(setClause.join(", "))
            .arg(idColumn);
        
        QSqlQuery query = m_dbManager->prepareQuery(sql);
        
        // Привязываем значения
        foreach (const DatabaseManager::ColumnDetail &detail, m_columnDetails) {
            if (detail.columnName == idColumn) {
                continue;
            }
            
            QString value = m_fields[detail.columnName]->text().trimmed();
            QVariant formattedValue = formatValueForSQL(detail.columnName, value, detail.dataType);
            query.bindValue(QString(":%1").arg(detail.columnName), formattedValue);
        }
        
        query.bindValue(":id_value", m_recordId);
        success = m_dbManager->executePreparedQuery(query);
    }
    
    if (success) {
        if (m_dbManager->commitTransaction()) {
            QMessageBox::information(this, "Успех", m_recordId < 0 ? "Запись успешно добавлена" : "Запись успешно обновлена");
            accept();
        } else {
            QMessageBox::critical(this, "Ошибка", "Не удалось зафиксировать транзакцию:\n" + m_dbManager->lastError());
            m_dbManager->rollbackTransaction();
        }
    } else {
        QMessageBox::critical(this, "Ошибка", "Не удалось сохранить запись:\n" + m_dbManager->lastError());
        m_dbManager->rollbackTransaction();
    }
}

