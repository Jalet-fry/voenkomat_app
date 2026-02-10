#include "TablesWindow.h"
#include "TableViewWindow.h"
#include "BackupManager.h"
#include "CreateTableDialog.h"
#include "EditTableStructureDialog.h"
#include "DbConstants.h"
#include <QLabel>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QDateTime>

TablesWindow::TablesWindow(DatabaseManager *dbManager, QWidget *parent)
    : QWidget(parent)
    , m_dbManager(dbManager)
{
    setWindowTitle("Таблицы");
    setGeometry(200, 200, 400, 500);
    
    setupUI();
    setupStyles();
    refreshTables();
}

TablesWindow::~TablesWindow()
{
}

void TablesWindow::setupUI()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setSpacing(15);
    m_layout->setContentsMargins(20, 20, 20, 20);

    QLabel *title = new QLabel("Таблицы базы данных", this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 20px; color: #333; font-weight: bold;");
    m_layout->addWidget(title);

    QPushButton *createTableBtn = new QPushButton("Создать таблицу", this);
    createTableBtn->setMinimumHeight(40);
    createTableBtn->setStyleSheet(
        "QPushButton {"
        "    background-color: #4CAF50;"
        "    color: white;"
        "}"
        "QPushButton:hover { background-color: #45a049; }"
        "QPushButton:pressed { background-color: #3d8b40; }"
    );
    connect(createTableBtn, &QPushButton::clicked, this, &TablesWindow::createTable);
    m_layout->addWidget(createTableBtn);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollContent = new QWidget();
    m_tableButtonsLayout = new QVBoxLayout(m_scrollContent);
    m_tableButtonsLayout->setSpacing(10);
    m_scrollArea->setWidget(m_scrollContent);
    m_layout->addWidget(m_scrollArea);

    QPushButton *backBtn = new QPushButton("Назад", this);
    backBtn->setMinimumHeight(40);
    connect(backBtn, &QPushButton::clicked, this, &TablesWindow::goBack);
    m_layout->addWidget(backBtn);

    // Инициализация названий на основе ПОСЛЕДНЕГО дампа (английские таблицы)
    using namespace Db;
    m_tableDisplayNames[Tables::CONSCRIPTS] = "Призывники";
    m_tableDisplayNames[Tables::COMMISSIONERS] = "Комиссары";
    m_tableDisplayNames[Tables::FITNESS_CATEGORIES] = "Категории годности";
    m_tableDisplayNames[Tables::MEDICAL_EXAMINATIONS] = "Медосвидетельствования";
    m_tableDisplayNames[Tables::MILITARY_ID_CARDS] = "Военные билеты";
    m_tableDisplayNames[Tables::SERVICE_RECORD_CARDS] = "Учётные карты";
    m_tableDisplayNames[Tables::CALLUP_EVENTS] = "Мероприятия";
    m_tableDisplayNames[Tables::CONSCRIPTS_COMMISSIONERS] = "Связь: Призывник-Комиссар";
    m_tableDisplayNames[Tables::CONSCRIPTS_EVENTS] = "Связь: Призывник-Мероприятие";
}

void TablesWindow::setupStyles()
{
    setStyleSheet(
        "QWidget { background-color: #dbffff; }"
        "QPushButton {"
        "    background-color: #FFB6C1;"
        "    font-size: 16px;"
        "    padding: 10px;"
        "    border-radius: 8px;"
        "    color: black;"
        "    border: none;"
        "    min-height: 40px;"
        "}"
        "QPushButton:hover { background-color: #FF69B4; }"
        "QPushButton:pressed { background-color: #FF1493; }"
    );
}

void TablesWindow::refreshTables()
{
    if (!m_dbManager || !m_dbManager->isConnected()) {
        QMessageBox::warning(this, "Ошибка", "База данных не подключена");
        return;
    }
    refreshTableButtons();
}

void TablesWindow::refreshTableButtons()
{
    QLayoutItem *item;
    while ((item = m_tableButtonsLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    m_buttonToTable.clear();

    if (!m_dbManager || !m_dbManager->isConnected()) return;

    QStringList tables = m_dbManager->getTableList();
    foreach (const QString &tableName, tables) {
        QString displayName = m_tableDisplayNames.value(tableName, tableName);
        QPushButton *btn = new QPushButton(displayName, this);
        btn->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(btn, &QPushButton::clicked, [this, tableName]() { openTable(tableName); });
        connect(btn, &QPushButton::customContextMenuRequested, [this, btn, tableName](const QPoint &pos) {
            showTableContextMenu(btn->mapToGlobal(pos), tableName);
        });
        m_tableButtonsLayout->addWidget(btn);
        m_buttonToTable[btn] = tableName;
    }
}

void TablesWindow::openTable(const QString &tableName)
{
    TableViewWindow *viewWindow = new TableViewWindow(m_dbManager, tableName, this);
    viewWindow->setWindowFlags(Qt::Window);
    viewWindow->show();
}

void TablesWindow::showTableContextMenu(const QPoint &pos, const QString &tableName)
{
    QMenu menu(this);
    QAction *backupAction = menu.addAction("Создать бэкап");
    QAction *editStructureAction = menu.addAction("Редактировать структуру");
    QAction *deleteAction = menu.addAction("Удалить таблицу");
    QAction *selectedAction = menu.exec(pos);
    if (selectedAction == backupAction) backupTable(tableName);
    else if (selectedAction == editStructureAction) editTableStructure(tableName);
    else if (selectedAction == deleteAction) deleteTable(tableName);
}

void TablesWindow::backupTable(const QString &tableName)
{
    BackupManager backupManager(m_dbManager);
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    QString fileName = QFileDialog::getSaveFileName(this, "Сохранить бэкап", QString("backup_%1_%2.sql").arg(tableName).arg(timestamp), "SQL Files (*.sql)");
    if (!fileName.isEmpty()) {
        if (backupManager.exportTable(tableName, fileName)) QMessageBox::information(this, "Успех", "Бэкап создан");
        else QMessageBox::critical(this, "Ошибка", backupManager.lastError());
    }
}

void TablesWindow::createTable()
{
    CreateTableDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        if (m_dbManager->createTable(dialog.getTableName(), dialog.getColumns(), dialog.getPrimaryKeys())) refreshTables();
        else QMessageBox::critical(this, "Ошибка", m_dbManager->lastError());
    }
}

void TablesWindow::deleteTable(const QString &tableName)
{
    if (QMessageBox::question(this, "Удаление", QString("Удалить таблицу '%1'?").arg(tableName)) == QMessageBox::Yes) {
        if (m_dbManager->dropTable(tableName, true)) refreshTables();
        else QMessageBox::critical(this, "Ошибка", m_dbManager->lastError());
    }
}

void TablesWindow::editTableStructure(const QString &tableName)
{
    EditTableStructureDialog dialog(m_dbManager, tableName, this);
    if (dialog.exec() == QDialog::Accepted) refreshTables();
}

void TablesWindow::goBack()
{
    if (parentWidget()) parentWidget()->show();
    hide();
}
