/**
 * @file MainWindow.cpp
 * @brief ГЛАВНОЕ ОКНО СИСТЕМЫ "ВОЕНКОМАТ"
 *
 * СПРАВКА ПО УПРАВЛЕНИЮ (CUA Standard):
 * ------------------------------------------------------------
 * НАВИГАЦИЯ:
 * - Стрелки: Перемещение между кнопками/строками
 * - Tab: Переход к следующему элементу
 * - Enter: Нажать кнопку (OK)
 * - Esc: Закрыть/Отмена
 *
 * ГОРЯЧИЕ КЛАВИШИ:
 * - F1: Вызов справки
 * - Ctrl+V: Просмотр данных
 * - Ctrl+A: Добавить запись
 * - Ctrl+D: Удалить запись (по ID)
 * - Ctrl+B: Бэкап (Admin)
 * - Ctrl+E: Выход
 * ------------------------------------------------------------
 */

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
    setWindowTitle("Военкомат - Главное меню (Клавиатурное управление)");
    resize(450, 650);
    
    setupUI();
    setupStyles();
    
    // Включаем фокус для клавиатуры
    setFocusPolicy(Qt::StrongFocus);

    if (!connectToDatabase()) {
        updateConnectionStatus();
        QMessageBox::critical(this, "Ошибка", "Не удалось подключиться к БД.");
    } else {
        updateConnectionStatus();
    }
}

void MainWindow::updateConnectionStatus()
{
    if (!m_statusLabel) return;
    QString mode = m_dbManager->isHttpMode() ? " (API)" : " (SQL)";

    if (m_dbManager->isConnected()) {
        m_statusLabel->setText("● Подключено" + mode);
        m_statusLabel->setStyleSheet("color: #27ae60; font-weight: bold;");
    } else {
        m_statusLabel->setText("○ Отключено");
        m_statusLabel->setStyleSheet("color: #e74c3c; font-weight: bold;");
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
    m_titleLabel->setStyleSheet("font-size: 22px; font-weight: bold; color: #2c3e50;");
    m_layout->addWidget(m_titleLabel);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_layout->addWidget(m_statusLabel);

    // Подсказка для пользователя в интерфейсе
    QLabel *helpHint = new QLabel("Нажмите [F1] для справки по клавишам", this);
    helpHint->setAlignment(Qt::AlignCenter);
    helpHint->setStyleSheet("color: #7f8c8d; font-style: italic; font-size: 11px;");
    m_layout->addWidget(helpHint);

    m_tablesBtn = new QPushButton("📜 Список таблиц", this);
    m_tablesBtn->setFocus(); // Фокус на первую кнопку
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

    m_exitBtn = new QPushButton("Выход (Ctrl+E)", this);
    connect(m_exitBtn, &QPushButton::clicked, this, &MainWindow::close);
    m_layout->addWidget(m_exitBtn);
}

void MainWindow::setupStyles()
{
    setStyleSheet(
        "QMainWindow { background-color: #f0f3f5; }"
        "QPushButton { "
        "   background-color: #2c3e50; color: white; border-radius: 6px; "
        "   padding: 12px; font-size: 15px; font-weight: bold; border: 2px solid transparent; "
        "}"
        "QPushButton:focus { border: 2px solid #3498db; background-color: #34495e; }"
        "QPushButton:hover { background-color: #34495e; }"
        "QPushButton#exitBtn { background-color: #c0392b; }"
    );
    m_exitBtn->setObjectName("exitBtn");
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // Логирование всех нажатий для отладки
    qDebug() << "[DEBUG] Key pressed:" << event->key() << "(" << event->text() << ")";

    // Глобальные горячие клавиши
    if (event->modifiers() & Qt::ControlModifier) {
        switch (event->key()) {
            case Qt::Key_E: close(); return;
            case Qt::Key_A: addRecord(); return;
            case Qt::Key_V: viewData(); return;
            case Qt::Key_B: createBackup(); return;
        }
    }

    // Обработка F1 (Помощь)
    if (event->key() == Qt::Key_F1) {
        QMessageBox::information(this, "Справка",
            "Управление клавиатурой:\n"
            "- Tab / Стрелки: Переход между кнопками\n"
            "- Enter / Space: Нажать кнопку\n"
            "- Ctrl+E: Выход\n"
            "- Ctrl+B: Бэкап БД");
        return;
    }

    // ИСПРАВЛЕНИЕ ENTER: Если нажата Enter на кнопке, имитируем клик
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        QWidget *focused = focusWidget();
        QPushButton *btn = qobject_cast<QPushButton*>(focused);
        if (btn) {
            qDebug() << "[DEBUG] Enter triggered click on:" << btn->text();
            btn->click();
            return;
        }
    }

    QMainWindow::keyPressEvent(event);
}

bool MainWindow::connectToDatabase()
{
    ConfigManager config("config.ini");
    m_dbManager->setHttpMode(config.isHttpMode());
    return m_dbManager->connectToDatabase(
        config.getDatabaseHost(), config.getDatabasePort(),
        config.getDatabaseName(), config.getDatabaseUsername(),
        config.getDatabasePassword()
    );
}

void MainWindow::openQueriesWindow() {
    if (!m_queriesWindow) m_queriesWindow = new QueriesWindow(m_dbManager);
    m_queriesWindow->show();
}

void MainWindow::openTablesWindow() {
    if (!m_tablesWindow) m_tablesWindow = new TablesWindow(m_dbManager);
    m_tablesWindow->show();
}

void MainWindow::exportAllData() {
    BackupManager bm(m_dbManager);
    if (bm.exportAllTables()) QMessageBox::information(this, "Экспорт", "Успешно!");
}

void MainWindow::restoreFromBackup() { /* ... */ }
void MainWindow::restoreTableFromBackup() { /* ... */ }
void MainWindow::viewData() { openTablesWindow(); }
void MainWindow::addRecord() { /* ... */ }
void MainWindow::updateRecord() {}
void MainWindow::deleteRecord() {}
void MainWindow::openQueries() { openQueriesWindow(); }
void MainWindow::saveQueryResult() {}
void MainWindow::createBackup() { exportAllData(); }
void MainWindow::exitApp() { close(); }
void MainWindow::applyFilter() {}
void MainWindow::onTableSelected(const QString&) {}
void MainWindow::switchMode() {}
void MainWindow::setupClassicUI() {}
void MainWindow::setupMenus() {}
void MainWindow::setupShortcuts() {}
void MainWindow::loadTablesMenu() {}
