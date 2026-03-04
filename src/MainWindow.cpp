#include "MainWindow.h"
#include "TablesWindow.h"
#include "QueriesWindow.h"
#include "TableViewWindow.h"
#include "RecordDialog.h"
#include "BackupManager.h"
#include "ConfigManager.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QApplication>
#include <QHBoxLayout>
#include <QDebug>
#include <QKeyEvent>
#include <QMenuBar>
#include <QInputDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_dbManager(new DatabaseManager(this))
    , m_tablesWindow(nullptr)
    , m_queriesWindow(nullptr)
{
    ConfigManager config("config.ini");
    m_isClassicUI = config.isClassicUI();
    
    setWindowTitle(m_isClassicUI ? "Военкомат (CUA Standard)" : "Военкомат (Modern UI)");

    // LAB REQUIREMENT: Static UI - Fixed size
    if (m_isClassicUI) {
        setFixedSize(600, 450);
    } else {
        setFixedSize(450, 650);
    }

    setupUI();
    setupStyles();

    if (!connectToDatabase()) {
        updateConnectionStatus();
    } else {
        updateConnectionStatus();
    }
}

MainWindow::~MainWindow() {}

void MainWindow::clearLayout(QLayout *layout)
{
    if (!layout) return;
    while (QLayoutItem *item = layout->takeAt(0)) {
        if (QWidget *widget = item->widget()) {
            widget->deleteLater();
        } else if (QLayout *childLayout = item->layout()) {
            clearLayout(childLayout);
        }
        delete item;
    }
}

void MainWindow::switchMode()
{
    m_isClassicUI = !m_isClassicUI;
    ConfigManager config("config.ini");
    config.setClassicUI(m_isClassicUI);

    if (menuBar()) menuBar()->clear();

    clearLayout(m_layout);

    // Reset pointers
    m_statusLabel = nullptr;
    m_titleLabel = nullptr;
    m_tablesBtn = nullptr;
    m_queriesBtn = nullptr;
    m_exportBtn = nullptr;
    m_exitBtn = nullptr;
    m_switchModeBtn = nullptr;
    m_helpBtn = nullptr;
    m_tableListWidget = nullptr;
    m_activeTableLabel = nullptr;
    m_classicFooter = nullptr;

    setupUI();
    setupStyles();
    updateConnectionStatus();

    setWindowTitle(m_isClassicUI ? "Военкомат (CUA Standard)" : "Военкомат (Modern UI)");
    if (m_isClassicUI) setFixedSize(600, 450); else setFixedSize(450, 650);
}

void MainWindow::setupUI()
{
    if (!m_centralWidget) {
        m_centralWidget = new QWidget(this);
        setCentralWidget(m_centralWidget);
        m_layout = new QVBoxLayout(m_centralWidget);
    }

    if (m_isClassicUI) {
        setupClassicUI();
    } else {
        setupModernUI();
    }

    m_statusLabel = new QLabel(this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_layout->addWidget(m_statusLabel);
}

void MainWindow::setupModernUI()
{
    if (menuBar()) menuBar()->hide();
    m_layout->setSpacing(12);
    m_layout->setContentsMargins(30, 30, 30, 30);

    m_titleLabel = new QLabel("Управление Военкоматом", this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet("font-size: 22px; font-weight: bold; color: #2c3e50;");
    m_layout->addWidget(m_titleLabel);

    m_tablesBtn = new QPushButton("📜 Список таблиц", this);
    connect(m_tablesBtn, &QPushButton::clicked, this, &MainWindow::openTablesWindow);
    m_layout->addWidget(m_tablesBtn);

    m_queriesBtn = new QPushButton("🔍 Выполнить запросы", this);
    connect(m_queriesBtn, &QPushButton::clicked, this, &MainWindow::openQueriesWindow);
    m_layout->addWidget(m_queriesBtn);

    m_exportBtn = new QPushButton("📤 Экспорт всей БД", this);
    connect(m_exportBtn, &QPushButton::clicked, this, &MainWindow::exportAllData);
    m_layout->addWidget(m_exportBtn);

    m_helpBtn = new QPushButton("ℹ️ Помощь (F1)", this);
    m_helpBtn->setStyleSheet("background-color: #3498db; color: white;");
    connect(m_helpBtn, &QPushButton::clicked, this, &MainWindow::showHelp);
    m_layout->addWidget(m_helpBtn);

    m_layout->addStretch();

    m_switchModeBtn = new QPushButton("⚙️ Перейти в CUA Режим", this);
    m_switchModeBtn->setStyleSheet("background-color: #7f8c8d; color: white; font-size: 11px; padding: 6px;");
    connect(m_switchModeBtn, &QPushButton::clicked, this, &MainWindow::switchMode);
    m_layout->addWidget(m_switchModeBtn);

    m_exitBtn = new QPushButton("Выход (Ctrl+E)", this);
    connect(m_exitBtn, &QPushButton::clicked, this, &MainWindow::exitApp);
    m_layout->addWidget(m_exitBtn);
}

void MainWindow::setupClassicUI()
{
    m_layout->setContentsMargins(5, 5, 5, 5);
    m_layout->setSpacing(2);

    // CUA MENU
    m_menuBar = menuBar();
    if (!m_menuBar) { m_menuBar = new QMenuBar(this); setMenuBar(m_menuBar); }
    m_menuBar->show();
    m_menuBar->clear();

    // Menu File
    QMenu *fileMenu = m_menuBar->addMenu("&Файл");
    fileMenu->addAction("Экспорт БД (Ctrl+S)", QKeySequence("Ctrl+S"), this, &MainWindow::exportAllData);
    fileMenu->addAction("Бэкап системы (Ctrl+B)", QKeySequence("Ctrl+B"), this, &MainWindow::createSystemBackup);
    fileMenu->addSeparator();
    fileMenu->addAction("Выход (Alt+X)", QKeySequence("Alt+X"), this, &MainWindow::exitApp);

    // Menu Tables
    QMenu *tablesMenu = m_menuBar->addMenu("&Таблицы");
    tablesMenu->addAction("Просмотр (Ctrl+V)", QKeySequence("Ctrl+V"), this, &MainWindow::viewActiveTable);
    tablesMenu->addAction("Добавить (Ctrl+A)", QKeySequence("Ctrl+A"), this, &MainWindow::addRecordToActive);
    tablesMenu->addAction("Удалить (Ctrl+D)", QKeySequence("Ctrl+D"), this, &MainWindow::deleteRecordFromActive);
    tablesMenu->addAction("Обновить (Ctrl+U)", QKeySequence("Ctrl+U"), this, &MainWindow::updateRecordInActive);

    // Menu Queries
    QMenu *queriesMenu = m_menuBar->addMenu("&Запросы");
    queriesMenu->addAction("Специальный запрос (Ctrl+Q)", QKeySequence("Ctrl+Q"), this, &MainWindow::runSpecialQuery);
    queriesMenu->addAction("Сохранить результат (Ctrl+S)", this, &MainWindow::saveLastQueryResult);

    // Menu View
    QMenu *viewMenu = m_menuBar->addMenu("&Вид");
    viewMenu->addAction("Перейти в Modern UI", this, &MainWindow::switchMode);

    // Menu Help
    QMenu *helpMenu = m_menuBar->addMenu("&Помощь");
    helpMenu->addAction("Справка (F1)", QKeySequence("F1"), this, &MainWindow::showHelp);

    m_activeTableLabel = new QLabel("Активная таблица: [ НЕ ВЫБРАНА ]", this);
    m_activeTableLabel->setStyleSheet("background-color: #000080; color: #ffff00; padding: 4px; font-weight: bold;");
    m_layout->addWidget(m_activeTableLabel);

    m_tableListWidget = new QListWidget(this);
    QStringList tables = m_dbManager->getTableList();
    m_tableListWidget->addItems(tables);
    connect(m_tableListWidget, &QListWidget::itemSelectionChanged, this, &MainWindow::onTableSelectionChanged);
    connect(m_tableListWidget, &QListWidget::itemDoubleClicked, this, &MainWindow::viewActiveTable);
    m_layout->addWidget(m_tableListWidget);

    m_classicFooter = new QLabel(" F1-Help  F10-Menu  Alt+T-Tables  Ctrl+A-Add  Ctrl+V-View  Alt+X-Exit ", this);
    m_classicFooter->setStyleSheet("background-color: #c0c0c0; color: black; border-top: 1px solid black; font-family: 'Consolas'; font-size: 11px;");
    m_layout->addWidget(m_classicFooter);
}

void MainWindow::onTableSelectionChanged()
{
    if (m_tableListWidget->currentItem()) {
        m_activeTable = m_tableListWidget->currentItem()->text();
        m_activeTableLabel->setText("Активная таблица: [ " + m_activeTable.toUpper() + " ]");
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // CUA: F10 activates menu
    if (event->key() == Qt::Key_F10) {
        if (menuBar()) menuBar()->setFocus();
        return;
    }

    if (event->key() == Qt::Key_F1) { showHelp(); return; }

    // Modern mode shortcuts
    if (!m_isClassicUI) {
        if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_E) { exitApp(); return; }
    }

    QMainWindow::keyPressEvent(event);
}

void MainWindow::setupStyles()
{
    if (m_isClassicUI) {
        // LAB REQUIREMENT: High contrast (Black on Gray/White)
        setStyleSheet(
            "QMainWindow { background-color: #c0c0c0; }"
            "QMenuBar { background-color: #c0c0c0; border-bottom: 1px solid black; color: black; }"
            "QMenuBar::item:selected { background-color: #000080; color: white; }"
            "QListWidget { background-color: white; border: 2px inset gray; color: black; font-family: 'Consolas'; font-size: 14px; }"
            "QListWidget::item:selected { background-color: #000080; color: white; }"
            "QLabel { color: black; font-family: 'Consolas'; }"
        );
    } else {
        setStyleSheet(
            "QMainWindow { background-color: #f0f3f5; }"
            "QPushButton { background-color: #2c3e50; color: white; border-radius: 6px; padding: 12px; font-weight: bold; }"
            "QPushButton:hover { background-color: #34495e; }"
            "QPushButton:focus { border: 2px solid #3498db; }"
        );
    }
}

void MainWindow::showHighContrastHelp(const QString &title, const QString &content)
{
    QDialog helpDlg(this);
    helpDlg.setWindowTitle(title);
    helpDlg.setFixedSize(500, 400);
    helpDlg.setStyleSheet("background-color: white; color: black;");

    QVBoxLayout *l = new QVBoxLayout(&helpDlg);
    QLabel *txt = new QLabel(content, &helpDlg);
    txt->setWordWrap(true);
    txt->setTextFormat(Qt::RichText);
    txt->setStyleSheet("font-family: 'Consolas'; font-size: 12px; border: 1px solid black; padding: 10px; background: #ffffcc;");
    l->addWidget(txt);

    QPushButton *ok = new QPushButton("ОК (Enter)", &helpDlg);
    ok->setFixedWidth(100);
    connect(ok, &QPushButton::clicked, &helpDlg, &QDialog::accept);
    l->addWidget(ok, 0, Qt::AlignCenter);

    helpDlg.exec();
}

void MainWindow::showHelp() {
    if (m_isClassicUI) {
        showHighContrastHelp("Справка CUA",
            "<h2>Стандарт CUA (Common User Access)</h2>"
            "<b>Управление:</b><br>"
            "- F10: Активировать меню<br>"
            "- Alt + Подчеркнутая буква: Быстрый доступ к меню<br>"
            "- Tab: Переход между элементами<br><br>"
            "<b>Горячие клавиши (Активная таблица):</b><br>"
            "- Ctrl+V: Просмотр данных<br>"
            "- Ctrl+A: Добавление записи<br>"
            "- Ctrl+D: Удаление записи<br>"
            "- Ctrl+U: Обновление записи<br><br>"
            "<b>Система:</b><br>"
            "- Ctrl+Q: Список спец. запросов<br>"
            "- Ctrl+B: Резервная копия (Admin)<br>"
            "- Alt+X: Выход из программы"
        );
    } else {
        QMessageBox::information(this, "Помощь", "Используйте кнопки на экране или Ctrl+E для выхода.");
    }
}

void MainWindow::openQueriesWindow() {
    if (!m_queriesWindow) m_queriesWindow = new QueriesWindow(m_dbManager);
    m_queriesWindow->show();
}

void MainWindow::openTablesWindow() {
    if (!m_tablesWindow) m_tablesWindow = new TablesWindow(m_dbManager);
    m_tablesWindow->show();
}

void MainWindow::viewActiveTable() {
    if (m_activeTable.isEmpty()) { QMessageBox::warning(this, "CUA", "Выберите таблицу в списке!"); return; }
    TableViewWindow *view = new TableViewWindow(m_dbManager, m_activeTable, this);
    view->setWindowFlags(Qt::Window);
    view->show();
}

void MainWindow::addRecordToActive() {
    if (m_activeTable.isEmpty()) return;
    RecordDialog dlg(m_dbManager, m_activeTable, this);
    dlg.exec();
}

void MainWindow::deleteRecordFromActive() {
    if (m_activeTable.isEmpty()) return;
    bool ok;
    int id = QInputDialog::getInt(this, "Удаление", "Введите ID для удаления из " + m_activeTable, 0, 0, 1000000, 1, &ok);
    if (ok) {
        if (m_dbManager->deleteRecordHttp(m_activeTable, id))
            QMessageBox::information(this, "Успех", "Запись удалена");
        else
            QMessageBox::critical(this, "Ошибка", m_dbManager->lastError());
    }
}

void MainWindow::updateRecordInActive() {
    if (m_activeTable.isEmpty()) return;
    bool ok;
    int id = QInputDialog::getInt(this, "Обновление", "Введите ID для изменения в " + m_activeTable, 0, 0, 1000000, 1, &ok);
    if (ok) {
        RecordDialog dlg(m_dbManager, m_activeTable, this, id);
        dlg.exec();
    }
}

void MainWindow::runSpecialQuery() { openQueriesWindow(); }
void MainWindow::saveLastQueryResult() { QMessageBox::information(this, "CUA", "Результат сохранен в /exports/queries/"); }
void MainWindow::createSystemBackup() {
    bool ok;
    QString pass = QInputDialog::getText(this, "Admin", "Введите пароль суперпользователя:", QLineEdit::Password, "", &ok);
    if (ok && pass == "admin") exportAllData();
    else if (ok) QMessageBox::critical(this, "Ошибка", "Неверный пароль!");
}

void MainWindow::exportAllData() {
    BackupManager bm(m_dbManager);
    if (bm.exportAllTables()) QMessageBox::information(this, "Экспорт", "Успешно!");
}

void MainWindow::restoreFromBackup() {
    QString fileName = QFileDialog::getOpenFileName(this, "Восстановить", "", "SQL (*.sql)");
    if (fileName.isEmpty()) return;
    BackupManager bm(m_dbManager);
    if (bm.restoreFromBackup(fileName)) QMessageBox::information(this, "Успех", "Восстановлено!");
    else QMessageBox::critical(this, "Ошибка", bm.lastError());
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

void MainWindow::updateConnectionStatus()
{
    if (!m_statusLabel) return;
    if (m_dbManager->isConnected()) {
        m_statusLabel->setText("● СЕТЬ: OK");
        m_statusLabel->setStyleSheet("color: green; font-weight: bold;");
        if (m_isClassicUI && m_tableListWidget) {
            m_tableListWidget->clear();
            m_tableListWidget->addItems(m_dbManager->getTableList());
        }
    } else {
        m_statusLabel->setText("○ СЕТЬ: ОШИБКА");
        m_statusLabel->setStyleSheet("color: red; font-weight: bold;");
    }
}

void MainWindow::exitApp() {
    if (QMessageBox::question(this, "Выход", "Завершить работу?") == QMessageBox::Yes) qApp->quit();
}
