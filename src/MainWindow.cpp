#include "MainWindow.h"
#include "TablesWindow.h"
#include "QueriesWindow.h"
#include "BackupManager.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QApplication>
#include <QHBoxLayout>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_dbManager(new DatabaseManager(this))
    , m_tablesWindow(nullptr)
    , m_queriesWindow(nullptr)
{
    setWindowTitle("Военкомат - Главное меню");
    setGeometry(100, 100, 400, 500);
    
    setupUI();
    setupStyles();
    
    if (!connectToDatabase()) {
        updateConnectionStatus();
        QString errorDetails = m_dbManager->lastError();
        QString message;

        if (m_dbManager->isHttpMode()) {
            message = QString("Не удалось подключиться к Python серверу.\n\n")
                             + QString("Ошибка: %1\n\n").arg(errorDetails.isEmpty() ? "Сервер не отвечает" : errorDetails)
                             + QString("Проверьте:\n")
                             + QString("1. Запущен ли скрипт main.py (FastAPI)\n")
                             + QString("2. Правильность адреса/порта в config.ini\n")
                             + QString("3. Не блокирует ли брандмауэр порт 8000");
        } else {
            message = QString("Не удалось подключиться к базе данных напрямую.\n\n")
                             + QString("Ошибка: %1\n\n").arg(errorDetails.isEmpty() ? "Неизвестная ошибка" : errorDetails)
                             + QString("Проверьте:\n")
                             + QString("1. Запущен ли PostgreSQL сервер\n")
                             + QString("2. Параметры подключения в config.ini\n")
                             + QString("3. Наличие драйвера QPSQL");
        }
        
        QMessageBox::critical(this, "Ошибка подключения", message);
    } else {
        updateConnectionStatus();
    }
}

void MainWindow::updateConnectionStatus()
{
    if (!m_statusLabel) return;
    
    if (m_dbManager && m_dbManager->isConnected()) {
        QString mode = m_dbManager->isHttpMode() ? " (API)" : " (SQL)";
        m_statusLabel->setText("✓ Подключено" + mode);
        m_statusLabel->setStyleSheet("font-size: 12px; padding: 5px; background-color: #90EE90; color: black; border-radius: 4px;");
    } else {
        m_statusLabel->setText("✗ Не подключено");
        m_statusLabel->setStyleSheet("font-size: 12px; padding: 5px; background-color: #FFB6C1; color: black; border-radius: 4px;");
    }
}

MainWindow::~MainWindow()
{
    if (m_tablesWindow) { m_tablesWindow->close(); m_tablesWindow->deleteLater(); }
    if (m_queriesWindow) { m_queriesWindow->close(); m_queriesWindow->deleteLater(); }
}

void MainWindow::setupUI()
{
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    
    m_layout = new QVBoxLayout(m_centralWidget);
    m_layout->setSpacing(15);
    m_layout->setContentsMargins(20, 20, 20, 20);

    m_titleLabel = new QLabel("Военкомат", this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet("font-size: 24px; color: #333; font-weight: bold;");
    m_layout->addWidget(m_titleLabel);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_layout->addWidget(m_statusLabel);

    m_queriesBtn = new QPushButton("Запросы", this);
    m_queriesBtn->setMinimumHeight(40);
    connect(m_queriesBtn, &QPushButton::clicked, this, &MainWindow::openQueriesWindow);
    m_layout->addWidget(m_queriesBtn);

    m_tablesBtn = new QPushButton("Таблицы", this);
    m_tablesBtn->setMinimumHeight(40);
    connect(m_tablesBtn, &QPushButton::clicked, this, &MainWindow::openTablesWindow);
    m_layout->addWidget(m_tablesBtn);

    m_exportBtn = new QPushButton("Экспорт данных", this);
    m_exportBtn->setMinimumHeight(40);
    connect(m_exportBtn, &QPushButton::clicked, this, &MainWindow::exportAllData);
    m_layout->addWidget(m_exportBtn);

    m_restoreBtn = new QPushButton("Восстановить данные", this);
    m_restoreBtn->setMinimumHeight(40);
    connect(m_restoreBtn, &QPushButton::clicked, this, &MainWindow::restoreFromBackup);
    m_layout->addWidget(m_restoreBtn);

    m_restoreTableBtn = new QPushButton("Восстановить таблицу", this);
    m_restoreTableBtn->setMinimumHeight(40);
    connect(m_restoreTableBtn, &QPushButton::clicked, this, &MainWindow::restoreTableFromBackup);
    m_layout->addWidget(m_restoreTableBtn);

    m_layout->addStretch();

    m_exitBtn = new QPushButton("Выйти", this);
    m_exitBtn->setMinimumHeight(40);
    connect(m_exitBtn, &QPushButton::clicked, this, &QWidget::close);
    m_layout->addWidget(m_exitBtn);
}

void MainWindow::setupStyles()
{
    setStyleSheet(
        "QWidget { background-color: #dbffff; }"
        "QPushButton { background-color: #FFB6C1; font-size: 16px; padding: 10px; border-radius: 8px; color: black; border: none; min-height: 40px; }"
        "QPushButton:hover { background-color: #FF69B4; }"
        "QPushButton#exitBtn { background-color: #E0B0FF; }"
    );
    m_exitBtn->setObjectName("exitBtn");
}

bool MainWindow::connectToDatabase()
{
    ConfigManager config;
    if (!config.configFileExists()) config.createDefaultConfig();

    m_dbManager->setHttpMode(config.isHttpMode());
    
    QString host = config.getDatabaseHost();
    QString port = config.getDatabasePort();

    if (m_dbManager->isHttpMode() && port == "5432") {
        port = "8000";
    }

    return m_dbManager->connectToDatabase(
        host,
        port,
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
    if (m_dbManager->isHttpMode()) {
        QMessageBox::information(this, "Экспорт", "В режиме API экспорт выполняется через серверный эндпоинт.");
        return;
    }
    BackupManager backupManager(m_dbManager);
    if (backupManager.exportAllTables()) {
        QMessageBox::information(this, "Успех", "Данные успешно экспортированы.");
    } else {
        QMessageBox::critical(this, "Ошибка", backupManager.lastError());
    }
}

void MainWindow::restoreFromBackup()
{
    if (m_dbManager->isHttpMode()) {
        QMessageBox::warning(this, "Ограничение", "Восстановление из бэкапа пока не поддерживается в режиме API.");
        return;
    }
    BackupManager backupManager(m_dbManager);
    QString fileName = QFileDialog::getOpenFileName(this, "Выберите файл", "", "SQL Files (*.sql)");
    if (!fileName.isEmpty() && backupManager.restoreFromBackup(fileName)) {
        QMessageBox::information(this, "Успех", "База восстановлена.");
    }
}

void MainWindow::restoreTableFromBackup()
{
    if (m_dbManager->isHttpMode()) {
        QMessageBox::warning(this, "Ограничение", "Восстановление таблицы пока не поддерживается в режиме API.");
        return;
    }
    BackupManager backupManager(m_dbManager);
    QString fileName = QFileDialog::getOpenFileName(this, "Выберите файл", "", "SQL Files (*.sql)");
    if (!fileName.isEmpty() && backupManager.restoreTableFromBackup(fileName)) {
        QMessageBox::information(this, "Успех", "Таблица восстановлена.");
    }
}
