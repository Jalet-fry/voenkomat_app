#include "MainWindow.h"
#include "TablesWindow.h"
#include "QueriesWindow.h"
#include "BackupManager.h"
#include "ConfigManager.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QApplication>
#include <QHBoxLayout>
#include <QDebug>
#include <QKeyEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_dbManager(new DatabaseManager(this))
    , m_tablesWindow(nullptr)
    , m_queriesWindow(nullptr)
    , m_classicMode(false)
{
    setWindowTitle("Военкомат - Главное меню");
    resize(450, 650);
    
    setupUI();
    setupStyles();
    
    if (!connectToDatabase()) {
        updateConnectionStatus();
        QMessageBox::critical(this, "Ошибка", "Не удалось подключиться к базе данных. Проверьте настройки и запущен ли сервер.");
    } else {
        updateConnectionStatus();
    }
}

void MainWindow::updateConnectionStatus()
{
    if (!m_statusLabel) return;
    
    if (m_dbManager && m_dbManager->isConnected()) {
        QString mode = m_dbManager->isHttpMode() ? " (API)" : " (SQL)";
        m_statusLabel->setText("● Подключено" + mode);
        m_statusLabel->setStyleSheet("color: #27ae60; font-weight: bold; font-size: 13px;");
    } else {
        m_statusLabel->setText("○ Отключено");
        m_statusLabel->setStyleSheet("color: #e74c3c; font-weight: bold; font-size: 13px;");
    }
}

MainWindow::~MainWindow()
{
    if (m_tablesWindow) m_tablesWindow->deleteLater();
    if (m_queriesWindow) m_queriesWindow->deleteLater();
}

void MainWindow::setupUI()
{
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    
    m_layout = new QVBoxLayout(m_centralWidget);
    m_layout->setSpacing(12);
    m_layout->setContentsMargins(30, 30, 30, 30);

    m_titleLabel = new QLabel("Управление Военкоматом", this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_layout->addWidget(m_titleLabel);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_layout->addWidget(m_statusLabel);

    m_tablesBtn = new QPushButton("📜 Список таблиц", this);
    connect(m_tablesBtn, &QPushButton::clicked, this, &MainWindow::openTablesWindow);
    m_layout->addWidget(m_tablesBtn);

    m_queriesBtn = new QPushButton("🔍 Выполнить запросы", this);
    connect(m_queriesBtn, &QPushButton::clicked, this, &MainWindow::openQueriesWindow);
    m_layout->addWidget(m_queriesBtn);

    m_exportBtn = new QPushButton("📤 Экспорт всей БД", this);
    connect(m_exportBtn, &QPushButton::clicked, this, &MainWindow::exportAllData);
    m_layout->addWidget(m_exportBtn);

    m_restoreBtn = new QPushButton("📥 Восстановить базу", this);
    connect(m_restoreBtn, &QPushButton::clicked, this, &MainWindow::restoreFromBackup);
    m_layout->addWidget(m_restoreBtn);

    m_restoreTableBtn = new QPushButton("🔄 Восстановить таблицу", this);
    connect(m_restoreTableBtn, &QPushButton::clicked, this, &MainWindow::restoreTableFromBackup);
    m_layout->addWidget(m_restoreTableBtn);

    m_layout->addStretch();

    m_exitBtn = new QPushButton("Выход", this);
    connect(m_exitBtn, &QPushButton::clicked, this, &MainWindow::close);
    m_layout->addWidget(m_exitBtn);
}

void MainWindow::setupStyles()
{
    m_titleLabel->setStyleSheet("font-size: 22px; font-weight: bold; color: #2c3e50; margin-bottom: 10px;");

    setStyleSheet(
        "QMainWindow { background-color: #f0f3f5; }"
        "QPushButton { "
        "   background-color: #2c3e50; color: white; border-radius: 6px; "
        "   padding: 12px; font-size: 15px; font-weight: bold; border: none; "
        "}"
        "QPushButton:hover { background-color: #34495e; }"
        "QPushButton:pressed { background-color: #1a252f; }"
        "QPushButton#exitBtn { background-color: #c0392b; }"
        "QPushButton#exitBtn:hover { background-color: #e74c3c; }"
    );
    m_exitBtn->setObjectName("exitBtn");
}

bool MainWindow::connectToDatabase()
{
    ConfigManager config("config.ini");
    if (!config.configFileExists()) config.createDefaultConfig();

    m_dbManager->setHttpMode(config.isHttpMode());
    
    return m_dbManager->connectToDatabase(
        config.getDatabaseHost(),
        config.getDatabasePort(),
        config.getDatabaseName(),
        config.getDatabaseUsername(),
        config.getDatabasePassword()
    );
}

void MainWindow::openQueriesWindow()
{
    if (!m_queriesWindow) {
        m_queriesWindow = new QueriesWindow(m_dbManager, this);
        m_queriesWindow->setWindowFlags(Qt::Window);
    }
    m_queriesWindow->show();
}

void MainWindow::openTablesWindow()
{
    if (!m_tablesWindow) {
        m_tablesWindow = new TablesWindow(m_dbManager, this);
        m_tablesWindow->setWindowFlags(Qt::Window);
    }
    m_tablesWindow->show();
}

void MainWindow::exportAllData()
{
    BackupManager backupManager(m_dbManager);
    if (backupManager.exportAllTables()) {
        QMessageBox::information(this, "Успех", "Данные успешно экспортированы.");
    } else {
        QMessageBox::critical(this, "Ошибка", backupManager.lastError());
    }
}

void MainWindow::restoreFromBackup()
{
    BackupManager backupManager(m_dbManager);
    QString fileName = QFileDialog::getOpenFileName(this, "Выберите файл", "", "SQL Files (*.sql)");
    if (!fileName.isEmpty() && backupManager.restoreFromBackup(fileName)) {
        QMessageBox::information(this, "Успех", "База успешно восстановлена.");
    }
}

void MainWindow::restoreTableFromBackup()
{
    BackupManager backupManager(m_dbManager);
    QString fileName = QFileDialog::getOpenFileName(this, "Выберите файл", "", "SQL Files (*.sql)");
    if (!fileName.isEmpty() && backupManager.restoreTableFromBackup(fileName)) {
        QMessageBox::information(this, "Успех", "Таблица успешно восстановлена.");
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event) { QMainWindow::keyPressEvent(event); }
void MainWindow::viewData() {}
void MainWindow::addRecord() {}
void MainWindow::updateRecord() {}
void MainWindow::deleteRecord() {}
void MainWindow::openQueries() {}
void MainWindow::saveQueryResult() {}
void MainWindow::createBackup() {}
void MainWindow::exitApp() { close(); }
void MainWindow::applyFilter() {}
void MainWindow::onTableSelected(const QString &tableName) { Q_UNUSED(tableName); }
void MainWindow::switchMode() {}
void MainWindow::setupClassicUI() {}
void MainWindow::setupMenus() {}
void MainWindow::setupShortcuts() {}
void MainWindow::loadTablesMenu() {}
