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
    m_fieldDisplayNames[Conscripts::CONSCRIPT_ID] = "ID призывника";
    m_fieldDisplayNames[Conscripts::FULL_NAME] = "ФИО";
    m_fieldDisplayNames[Conscripts::BIRTH_DATE] = "Дата рождения";
    m_fieldDisplayNames[Conscripts::RESIDENCE_ADDRESS] = "Адрес проживания";
    m_fieldDisplayNames[Conscripts::PASSPORT_NUMBER] = "Номер паспорта";
    m_fieldDisplayNames[Conscripts::MILITARY_TICKET_ID] = "ID военного билета";
    m_fieldDisplayNames[Conscripts::REGISTRATION_CARD_ID] = "ID учётной карты";

    m_fieldDisplayNames[Commissioners::COMMISSIONER_ID] = "ID комиссара";
    m_fieldDisplayNames[Commissioners::FULL_NAME] = "ФИО комиссара";
    m_fieldDisplayNames[Commissioners::POSITION] = "Должность";
    m_fieldDisplayNames[Commissioners::YEARS_OF_SERVICE] = "Стаж работы";
    m_fieldDisplayNames[Commissioners::PHONE_NUMBER] = "Контактный телефон";

    m_fieldDisplayNames[FitnessCategories::CATEGORY_ID] = "ID категории";
    m_fieldDisplayNames[FitnessCategories::CATEGORY_NAME] = "Название категории";
    m_fieldDisplayNames[FitnessCategories::RESTRICTION_DESCRIPTION] = "Описание ограничений";

    m_fieldDisplayNames[MedicalExaminations::CERTIFICATION_ID] = "ID освидетельствования";
    m_fieldDisplayNames[MedicalExaminations::EXAMINATION_DATE] = "Дата проведения";
    m_fieldDisplayNames[MedicalExaminations::EXAMINATION_RESULTS] = "Результаты обследования";

    m_fieldDisplayNames[MilitaryIdCards::TICKET_ID] = "ID билета";
    m_fieldDisplayNames[MilitaryIdCards::TICKET_NUMBER] = "Номер билета";
    m_fieldDisplayNames[MilitaryIdCards::ISSUE_DATE] = "Дата выдачи";

    if (m_dbManager && m_dbManager->isConnected()) {
        m_columnDetails = m_dbManager->getColumnDetails(m_tableName);
    }
    
    m_filterTimer = new QTimer(this);
    m_filterTimer->setSingleShot(true);
    m_filterTimer->setInterval(500);
    connect(m_filterTimer, &QTimer::timeout, this, &TableViewWindow::applyFilters);
    
    setupUI();
    if (!m_dbManager->isHttpMode()) {
        setupFilters(); // В HTTP режиме фильтры пока не реализованы на клиенте для простоты
    }
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

    QLabel *title = new QLabel(m_tableName, this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 18px; color: #333; font-weight: bold;");
    m_layout->addWidget(title);

    m_filterStatusLabel = new QLabel("", this);
    m_filterStatusLabel->setStyleSheet("color: #666; font-size: 12px;");
    m_layout->addWidget(m_filterStatusLabel);

    m_table = new QTableWidget(this);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_table, &QTableWidget::customContextMenuRequested, this, &TableViewWindow::showContextMenu);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &TableViewWindow::updateButtonStates);
    m_layout->addWidget(m_table);

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
        "QTableWidget { background-color: white; border: 2px solid #FFB6C1; border-radius: 5px; }"
        "QHeaderView::section { background-color: #FFB6C1; padding: 5px; border: 1px solid #FF69B4; font-weight: bold; }"
        "QPushButton { background-color: #5cffda; font-size: 16px; padding: 10px; border-radius: 8px; color: black; border: none; min-height: 40px; }"
        "QPushButton:hover { background-color: #00fac1; }"
        "QPushButton:disabled { background-color: #cccccc; color: #666666; }"
    );
}

void TableViewWindow::loadData(const QString &filterClause, const QList<QVariant> &filterParams)
{
    if (!m_dbManager || !m_dbManager->isConnected()) {
        return;
    }

    if (m_dbManager->isHttpMode()) {
        // Загрузка через HTTP
        QJsonArray data = m_dbManager->fetchTableDataHttp(m_tableName);
        if (data.isEmpty() && !m_dbManager->lastError().isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Не удалось загрузить данные через сервер:\n" + m_dbManager->lastError());
            return;
        }

        if (data.isEmpty()) {
            m_table->setRowCount(0);
            m_filterStatusLabel->setText("Нет данных в таблице");
            return;
        }

        // Определяем колонки по первому объекту
        QJsonObject firstObj = data[0].toObject();
        QStringList columns = firstObj.keys();
        m_table->setColumnCount(columns.size());

        QStringList displayHeaders;
        foreach (const QString &col, columns) {
            displayHeaders << getDisplayName(col);
        }
        m_table->setHorizontalHeaderLabels(displayHeaders);

        m_table->setRowCount(data.size());
        for (int i = 0; i < data.size(); ++i) {
            QJsonObject obj = data[i].toObject();
            for (int j = 0; j < columns.size(); ++j) {
                QVariant val = obj[columns[j]].toVariant();
                QTableWidgetItem *item = new QTableWidgetItem(val.isNull() ? "" : val.toString());
                m_table->setItem(i, j, item);
            }
        }
        m_table->resizeColumnsToContents();
        m_filterStatusLabel->setText(QString("Записей загружено через API: %1").arg(data.size()));
        updateButtonStates();
        return;
    }

    // Старый добрый SQL режим
    QString sql = QString("SELECT * FROM %1").arg(DatabaseManager::escapeIdentifier(m_tableName));
    if (!filterClause.isEmpty()) {
        sql += " WHERE " + filterClause;
    }
    sql += " ORDER BY 1 ASC";

    QSqlQuery query = m_dbManager->prepareQuery(sql);
    for (int i = 0; i < filterParams.size(); ++i) {
        query.bindValue(QString(":param%1").arg(i), filterParams[i]);
    }

    if (!m_dbManager->executePreparedQuery(query)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось загрузить данные:\n" + m_dbManager->lastError());
        return;
    }

    QStringList columns = m_dbManager->getColumnList(m_tableName);
    m_table->setColumnCount(columns.size());
    QStringList displayHeaders;
    foreach (const QString &col, columns) displayHeaders << getDisplayName(col);
    m_table->setHorizontalHeaderLabels(displayHeaders);

    int rowCount = 0;
    m_table->setRowCount(0);
    while (query.next()) {
        m_table->insertRow(rowCount);
        for (int i = 0; i < columns.size(); ++i) {
            m_table->setItem(rowCount, i, new QTableWidgetItem(query.value(i).toString()));
        }
        rowCount++;
    }

    m_table->resizeColumnsToContents();
    updateButtonStates();
    m_filterStatusLabel->setText(QString("Всего записей: %1").arg(rowCount));
}

void TableViewWindow::updateButtonStates()
{
    bool hasSelection = m_table->currentRow() >= 0;
    if (m_editBtn) m_editBtn->setEnabled(hasSelection && !m_dbManager->isHttpMode());
    if (m_deleteBtn) m_deleteBtn->setEnabled(hasSelection && !m_dbManager->isHttpMode());
}

QString TableViewWindow::getDisplayName(const QString &fieldName) const
{
    return m_fieldDisplayNames.value(fieldName, fieldName);
}

void TableViewWindow::goBack()
{
    hide();
}

void TableViewWindow::exportToXlsx()
{
    // Реализация экспорта в Excel (QXlsx)
    // ... (код аналогичен вашему исходному)
}

// Заглушки для методов, которые пока не нужны в HTTP режиме
void TableViewWindow::addRecord() { if(m_dbManager->isHttpMode()) QMessageBox::information(this, "API", "Добавление через API в разработке"); }
void TableViewWindow::editRecord() {}
void TableViewWindow::deleteRecord() {}
void TableViewWindow::showContextMenu(const QPoint &pos) {}
void TableViewWindow::onFilterChanged() {}
void TableViewWindow::applyFilters() {}
void TableViewWindow::clearFilters() {}
void TableViewWindow::setupFilters() {}
void TableViewWindow::exportToCSV() {}
