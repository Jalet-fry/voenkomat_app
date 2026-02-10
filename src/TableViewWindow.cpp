#include "TableViewWindow.h"
#include "RecordDialog.h"
#include "BackupManager.h"
#include "DbConstants.h"
#include <QLabel>
#include <QMessageBox>
#include <QHeaderView>
#include <QSqlQuery>
#include <QSqlError>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QLineEdit>
#include <QDateEdit>
#include <QDateTimeEdit>
#include <QComboBox>
#include <QScrollArea>
#include <QGridLayout>
#include <QRegExpValidator>
#include <QIntValidator>
#include <QDoubleValidator>
#include "xlsxdocument.h"
#include "xlsxformat.h"
using namespace QXlsx;

TableViewWindow::TableViewWindow(DatabaseManager *dbManager, const QString &tableName, QWidget *parent)
    : QWidget(parent)
    , m_dbManager(dbManager)
    , m_tableName(tableName)
    , m_editBtn(nullptr)
    , m_deleteBtn(nullptr)
    , m_filterScrollArea(nullptr)
    , m_filterWidget(nullptr)
    , m_filterLayout(nullptr)
    , m_clearFiltersBtn(nullptr)
    , m_filterTimer(nullptr)
    , m_filterStatusLabel(nullptr)
{
    setWindowTitle(m_tableName);
    setGeometry(100, 100, 800, 600);
    
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
    m_fieldDisplayNames["id_prizivnika"] = "ID призывника"; // В некоторых местах может остаться старое имя в БД (внешние ключи)
    m_fieldDisplayNames[ConscriptsCommissioners::INTERACTION_DATE] = "Дата взаимодействия";
    m_fieldDisplayNames[ConscriptsCommissioners::OFFICE_NUMBER] = "Номер кабинета";
    
    // Получаем информацию о колонках
    if (m_dbManager && m_dbManager->isConnected()) {
        m_columnDetails = m_dbManager->getColumnDetails(m_tableName);
    }
    
    // Инициализация таймера для debounce фильтров
    m_filterTimer = new QTimer(this);
    m_filterTimer->setSingleShot(true);
    m_filterTimer->setInterval(500); // 500ms задержка
    connect(m_filterTimer, &QTimer::timeout, this, &TableViewWindow::applyFilters);
    
    setupUI();
    setupFilters();
    setupStyles();
    loadData();
}

TableViewWindow::~TableViewWindow()
{
}

void TableViewWindow::setupUI()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setSpacing(10);
    m_layout->setContentsMargins(20, 20, 20, 20);

    // Заголовок
    QLabel *title = new QLabel(m_tableName, this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 18px; color: #333; font-weight: bold;");
    m_layout->addWidget(title);

    // Метка статуса фильтров (будет заполнена в setupFilters)
    m_filterStatusLabel = new QLabel("", this);
    m_filterStatusLabel->setStyleSheet("color: #666; font-size: 12px;");
    m_layout->addWidget(m_filterStatusLabel);

    // Таблица
    m_table = new QTableWidget(this);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_table, &QTableWidget::customContextMenuRequested, this, &TableViewWindow::showContextMenu);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &TableViewWindow::updateButtonStates);
    m_layout->addWidget(m_table);

    // Кнопки
    QHBoxLayout *btnLayout = new QHBoxLayout();
    
    QPushButton *addBtn = new QPushButton("Добавить", this);
    addBtn->setMinimumHeight(40);
    connect(addBtn, &QPushButton::clicked, this, &TableViewWindow::addRecord);
    btnLayout->addWidget(addBtn);
    
    m_editBtn = new QPushButton("Изменить", this);
    m_editBtn->setMinimumHeight(40);
    m_editBtn->setEnabled(false);
    connect(m_editBtn, &QPushButton::clicked, this, &TableViewWindow::editRecord);
    btnLayout->addWidget(m_editBtn);
    
    m_deleteBtn = new QPushButton("Удалить", this);
    m_deleteBtn->setMinimumHeight(40);
    m_deleteBtn->setEnabled(false);
    connect(m_deleteBtn, &QPushButton::clicked, this, &TableViewWindow::deleteRecord);
    btnLayout->addWidget(m_deleteBtn);
    
    QPushButton *exportCsvBtn = new QPushButton("Экспорт в CSV", this);
    exportCsvBtn->setMinimumHeight(40);
    connect(exportCsvBtn, &QPushButton::clicked, this, &TableViewWindow::exportToCSV);
    btnLayout->addWidget(exportCsvBtn);
    
    QPushButton *exportXlsxBtn = new QPushButton("Экспорт в Excel", this);
    exportXlsxBtn->setMinimumHeight(40);
    connect(exportXlsxBtn, &QPushButton::clicked, this, &TableViewWindow::exportToXlsx);
    btnLayout->addWidget(exportXlsxBtn);
    
    QPushButton *backBtn = new QPushButton("Назад", this);
    backBtn->setMinimumHeight(40);
    connect(backBtn, &QPushButton::clicked, this, &TableViewWindow::goBack);
    btnLayout->addWidget(backBtn);
    
    m_layout->addLayout(btnLayout);
}

void TableViewWindow::setupStyles()
{
    setStyleSheet(
        "QWidget { background-color: #dbffff; }"
        "QTableWidget {"
        "    background-color: white;"
        "    border: 2px solid #FFB6C1;"
        "    border-radius: 5px;"
        "}"
        "QHeaderView::section {"
        "    background-color: #FFB6C1;"
        "    padding: 5px;"
        "    border: 1px solid #FF69B4;"
        "    font-weight: bold;"
        "}"
        "QPushButton {"
        "    background-color: #5cffda;"
        "    font-size: 16px;"
        "    padding: 10px;"
        "    border-radius: 8px;"
        "    color: black;"
        "    border: none;"
        "    min-height: 40px;"
        "}"
        "QPushButton:hover { background-color: #00fac1; }"
        "QPushButton:pressed { background-color: #00c79a; }"
        "QPushButton:disabled { background-color: #cccccc; color: #666666; }"
    );
    
    // Специальные стили для кнопки удаления
    if (m_deleteBtn) {
        m_deleteBtn->setStyleSheet(
            "QPushButton {"
            "    background-color: #ff6b6b;"
            "    font-size: 16px;"
            "    padding: 10px;"
            "    border-radius: 8px;"
            "    color: white;"
            "    border: none;"
            "    min-height: 40px;"
            "}"
            "QPushButton:hover { background-color: #ff5252; }"
            "QPushButton:pressed { background-color: #e53935; }"
            "QPushButton:disabled { background-color: #cccccc; color: #666666; }"
        );
    }
}

void TableViewWindow::setupFilters()
{
    if (m_columnDetails.isEmpty()) {
        return;
    }

    // Создаем ScrollArea для панели фильтров
    m_filterScrollArea = new QScrollArea(this);
    m_filterScrollArea->setWidgetResizable(true);
    m_filterScrollArea->setMaximumHeight(200);
    m_filterScrollArea->setStyleSheet(
        "QScrollArea { border: 1px solid #ccc; border-radius: 5px; background-color: #f9f9f9; }"
    );

    // Создаем виджет для фильтров
    m_filterWidget = new QWidget();
    m_filterLayout = new QGridLayout(m_filterWidget);
    m_filterLayout->setSpacing(10);
    m_filterLayout->setContentsMargins(10, 10, 10, 10);

    int row = 0;
    int col = 0;
    const int colsPerRow = 3; // 3 колонки фильтров в ряд

    foreach (const DatabaseManager::ColumnDetail &column, m_columnDetails) {
        QString columnName = column.columnName;
        QString dataType = column.dataType.toUpper();
        QString displayName = getDisplayName(columnName);

        // Создаем метку
        QLabel *label = new QLabel(displayName + ":", m_filterWidget);
        label->setStyleSheet("font-weight: bold; color: #333;");
        m_filterLayout->addWidget(label, row, col * 2);

        // Создаем виджет фильтра в зависимости от типа данных
        QWidget *filterWidget = nullptr;

        if (dataType.contains("INT", Qt::CaseInsensitive) || 
            dataType == "BIGINT" || dataType == "SMALLINT") {
            // Целочисленные типы
            QLineEdit *lineEdit = new QLineEdit(m_filterWidget);
            lineEdit->setPlaceholderText("Число");
            lineEdit->setValidator(new QIntValidator(lineEdit));
            connect(lineEdit, &QLineEdit::textChanged, this, &TableViewWindow::onFilterChanged);
            filterWidget = lineEdit;
        }
        else if (dataType.contains("NUMERIC", Qt::CaseInsensitive) || 
                 dataType.contains("DECIMAL", Qt::CaseInsensitive) ||
                 dataType.contains("FLOAT", Qt::CaseInsensitive) ||
                 dataType.contains("REAL", Qt::CaseInsensitive) ||
                 dataType.contains("DOUBLE", Qt::CaseInsensitive)) {
            // Числовые типы с плавающей точкой
            QLineEdit *lineEdit = new QLineEdit(m_filterWidget);
            lineEdit->setPlaceholderText("Число");
            lineEdit->setValidator(new QDoubleValidator(lineEdit));
            connect(lineEdit, &QLineEdit::textChanged, this, &TableViewWindow::onFilterChanged);
            filterWidget = lineEdit;
        }
        else if (dataType == "DATE") {
            // Дата
            QDateEdit *dateEdit = new QDateEdit(m_filterWidget);
            dateEdit->setCalendarPopup(true);
            dateEdit->setMinimumDate(QDate(1900, 1, 1));
            dateEdit->setMaximumDate(QDate(2100, 12, 31));
            dateEdit->setDisplayFormat("yyyy-MM-dd");
            dateEdit->setSpecialValueText(""); // Пустое значение
            dateEdit->setDate(QDate(2000, 1, 1)); // Устанавливаем специальное значение (пустое)
            connect(dateEdit, &QDateEdit::dateChanged, this, &TableViewWindow::onFilterChanged);
            filterWidget = dateEdit;
        }
        else if (dataType.contains("TIMESTAMP", Qt::CaseInsensitive) ||
                 dataType.contains("TIME", Qt::CaseInsensitive)) {
            // Дата и время
            QDateTimeEdit *dateTimeEdit = new QDateTimeEdit(m_filterWidget);
            dateTimeEdit->setCalendarPopup(true);
            dateTimeEdit->setDateTime(QDateTime::currentDateTime());
            dateTimeEdit->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
            connect(dateTimeEdit, &QDateTimeEdit::dateTimeChanged, this, &TableViewWindow::onFilterChanged);
            filterWidget = dateTimeEdit;
        }
        else if (dataType == "BOOLEAN" || dataType == "BOOL") {
            // Логический тип
            QComboBox *combo = new QComboBox(m_filterWidget);
            combo->addItem("", QVariant());
            combo->addItem("Да", true);
            combo->addItem("Нет", false);
            connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(onFilterChanged()));
            filterWidget = combo;
        }
        else {
            // Текстовые типы (VARCHAR, TEXT, CHAR и т.д.)
            QLineEdit *lineEdit = new QLineEdit(m_filterWidget);
            lineEdit->setPlaceholderText("Текст");
            connect(lineEdit, &QLineEdit::textChanged, this, &TableViewWindow::onFilterChanged);
            filterWidget = lineEdit;
        }

        if (filterWidget) {
            filterWidget->setObjectName(columnName); // Сохраняем имя колонки
            m_filterLayout->addWidget(filterWidget, row, col * 2 + 1);
            m_filterWidgets[columnName] = filterWidget;
        }

        col++;
        if (col >= colsPerRow) {
            col = 0;
            row++;
        }
    }

    // Добавляем кнопку "Сбросить фильтры"
    row++;
    m_clearFiltersBtn = new QPushButton("Сбросить фильтры", m_filterWidget);
    m_clearFiltersBtn->setStyleSheet(
        "QPushButton {"
        "    background-color: #ff9800;"
        "    color: white;"
        "    padding: 8px 15px;"
        "    border-radius: 5px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #f57c00; }"
        "QPushButton:pressed { background-color: #e65100; }"
    );
    connect(m_clearFiltersBtn, &QPushButton::clicked, this, &TableViewWindow::clearFilters);
    m_filterLayout->addWidget(m_clearFiltersBtn, row, 0, 1, colsPerRow * 2);

    m_filterWidget->setLayout(m_filterLayout);
    m_filterScrollArea->setWidget(m_filterWidget);

    // Вставляем панель фильтров перед таблицей
    int tableIndex = m_layout->indexOf(m_table);
    m_layout->insertWidget(tableIndex, m_filterScrollArea);
}

void TableViewWindow::loadData(const QString &filterClause, const QList<QVariant> &filterParams)
{
    if (!m_dbManager || !m_dbManager->isConnected()) {
        return;
    }

    // Формируем SQL запрос
    QString sql = QString("SELECT * FROM %1").arg(DatabaseManager::escapeIdentifier(m_tableName));
    if (!filterClause.isEmpty()) {
        sql += " WHERE " + filterClause;
    }
    sql += " ORDER BY 1 ASC";

    // Используем параметризованный запрос
    QSqlQuery query = m_dbManager->prepareQuery(sql);
    
    // Привязываем параметры по именам
    for (int i = 0; i < filterParams.size(); ++i) {
        query.bindValue(QString(":param%1").arg(i), filterParams[i]);
    }

    bool ok = m_dbManager->executePreparedQuery(query);
    
    if (!ok) {
        QMessageBox::critical(this, "Ошибка", "Не удалось загрузить данные:\n" + m_dbManager->lastError());
        return;
    }

    // Получаем названия столбцов
    QStringList columns = m_dbManager->getColumnList(m_tableName);
    m_table->setColumnCount(columns.size());
    
    QStringList displayHeaders;
    foreach (const QString &col, columns) {
        displayHeaders << getDisplayName(col);
    }
    m_table->setHorizontalHeaderLabels(displayHeaders);

    // Получаем данные
    QList<QList<QVariant>> rows;
    int rowCount = 0;
    while (query.next()) {
        QList<QVariant> row;
        for (int i = 0; i < columns.size(); ++i) {
            row << query.value(i);
        }
        rows << row;
        rowCount++;
    }

    m_table->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); ++i) {
        for (int j = 0; j < columns.size(); ++j) {
            QTableWidgetItem *item = new QTableWidgetItem(
                rows[i][j].isNull() ? "" : rows[i][j].toString());
            m_table->setItem(i, j, item);
        }
    }

    m_table->resizeColumnsToContents();
    updateButtonStates();
    
    // Обновляем статус фильтров
    if (!filterClause.isEmpty()) {
        m_filterStatusLabel->setText(QString("Найдено записей: %1 (применены фильтры)").arg(rowCount));
        m_filterStatusLabel->setStyleSheet("color: #2196F3; font-size: 12px; font-weight: bold;");
    } else {
        m_filterStatusLabel->setText(QString("Всего записей: %1").arg(rowCount));
        m_filterStatusLabel->setStyleSheet("color: #666; font-size: 12px;");
    }
}

QString TableViewWindow::escapeLikePattern(const QString &text) const
{
    // Экранируем специальные символы для LIKE: % и _
    QString escaped = text;
    escaped.replace("\\", "\\\\"); // Сначала экранируем обратный слэш
    escaped.replace("%", "\\%");
    escaped.replace("_", "\\_");
    return escaped;
}

void TableViewWindow::buildFilterQuery(QString &whereClause, QList<QVariant> &params)
{
    whereClause.clear();
    params.clear();
    
    QStringList conditions;
    int paramIndex = 0;
    
    foreach (const DatabaseManager::ColumnDetail &column, m_columnDetails) {
        QString columnName = column.columnName;
        QString dataType = column.dataType.toUpper();
        
        QWidget *filterWidget = m_filterWidgets.value(columnName);
        if (!filterWidget) {
            continue;
        }
        
        QVariant filterValue;
        bool hasValue = false;
        
        // Получаем значение в зависимости от типа виджета
        if (QLineEdit *lineEdit = qobject_cast<QLineEdit*>(filterWidget)) {
            QString text = lineEdit->text().trimmed();
            if (!text.isEmpty()) {
                if (dataType.contains("INT", Qt::CaseInsensitive) || 
                    dataType == "BIGINT" || dataType == "SMALLINT") {
                    bool ok;
                    int intValue = text.toInt(&ok);
                    if (ok) {
                        filterValue = intValue;
                        hasValue = true;
                    }
                }
                else if (dataType.contains("NUMERIC", Qt::CaseInsensitive) || 
                         dataType.contains("DECIMAL", Qt::CaseInsensitive) ||
                         dataType.contains("FLOAT", Qt::CaseInsensitive) ||
                         dataType.contains("REAL", Qt::CaseInsensitive) ||
                         dataType.contains("DOUBLE", Qt::CaseInsensitive)) {
                    bool ok;
                    double doubleValue = text.toDouble(&ok);
                    if (ok) {
                        filterValue = doubleValue;
                        hasValue = true;
                    }
                }
                else {
                    // Текстовые типы - используем LIKE
                    filterValue = escapeLikePattern(text);
                    hasValue = true;
                }
            }
        }
        else if (QDateEdit *dateEdit = qobject_cast<QDateEdit*>(filterWidget)) {
            QDate date = dateEdit->date();
            // Проверяем, что дата не является специальным значением (пустым)
            if (date.isValid() && date != QDate(2000, 1, 1)) {
                filterValue = date;
                hasValue = true;
            }
        }
        else if (QDateTimeEdit *dateTimeEdit = qobject_cast<QDateTimeEdit*>(filterWidget)) {
            QDateTime dateTime = dateTimeEdit->dateTime();
            if (dateTime.isValid()) {
                filterValue = dateTime;
                hasValue = true;
            }
        }
        else if (QComboBox *combo = qobject_cast<QComboBox*>(filterWidget)) {
            if (combo->currentIndex() > 0) { // Индекс 0 - пустое значение
                filterValue = combo->currentData();
                hasValue = true;
            }
        }
        
        if (hasValue) {
            QString escapedColumnName = DatabaseManager::escapeIdentifier(columnName);
            
            if (dataType.contains("INT", Qt::CaseInsensitive) || 
                dataType == "BIGINT" || dataType == "SMALLINT" ||
                dataType.contains("NUMERIC", Qt::CaseInsensitive) || 
                dataType.contains("DECIMAL", Qt::CaseInsensitive) ||
                dataType.contains("FLOAT", Qt::CaseInsensitive) ||
                dataType.contains("REAL", Qt::CaseInsensitive) ||
                dataType.contains("DOUBLE", Qt::CaseInsensitive) ||
                dataType == "BOOLEAN" || dataType == "BOOL") {
                // Точное сравнение для числовых и булевых типов
                conditions << QString("%1 = :param%2").arg(escapedColumnName).arg(paramIndex);
                params << filterValue;
                paramIndex++;
            }
            else if (dataType == "DATE") {
                conditions << QString("%1 = :param%2").arg(escapedColumnName).arg(paramIndex);
                params << filterValue;
                paramIndex++;
            }
            else if (dataType.contains("TIMESTAMP", Qt::CaseInsensitive) ||
                     dataType.contains("TIME", Qt::CaseInsensitive)) {
                conditions << QString("%1 = :param%2").arg(escapedColumnName).arg(paramIndex);
                params << filterValue;
                paramIndex++;
            }
            else {
                // LIKE для текстовых типов
                conditions << QString("%1 LIKE :param%2").arg(escapedColumnName).arg(paramIndex);
                params << QString("%%1%").arg(filterValue.toString()); // Добавляем % для поиска подстроки
                paramIndex++;
            }
        }
    }
    
    if (!conditions.isEmpty()) {
        whereClause = conditions.join(" AND ");
    }
}

void TableViewWindow::onFilterChanged()
{
    // Запускаем таймер для debounce - применяем фильтры через 500ms после последнего изменения
    m_filterTimer->stop();
    m_filterTimer->start();
}

void TableViewWindow::applyFilters()
{
    QString whereClause;
    QList<QVariant> params;
    
    buildFilterQuery(whereClause, params);
    loadData(whereClause, params);
}

void TableViewWindow::clearFilters()
{
    // Очищаем все виджеты фильтров
    foreach (QWidget *widget, m_filterWidgets.values()) {
        if (QLineEdit *lineEdit = qobject_cast<QLineEdit*>(widget)) {
            lineEdit->clear();
        }
        else if (QDateEdit *dateEdit = qobject_cast<QDateEdit*>(widget)) {
            dateEdit->setDate(QDate(2000, 1, 1)); // Устанавливаем специальное значение (пустое)
        }
        else if (QDateTimeEdit *dateTimeEdit = qobject_cast<QDateTimeEdit*>(widget)) {
            dateTimeEdit->setDateTime(QDateTime::currentDateTime());
        }
        else if (QComboBox *combo = qobject_cast<QComboBox*>(widget)) {
            combo->setCurrentIndex(0); // Пустое значение
        }
    }
    
    // Загружаем данные без фильтров
    loadData();
}

void TableViewWindow::updateButtonStates()
{
    bool hasSelection = m_table->currentRow() >= 0;
    if (m_editBtn) {
        m_editBtn->setEnabled(hasSelection);
    }
    if (m_deleteBtn) {
        m_deleteBtn->setEnabled(hasSelection);
    }
}

QString TableViewWindow::getDisplayName(const QString &fieldName) const
{
    return m_fieldDisplayNames.value(fieldName, fieldName);
}

void TableViewWindow::addRecord()
{
    RecordDialog dialog(m_dbManager, m_tableName, this);
    if (dialog.exec() == QDialog::Accepted) {
        loadData();
    }
}

void TableViewWindow::editRecord()
{
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите запись для редактирования");
        return;
    }

    QStringList columns = m_dbManager->getColumnList(m_tableName);
    if (columns.isEmpty()) {
        return;
    }

    // Для составных ключей используем специальный идентификатор
    // В RecordDialog будет использоваться первый первичный ключ для идентификации записи
    QStringList primaryKeys = m_dbManager->getPrimaryKeys(m_tableName);
    if (primaryKeys.isEmpty()) {
        QMessageBox::critical(this, "Ошибка", "Не удалось определить первичный ключ таблицы");
        return;
    }
    
    // Находим индекс первой колонки первичного ключа
    QString firstPkColumn = primaryKeys.first();
    int pkColumnIndex = columns.indexOf(firstPkColumn);
    if (pkColumnIndex < 0) {
        QMessageBox::critical(this, "Ошибка", "Колонка первичного ключа не найдена");
        return;
    }
    
    QTableWidgetItem *idItem = m_table->item(row, pkColumnIndex);
    if (!idItem) {
        return;
    }

    // Для составных ключей передаем -1, чтобы RecordDialog загрузил запись по всем ключам
    int recordId = -1;
    if (primaryKeys.size() == 1) {
        // Одиночный ключ - используем его значение
        bool ok;
        recordId = idItem->text().toInt(&ok);
        if (!ok) {
            recordId = -1;
        }
    }
    
    RecordDialog dialog(m_dbManager, m_tableName, this, recordId);
    if (dialog.exec() == QDialog::Accepted) {
        loadData();
    }
}

void TableViewWindow::deleteRecord()
{
    // Проверяем подключение к БД
    if (!m_dbManager || !m_dbManager->isConnected()) {
        QMessageBox::critical(this, "Ошибка", "База данных не подключена");
        return;
    }

    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите запись для удаления");
        return;
    }

    // Получаем первичные ключи (может быть составной ключ)
    QStringList primaryKeys = m_dbManager->getPrimaryKeys(m_tableName);
    if (primaryKeys.isEmpty()) {
        QMessageBox::critical(this, "Ошибка", "Не удалось определить первичный ключ таблицы");
        return;
    }

    // Находим индексы колонок с первичными ключами
    QStringList columns = m_dbManager->getColumnList(m_tableName);
    QStringList whereClauses;
    
    bool allFound = true;
    foreach (const QString &pkColumn, primaryKeys) {
        int pkColumnIndex = columns.indexOf(pkColumn);
        if (pkColumnIndex < 0) {
            QMessageBox::critical(this, "Ошибка", QString("Колонка первичного ключа '%1' не найдена в таблице").arg(pkColumn));
            allFound = false;
            break;
        }
        
        QTableWidgetItem *pkItem = m_table->item(row, pkColumnIndex);
        if (!pkItem) {
            QMessageBox::warning(this, "Ошибка", QString("Не удалось получить значение первичного ключа '%1'").arg(pkColumn));
            allFound = false;
            break;
        }
        
        whereClauses << QString("%1 = :%2").arg(DatabaseManager::escapeIdentifier(pkColumn)).arg(pkColumn);
    }
    
    if (!allFound) {
        return;
    }

    // Получаем информацию о записи для отображения в подтверждении
    QString recordInfo;
    if (!primaryKeys.isEmpty() && primaryKeys.size() == 1) {
        int pkIndex = columns.indexOf(primaryKeys.first());
        if (pkIndex >= 0) {
            QTableWidgetItem *pkItem = m_table->item(row, pkIndex);
            if (pkItem) {
                recordInfo = QString("\n\nID записи: %1").arg(pkItem->text());
            }
        }
    }
    
    int ret = QMessageBox::question(this, "Подтверждение удаления",
        QString("Вы уверены, что хотите удалить эту запись из таблицы '%1'?%2").arg(m_tableName).arg(recordInfo),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        // Начинаем транзакцию
        if (!m_dbManager->beginTransaction()) {
            QMessageBox::critical(this, "Ошибка", "Не удалось начать транзакцию");
            return;
        }

        // Формируем WHERE clause для составного ключа
        QString sql = QString("DELETE FROM %1 WHERE %2").arg(DatabaseManager::escapeIdentifier(m_tableName)).arg(whereClauses.join(" AND "));
        QSqlQuery query = m_dbManager->prepareQuery(sql);
        
        // Привязываем значения
        foreach (const QString &pkColumn, primaryKeys) {
            int pkColumnIndex = columns.indexOf(pkColumn);
            QTableWidgetItem *pkItem = m_table->item(row, pkColumnIndex);
            QString pkValue = pkItem->text();
            query.bindValue(QString(":%1").arg(pkColumn), pkValue);
        }

        bool success = m_dbManager->executePreparedQuery(query);

        if (success) {
            if (m_dbManager->commitTransaction()) {
                QMessageBox::information(this, "Успех", QString("Запись успешно удалена из таблицы '%1'").arg(m_tableName));
                loadData();
            } else {
                QMessageBox::critical(this, "Ошибка", "Не удалось зафиксировать транзакцию:\n" + m_dbManager->lastError());
                m_dbManager->rollbackTransaction();
            }
        } else {
            QString errorMsg = m_dbManager->lastError();
            // Проверяем, не связана ли запись с другими записями
            if (errorMsg.contains("foreign key", Qt::CaseInsensitive) || 
                errorMsg.contains("нарушает ограничение внешнего ключа", Qt::CaseInsensitive)) {
                QMessageBox::warning(this, "Ошибка удаления", 
                    "Не удалось удалить запись, так как она связана с другими записями в базе данных.\n\n"
                    "Сначала удалите связанные записи.");
            } else {
                QMessageBox::critical(this, "Ошибка", "Не удалось удалить запись:\n" + errorMsg);
            }
            m_dbManager->rollbackTransaction();
        }
    }
}

void TableViewWindow::showContextMenu(const QPoint &pos)
{
    if (m_table->currentRow() < 0) {
        return;
    }

    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background-color: white; border: 1px solid #ccc; }"
        "QMenu::item { padding: 5px 25px 5px 20px; color: black; }"
        "QMenu::item:selected { background-color: #E0B0FF; color: white; }"
    );

    QAction *editAction = menu.addAction("Изменить");
    QAction *deleteAction = menu.addAction("Удалить");

    QAction *selectedAction = menu.exec(m_table->viewport()->mapToGlobal(pos));

    if (selectedAction == editAction) {
        editRecord();
    } else if (selectedAction == deleteAction) {
        deleteRecord();
    }
}

void TableViewWindow::goBack()
{
    if (parentWidget()) {
        parentWidget()->raise();
        parentWidget()->activateWindow();
    }
    hide();
}

void TableViewWindow::exportToCSV()
{
    if (!m_table || m_table->rowCount() == 0) {
        QMessageBox::information(this, "Информация", "Нет данных для экспорта");
        return;
    }
    
    // Получаем путь к директории для экспорта таблиц в CSV
    BackupManager backupManager(m_dbManager);
    QString tablesDir = backupManager.getTablesExportPath("csv");
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    QString defaultFileName = QString("table_%1_%2.csv").arg(m_tableName).arg(timestamp);
    QString defaultPath = QDir(tablesDir).absoluteFilePath(defaultFileName);
    
    QString fileName = QFileDialog::getSaveFileName(this,
        QString("Экспорт таблицы %1 в CSV").arg(m_tableName),
        defaultPath,
        "CSV Files (*.csv);;All Files (*)");
    
    if (fileName.isEmpty()) {
        return;
    }
    
    // Добавляем расширение .csv если его нет
    if (!fileName.endsWith(".csv", Qt::CaseInsensitive)) {
        fileName += ".csv";
    }
    
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Ошибка", 
                            QString("Не удалось создать файл:\n%1").arg(file.errorString()));
        return;
    }
    
    QTextStream out(&file);
    out.setCodec("UTF-8");
    
    // Добавляем BOM для UTF-8 (необходимо для правильного отображения в Excel)
    out << "\xEF\xBB\xBF";
    
    // Записываем заголовки
    QStringList headers;
    for (int col = 0; col < m_table->columnCount(); ++col) {
        QString header = m_table->horizontalHeaderItem(col) ? 
                        m_table->horizontalHeaderItem(col)->text() : 
                        QString("Column %1").arg(col + 1);
        headers << header;
    }
    out << headers.join(",") << "\n";
    
    // Записываем данные
    for (int row = 0; row < m_table->rowCount(); ++row) {
        QStringList values;
        for (int col = 0; col < m_table->columnCount(); ++col) {
            QTableWidgetItem *item = m_table->item(row, col);
            QString value = item ? item->text() : "";
            // Экранируем кавычки и запятые для CSV
            if (value.contains(",") || value.contains("\"") || value.contains("\n")) {
                value.replace("\"", "\"\""); // Экранируем двойные кавычки
                value = "\"" + value + "\""; // Оборачиваем в кавычки
            }
            values << value;
        }
        out << values.join(",") << "\n";
    }
    
    file.close();
    QMessageBox::information(this, "Успех", 
                            QString("Таблица успешно экспортирована в файл:\n%1").arg(fileName));
}

void TableViewWindow::exportToXlsx()
{
    if (!m_table || m_table->rowCount() == 0) {
        QMessageBox::information(this, "Информация", "Нет данных для экспорта");
        return;
    }
    
    // Получаем путь к директории для экспорта таблиц в Excel
    BackupManager backupManager(m_dbManager);
    QString tablesDir = backupManager.getTablesExportPath("xlsx");
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    QString defaultFileName = QString("table_%1_%2.xlsx").arg(m_tableName).arg(timestamp);
    QString defaultPath = QDir(tablesDir).absoluteFilePath(defaultFileName);
    
    QString fileName = QFileDialog::getSaveFileName(this,
        QString("Экспорт таблицы %1 в Excel").arg(m_tableName),
        defaultPath,
        "Excel Files (*.xlsx);;All Files (*)");
    
    if (fileName.isEmpty()) {
        return;
    }
    
    // Добавляем расширение .xlsx если его нет
    if (!fileName.endsWith(".xlsx", Qt::CaseInsensitive)) {
        fileName += ".xlsx";
    }
    
    // Создаем Excel документ
    Document xlsx;
    
    // Записываем заголовки
    for (int col = 0; col < m_table->columnCount(); ++col) {
        QString header = m_table->horizontalHeaderItem(col) ? 
                        m_table->horizontalHeaderItem(col)->text() : 
                        QString("Column %1").arg(col + 1);
        xlsx.write(1, col + 1, header);
    }
    
    // Форматируем заголовки
    Format headerFormat;
    headerFormat.setFontBold(true);
    headerFormat.setFillPattern(Format::PatternSolid);
    headerFormat.setPatternBackgroundColor(QColor(200, 200, 200));
    for (int col = 1; col <= m_table->columnCount(); ++col) {
        xlsx.write(1, col, xlsx.read(1, col), headerFormat);
    }
    
    // Записываем данные
    for (int row = 0; row < m_table->rowCount(); ++row) {
        for (int col = 0; col < m_table->columnCount(); ++col) {
            QTableWidgetItem *item = m_table->item(row, col);
            if (item) {
                QVariant value = item->text();
                xlsx.write(row + 2, col + 1, value);
            }
        }
    }
    
    // Автоматически подгоняем ширину колонок
    for (int col = 1; col <= m_table->columnCount(); ++col) {
        xlsx.setColumnWidth(col, 15);
    }
    
    // Сохраняем файл
    if (xlsx.saveAs(fileName)) {
        QMessageBox::information(this, "Успех", 
                                QString("Таблица успешно экспортирована в Excel:\n%1").arg(fileName));
    } else {
        QMessageBox::critical(this, "Ошибка", 
                            QString("Не удалось сохранить файл:\n%1").arg(fileName));
    }
}

