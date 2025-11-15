#include "MainWindow.h"
#include "TablesWindow.h"
#include "QueriesWindow.h"
#include "TableAdditionWindow.h"
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
    , m_tableAdditionWindow(nullptr)
{
    setWindowTitle("Военкомат - Главное меню");
    setGeometry(100, 100, 400, 500);
    
    setupUI();
    setupStyles();
    
    if (!connectToDatabase()) {
        updateConnectionStatus();
        QString errorDetails = m_dbManager->lastError();
        QString message = QString("Не удалось подключиться к базе данных.\n\n")
                         + QString("Ошибка: %1\n\n").arg(errorDetails.isEmpty() ? "Неизвестная ошибка" : errorDetails)
                         + QString("Проверьте:\n")
                         + QString("1. Запущен ли PostgreSQL сервер\n")
                         + QString("2. Правильность параметров в config.ini\n")
                         + QString("3. Установлена ли переменная окружения PGPASSWORD\n")
                         + QString("   (В PowerShell: $env:PGPASSWORD = \"ваш_пароль\")\n")
                         + QString("4. Доступность драйвера QPSQL в Qt");
        
        QMessageBox::critical(this, "Ошибка подключения", message);
    } else {
        updateConnectionStatus();
    }
}

void MainWindow::updateConnectionStatus()
{
    if (!m_statusLabel) {
        return;
    }
    
    if (m_dbManager && m_dbManager->isConnected()) {
        m_statusLabel->setText("✓ Подключено к базе данных");
        m_statusLabel->setStyleSheet("font-size: 12px; padding: 5px; background-color: #90EE90; color: black; border-radius: 4px;");
    } else {
        m_statusLabel->setText("✗ Не подключено к базе данных");
        m_statusLabel->setStyleSheet("font-size: 12px; padding: 5px; background-color: #FFB6C1; color: black; border-radius: 4px;");
    }
}

MainWindow::~MainWindow()
{
    // Явно закрываем и удаляем дочерние окна для корректного освобождения ресурсов
    if (m_tablesWindow) {
        m_tablesWindow->close();
        m_tablesWindow->deleteLater();
        m_tablesWindow = nullptr;
    }
    if (m_queriesWindow) {
        m_queriesWindow->close();
        m_queriesWindow->deleteLater();
        m_queriesWindow = nullptr;
    }
    if (m_tableAdditionWindow) {
        m_tableAdditionWindow->close();
        m_tableAdditionWindow->deleteLater();
        m_tableAdditionWindow = nullptr;
    }
}

void MainWindow::setupUI()
{
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    
    m_layout = new QVBoxLayout(m_centralWidget);
    m_layout->setSpacing(15);
    m_layout->setContentsMargins(20, 20, 20, 20);

    // Заголовок
    m_titleLabel = new QLabel("Военкомат", this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet("font-size: 24px; color: #333; font-weight: bold;");
    m_layout->addWidget(m_titleLabel);

    // Статус подключения к БД
    m_statusLabel = new QLabel(this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("font-size: 12px; padding: 5px;");
    m_layout->addWidget(m_statusLabel);

    // Кнопки
    m_queriesBtn = new QPushButton("Запросы", this);
    m_queriesBtn->setMinimumHeight(40);
    connect(m_queriesBtn, &QPushButton::clicked, this, &MainWindow::openQueriesWindow);
    m_layout->addWidget(m_queriesBtn);

    m_tablesBtn = new QPushButton("Таблицы", this);
    m_tablesBtn->setMinimumHeight(40);
    connect(m_tablesBtn, &QPushButton::clicked, this, &MainWindow::openTablesWindow);
    m_layout->addWidget(m_tablesBtn);

    m_addTableBtn = new QPushButton("Добавить таблицу", this);
    m_addTableBtn->setMinimumHeight(40);
    connect(m_addTableBtn, &QPushButton::clicked, this, &MainWindow::openTableAdditionWindow);
    m_layout->addWidget(m_addTableBtn);

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
        "QPushButton {"
        "    background-color: #FFB6C1;"
        "    font-size: 16px;"
        "    padding: 10px;"
        "    border-radius: 8px;"
        "    color: black;"
        "    border: none;"
        "}"
        "QPushButton:hover { background-color: #FF69B4; }"
        "QPushButton:pressed { background-color: #FF1493; }"
        "QPushButton#exitBtn {"
        "    background-color: #E0B0FF;"
        "}"
        "QPushButton#exitBtn:hover { background-color: #c770ff; }"
        "QPushButton#exitBtn:pressed { background-color: #a314ff; }"
    );
    m_exitBtn->setObjectName("exitBtn");
}

bool MainWindow::connectToDatabase()
{
    // Загружаем параметры подключения из конфигурационного файла
    ConfigManager config;
    
    // Если конфигурационный файл не существует, создаем его с значениями по умолчанию
    if (!config.configFileExists()) {
        if (!config.createDefaultConfig()) {
            qDebug() << "Не удалось создать конфигурационный файл";
        }
    }
    
    // Получаем параметры подключения из конфигурации
    QString host = config.getDatabaseHost();
    QString port = config.getDatabasePort();
    QString database = config.getDatabaseName();
    QString username = config.getDatabaseUsername();
    QString password = config.getDatabasePassword();
    
    // Подключаемся к базе данных
    return m_dbManager->connectToDatabase(host, port, database, username, password);
}

void MainWindow::openQueriesWindow()
{
    if (!m_queriesWindow) {
        m_queriesWindow = new QueriesWindow(m_dbManager, this);
        m_queriesWindow->setWindowFlags(Qt::Window);
    }
    m_queriesWindow->raise();
    m_queriesWindow->activateWindow();
    m_queriesWindow->show();
}

void MainWindow::openTablesWindow()
{
    if (!m_tablesWindow) {
        m_tablesWindow = new TablesWindow(m_dbManager, this);
        m_tablesWindow->setWindowFlags(Qt::Window);
    }
    m_tablesWindow->raise();
    m_tablesWindow->activateWindow();
    m_tablesWindow->show();
}

void MainWindow::openTableAdditionWindow()
{
    if (!m_tableAdditionWindow) {
        m_tableAdditionWindow = new TableAdditionWindow(m_dbManager, this);
        m_tableAdditionWindow->setWindowFlags(Qt::Window);
    }
    m_tableAdditionWindow->raise();
    m_tableAdditionWindow->activateWindow();
    m_tableAdditionWindow->show();
}

void MainWindow::exportAllData()
{
    BackupManager backupManager(m_dbManager);
    if (backupManager.exportAllTables()) {
        QString exportsDir = backupManager.getExportsDirectory();
        QMessageBox::information(this, "Успех", 
            QString("Данные успешно экспортированы в папку:\n%1").arg(exportsDir));
    } else {
        QMessageBox::critical(this, "Ошибка", "Не удалось экспортировать данные:\n" + backupManager.lastError());
    }
}

void MainWindow::restoreFromBackup()
{
    BackupManager backupManager(m_dbManager);
    QString exportsDir = backupManager.getExportsDirectory();
    
    QString fileName = QFileDialog::getOpenFileName(this,
        "Выберите файл резервной копии", exportsDir, "SQL Files (*.sql)");
    
    if (fileName.isEmpty()) {
        return;
    }

    int ret = QMessageBox::question(this, "Подтверждение восстановления",
        "Вы уверены, что хотите восстановить базу данных из резервной копии?\n"
        "Все текущие данные будут удалены!",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        if (backupManager.restoreFromBackup(fileName)) {
            QMessageBox::information(this, "Успех", "База данных успешно восстановлена!");
        } else {
            QMessageBox::critical(this, "Ошибка", "Не удалось восстановить данные:\n" + backupManager.lastError());
        }
    }
}

void MainWindow::restoreTableFromBackup()
{
    BackupManager backupManager(m_dbManager);
    QString exportsDir = backupManager.getExportsDirectory();
    
    QString fileName = QFileDialog::getOpenFileName(this,
        "Выберите файл резервной копии таблицы", exportsDir, "SQL Files (*.sql)");
    
    if (fileName.isEmpty()) {
        return;
    }

    int ret = QMessageBox::question(this, "Подтверждение восстановления",
        "Вы уверены, что хотите восстановить таблицу из резервной копии?",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        if (backupManager.restoreTableFromBackup(fileName)) {
            QMessageBox::information(this, "Успех", "Таблица успешно восстановлена!");
        } else {
            QMessageBox::critical(this, "Ошибка", "Не удалось восстановить таблицу:\n" + backupManager.lastError());
        }
    }
}

