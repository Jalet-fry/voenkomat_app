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
    setMinimumSize(450, 600);
    
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
    m_layout->setContentsMargins(25, 25, 25, 25);

    QLabel *title = new QLabel("Таблицы базы данных", this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 22px; color: #2c3e50; font-weight: bold; margin-bottom: 10px;");
    m_layout->addWidget(title);

    QPushButton *createTableBtn = new QPushButton("+ Создать новую таблицу", this);
    createTableBtn->setMinimumHeight(45);
    createTableBtn->setCursor(Qt::PointingHandCursor);
    connect(createTableBtn, &QPushButton::clicked, this, &TablesWindow::createTable);
    m_layout->addWidget(createTableBtn);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);

    m_scrollContent = new QWidget();
    m_tableButtonsLayout = new QVBoxLayout(m_scrollContent);
    m_tableButtonsLayout->setSpacing(10);
    m_tableButtonsLayout->setContentsMargins(5, 5, 5, 5);
    m_tableButtonsLayout->addStretch();

    m_scrollArea->setWidget(m_scrollContent);
    m_layout->addWidget(m_scrollArea);

    QPushButton *backBtn = new QPushButton("← Вернуться в меню", this);
    backBtn->setMinimumHeight(45);
    backBtn->setCursor(Qt::PointingHandCursor);
    connect(backBtn, &QPushButton::clicked, this, &TablesWindow::goBack);
    m_layout->addWidget(backBtn);

    using namespace Db;
    m_tableDisplayNames[Tables::CONSCRIPTS] = "Призывники";
    m_tableDisplayNames[Tables::COMMISSIONERS] = "Комиссары";
    m_tableDisplayNames[Tables::FITNESS_CATEGORIES] = "Категории годности";
    m_tableDisplayNames[Tables::MEDICAL_EXAMINATIONS] = "Медосвидетельствования";
    m_tableDisplayNames[Tables::MILITARY_ID_CARDS] = "Военные билеты";
    m_tableDisplayNames[Tables::SERVICE_RECORD_CARDS] = "Учётные карты";
    m_tableDisplayNames[Tables::CALLUP_EVENTS] = "Мероприятия";
    m_tableDisplayNames[Tables::CONSCRIPTS_COMMISSIONERS] = "Связь: Призывник-Комиссар";
}

void TablesWindow::setupStyles()
{
    // Современный нейтральный стиль
    setStyleSheet(
        "QWidget { background-color: #f8f9fa; }"
        "QScrollArea { background-color: transparent; }"
        "QWidget#scrollContent { background-color: transparent; }"
        "QPushButton {"
        "    background-color: #2c3e50;"
        "    font-size: 15px;"
        "    padding: 10px;"
        "    border-radius: 6px;"
        "    color: white;"
        "    border: none;"
        "}"
        "QPushButton:hover { background-color: #34495e; }"
        "QPushButton:pressed { background-color: #1a252f; }"
        "QPushButton[tableButton='true'] {"
        "    background-color: #ffffff;"
        "    color: #2c3e50;"
        "    border: 1px solid #dee2e6;"
        "    text-align: left;"
        "    padding-left: 20px;"
        "}"
        "QPushButton[tableButton='true']:hover {"
        "    background-color: #e9ecef;"
        "    border: 1px solid #3498db;"
        "}"
    );
}

void TablesWindow::refreshTables()
{
    if (!m_dbManager || !m_dbManager->isConnected()) return;
    refreshTableButtons();
}

void TablesWindow::refreshTableButtons()
{
    // Очистка
    QLayoutItem *item;
    while (m_tableButtonsLayout->count() > 1) { // Оставляем растяжку (stretch)
        item = m_tableButtonsLayout->takeAt(0);
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    m_buttonToTable.clear();

    QStringList tables = m_dbManager->getTableList();
    int index = 0;
    foreach (const QString &tableName, tables) {
        QString displayName = m_tableDisplayNames.value(tableName, tableName);
        QPushButton *btn = new QPushButton(QString("%1. %2").arg(index + 1).arg(displayName), this);
        btn->setProperty("tableButton", true);
        btn->setMinimumHeight(50);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setContextMenuPolicy(Qt::CustomContextMenu);

        connect(btn, &QPushButton::clicked, [this, tableName]() { openTable(tableName); });
        connect(btn, &QPushButton::customContextMenuRequested, [this, btn, tableName](const QPoint &pos) {
            showTableContextMenu(btn->mapToGlobal(pos), tableName);
        });

        m_tableButtonsLayout->insertWidget(index, btn);
        m_buttonToTable[btn] = tableName;
        index++;
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
    menu.setStyleSheet("QMenu { background-color: white; border: 1px solid #dee2e6; color: #2c3e50; } "
                       "QMenu::item:selected { background-color: #3498db; color: white; }");

    QAction *backupAction = menu.addAction("📦 Создать бэкап");
    QAction *editStructureAction = menu.addAction("🛠 Структура");
    menu.addSeparator();
    QAction *deleteAction = menu.addAction("❌ Удалить таблицу");

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
        if (backupManager.exportTable(tableName, fileName)) QMessageBox::information(this, "Успех", "Бэкап успешно создан.");
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
    if (QMessageBox::question(this, "Удаление", QString("Вы действительно хотите полностью удалить таблицу '%1'?").arg(tableName)) == QMessageBox::Yes) {
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
    hide();
}
