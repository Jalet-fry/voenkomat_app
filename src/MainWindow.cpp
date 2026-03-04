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
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>
#include <QKeyEvent>
#include <QMenuBar>
#include <QInputDialog>
#include <QHeaderView>
#include <QComboBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QPushButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_dbManager(new DatabaseManager(this))
{
    ConfigManager config("config.ini");
    m_isClassicUI = config.isClassicUI();
    
    setWindowTitle(m_isClassicUI ? "ИС Военкомат (CUA)" : "ИС Военкомат (Modern UI)");
    if (m_isClassicUI) setFixedSize(950, 700); else setFixedSize(450, 720);

    setupUI();
    setupStyles();

    if (connectToDatabase()) {
        updateConnectionStatus();
        m_activeTable = "conscripts";
        if (m_isClassicUI) {
            refreshTablesMenu();
            viewActiveTable();
            if (m_filterColumnCombo) m_filterColumnCombo->setFocus();
        }
    }
}

MainWindow::~MainWindow() {}

void MainWindow::switchMode()
{
    m_isClassicUI = !m_isClassicUI;
    ConfigManager config("config.ini");
    config.setClassicUI(m_isClassicUI);

    if (menuBar()) menuBar()->clear();
    clearLayout(m_layout);

    m_statusLabel = nullptr; m_titleLabel = nullptr; m_tablesBtn = nullptr;
    m_queriesBtn = nullptr; m_exportBtn = nullptr; m_exitBtn = nullptr;
    m_switchModeBtn = nullptr; m_helpBtn = nullptr;
    m_mainTable = nullptr; m_filterColumnCombo = nullptr; m_filterValueEdit = nullptr;
    m_activeTableLabel = nullptr; m_classicFooter = nullptr; m_tablesMenu = nullptr;

    setupUI();
    setupStyles();
    updateConnectionStatus();

    if (m_isClassicUI) {
        setFixedSize(950, 700);
        refreshTablesMenu();
        viewActiveTable();
        if (m_filterColumnCombo) m_filterColumnCombo->setFocus();
    } else {
        setFixedSize(450, 720);
    }
}

void MainWindow::setupUI()
{
    if (!m_centralWidget) {
        m_centralWidget = new QWidget(this);
        setCentralWidget(m_centralWidget);
        m_layout = new QVBoxLayout(m_centralWidget);
    }
    if (m_isClassicUI) setupClassicUI(); else setupModernUI();

    m_statusLabel = new QLabel(this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_layout->addWidget(m_statusLabel);
}

void MainWindow::setupModernUI()
{
    if (menuBar()) menuBar()->hide();
    m_layout->setSpacing(10);
    m_layout->setContentsMargins(30, 20, 30, 20);

    m_titleLabel = new QLabel("Система Военкомат", this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #2f3640; padding: 10px;");
    m_layout->addWidget(m_titleLabel);

    QString btnBase = "QPushButton { background-color: #2f3640; color: #ffffff; border-radius: 6px; padding: 12px; font-weight: bold; } "
                      "QPushButton:hover { background-color: #353b48; }";

    m_tablesBtn = new QPushButton("📜 Список Таблиц", this);
    m_queriesBtn = new QPushButton("🔍 Спец. Запросы", this);
    m_exportBtn = new QPushButton("📤 Сохранить (Ctrl+S)", this);
    m_helpBtn = new QPushButton("ℹ️ Справка (F1)", this);
    m_switchModeBtn = new QPushButton("⚙️ Перейти в CUA", this);
    m_exitBtn = new QPushButton("Выход (Ctrl+E)", this);

    m_tablesBtn->setStyleSheet(btnBase);
    m_queriesBtn->setStyleSheet(btnBase);
    m_exportBtn->setStyleSheet(btnBase);
    m_helpBtn->setStyleSheet("background-color: #27ae60; color: #ffffff; border-radius: 6px; padding: 12px; font-weight: bold;");
    m_switchModeBtn->setStyleSheet(btnBase);
    m_exitBtn->setStyleSheet("background-color: #c0392b; color: #ffffff; border-radius: 6px; padding: 12px; font-weight: bold;");

    QPushButton *loginBtn = new QPushButton("🔐 Войти как Admin", this);
    loginBtn->setStyleSheet("background-color: #e67e22; color: #ffffff; font-weight: bold; padding: 12px; border-radius: 6px;");

    QPushButton *logoutBtn = new QPushButton("🔓 Выйти из Admin", this);
    logoutBtn->setStyleSheet("background-color: #7f8c8d; color: #ffffff; font-weight: bold; padding: 12px; border-radius: 6px;");

    m_layout->addWidget(m_tablesBtn);
    m_layout->addWidget(m_queriesBtn);
    m_layout->addWidget(m_exportBtn);
    m_layout->addWidget(loginBtn);
    m_layout->addWidget(logoutBtn);
    m_layout->addWidget(m_helpBtn);
    m_layout->addStretch();
    m_layout->addWidget(m_switchModeBtn);
    m_layout->addWidget(m_exitBtn);

    connect(m_tablesBtn, &QPushButton::clicked, this, &MainWindow::openTablesWindow);
    connect(m_queriesBtn, &QPushButton::clicked, this, &MainWindow::openQueriesWindow);
    connect(m_exportBtn, &QPushButton::clicked, this, &MainWindow::saveQueryResult);
    connect(m_helpBtn, &QPushButton::clicked, this, &MainWindow::showHelp);
    connect(m_switchModeBtn, &QPushButton::clicked, this, &MainWindow::switchMode);
    connect(m_exitBtn, &QPushButton::clicked, this, &MainWindow::exitApp);

    QString dialogStyle =
        "QDialog, QMessageBox, QInputDialog { background-color: #ffffff; border: 2px solid #2f3640; }"
        "QLabel { color: #000000; font-weight: bold; font-size: 14px; min-width: 350px; }"
        "QLineEdit { background-color: #ffffff; color: #000000; border: 2px solid #2f3640; padding: 8px; }"
        "QPushButton { background-color: #2f3640; color: #ffffff; font-weight: bold; padding: 8px 20px; border-radius: 4px; min-width: 100px; }";

    connect(loginBtn, &QPushButton::clicked, this, [this, dialogStyle](){
        QInputDialog dialog(this);
        dialog.setWindowTitle("Авторизация");
        dialog.setLabelText("Введите пароль суперпользователя:");
        dialog.setTextEchoMode(QLineEdit::Password);
        dialog.setStyleSheet(dialogStyle);
        if (dialog.exec() == QDialog::Accepted) {
            if (dialog.textValue() == "admin") {
                m_dbManager->setAuthToken("admin");
                updateConnectionStatus();
                if(m_isClassicUI) viewActiveTable();
                QMessageBox msg(this); msg.setStyleSheet(dialogStyle);
                msg.setWindowTitle("Успех"); msg.setText("Режим администратора включен!"); msg.setIcon(QMessageBox::Information); msg.exec();
            } else {
                QMessageBox msg(this); msg.setStyleSheet(dialogStyle);
                msg.setWindowTitle("Ошибка"); msg.setText("Неверный пароль!"); msg.setIcon(QMessageBox::Critical); msg.exec();
            }
        }
    });

    connect(logoutBtn, &QPushButton::clicked, this, [this, dialogStyle](){
        QMessageBox confirm(this);
        confirm.setWindowTitle("Выход");
        confirm.setText("Вы действительно хотите выйти из режима администратора?");
        confirm.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        confirm.setIcon(QMessageBox::Question);
        confirm.setStyleSheet(dialogStyle);
        if(confirm.button(QMessageBox::Yes)) confirm.button(QMessageBox::Yes)->setText("Да, выйти");
        if(confirm.button(QMessageBox::No)) confirm.button(QMessageBox::No)->setText("Отмена");
        if (confirm.exec() == QMessageBox::Yes) {
            m_dbManager->setAuthToken("");
            updateConnectionStatus();
            if(m_isClassicUI) viewActiveTable();
            QMessageBox msg(this); msg.setStyleSheet(dialogStyle);
            msg.setWindowTitle("Статус"); msg.setText("Вы перешли в режим Guest."); msg.setIcon(QMessageBox::Information); msg.exec();
        }
    });
}

void MainWindow::setupClassicUI()
{
    m_layout->setContentsMargins(5, 5, 5, 5);
    m_layout->setSpacing(4);
    m_menuBar = menuBar();
    if (!m_menuBar) { m_menuBar = new QMenuBar(this); setMenuBar(m_menuBar); }
    m_menuBar->show();
    m_menuBar->clear();

    QMenu *fileMenu = m_menuBar->addMenu("&File");
    fileMenu->addAction("Exit", QKeySequence("Ctrl+E"), this, &MainWindow::exitApp);

    m_tablesMenu = m_menuBar->addMenu("&Tables");
    refreshTablesMenu();

    QMenu *opsMenu = m_menuBar->addMenu("&Operations");
    opsMenu->addAction("View", QKeySequence("Ctrl+V"), this, &MainWindow::viewActiveTable);
    opsMenu->addAction("Add", QKeySequence("Ctrl+A"), this, &MainWindow::addRecord);
    opsMenu->addAction("Update", QKeySequence("Ctrl+U"), this, &MainWindow::updateRecord);
    opsMenu->addAction("Delete", QKeySequence("Ctrl+D"), this, &MainWindow::deleteRecord);
    opsMenu->addSeparator();
    opsMenu->addAction("Queries", QKeySequence("Ctrl+Q"), this, &MainWindow::openQueries);
    opsMenu->addAction("Save Query", QKeySequence("Ctrl+S"), this, &MainWindow::saveQueryResult);
    opsMenu->addAction("Backup", QKeySequence("Ctrl+B"), this, &MainWindow::createBackup);

    m_menuBar->addMenu("&View")->addAction("Modern UI", this, &MainWindow::switchMode);
    m_menuBar->addMenu("&Help")->addAction("Help Content", QKeySequence("F1"), this, &MainWindow::showHelp);

    QHBoxLayout *fL = new QHBoxLayout();
    m_filterColumnCombo = new QComboBox(this);
    m_filterValueEdit = new QLineEdit(this);
    m_applyFilterBtn = new QPushButton("Apply", this);
    m_applyFilterBtn->setFixedWidth(80);

    // НАСТРОЙКА ФОКУСА (CUA)
    m_filterColumnCombo->setFocusPolicy(Qt::StrongFocus);
    m_filterValueEdit->setFocusPolicy(Qt::StrongFocus);
    m_applyFilterBtn->setFocusPolicy(Qt::StrongFocus);

    connect(m_applyFilterBtn, &QPushButton::clicked, this, &MainWindow::applyFilter);
    connect(m_filterValueEdit, &QLineEdit::returnPressed, this, &MainWindow::applyFilter);

    fL->addWidget(new QLabel("Field:")); fL->addWidget(m_filterColumnCombo);
    fL->addWidget(m_filterValueEdit); fL->addWidget(m_applyFilterBtn);
    m_layout->addLayout(fL);

    m_activeTableLabel = new QLabel(this);
    m_layout->addWidget(m_activeTableLabel);

    m_mainTable = new QTableWidget(this);
    m_mainTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_mainTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_mainTable->setFocusPolicy(Qt::StrongFocus);
    m_mainTable->setTabKeyNavigation(false); // Tab ВЫХОДИТ из таблицы к фильтрам
    m_layout->addWidget(m_mainTable);

    // УСТАНОВКА ЦИКЛИЧЕСКОЙ НАВИГАЦИИ TAB
    setTabOrder(m_filterColumnCombo, m_filterValueEdit);
    setTabOrder(m_filterValueEdit, m_applyFilterBtn);
    setTabOrder(m_applyFilterBtn, m_mainTable);
    setTabOrder(m_mainTable, m_filterColumnCombo);

    m_classicFooter = new QLabel(" F1-Help | Tab-Navigation | F10-Menu | Alt+F/T/O-Shortcuts ", this);
    m_layout->addWidget(m_classicFooter);
}

void MainWindow::viewActiveTable()
{
    if (m_activeTable.isEmpty() || !m_dbManager->isConnected()) return;
    QStringList lookupTables = {"fitness_categories", "commissioners"};
    bool isLookup = lookupTables.contains(m_activeTable);
    bool canEdit = !isLookup || m_dbManager->isSuperuser();
    if (m_activeTableLabel) {
        QString status = canEdit ? "" : " (READ ONLY MODE)";
        m_activeTableLabel->setText("Active Table: [ " + m_activeTable.toUpper() + " ]" + status);
        m_activeTableLabel->setStyleSheet(canEdit ? "background-color: #2f3640; color: #ffffff; padding: 6px; font-weight: bold;"
                                                  : "background-color: #c0392b; color: #ffffff; padding: 6px; font-weight: bold;");
    }
    QStringList cols = m_dbManager->getColumnList(m_activeTable);
    if (m_filterColumnCombo) { m_filterColumnCombo->clear(); m_filterColumnCombo->addItems(cols); }
    if (m_mainTable) {
        m_mainTable->setColumnCount(cols.size());
        m_mainTable->setHorizontalHeaderLabels(cols);
        applyFilter();
    }
}

void MainWindow::applyFilter()
{
    if (!m_mainTable || m_activeTable.isEmpty()) return;
    QString where = "";
    if (m_filterColumnCombo && m_filterValueEdit && !m_filterValueEdit->text().isEmpty()) {
        QString val = m_filterValueEdit->text().trimmed();
        QString col = m_filterColumnCombo->currentText();
        if (val.startsWith(">") || val.startsWith("<") || val.startsWith("=")) where = QString("%1 %2").arg(col).arg(val);
        else where = QString("%1::text ILIKE '%%2%'").arg(col).arg(val);
    }
    QJsonArray data = m_dbManager->isHttpMode() ? m_dbManager->fetchTableDataHttp(m_activeTable, where) : QJsonArray();
    m_mainTable->setRowCount(data.size());
    QStringList cols = m_dbManager->getColumnList(m_activeTable);
    for(int i=0; i<data.size(); ++i) {
        QJsonObject o = data[i].toObject();
        for(int j=0; j<cols.size(); ++j) {
            QTableWidgetItem *it = new QTableWidgetItem(o[cols[j]].toVariant().toString());
            it->setForeground(QBrush(Qt::black));
            m_mainTable->setItem(i, j, it);
        }
    }
}

void MainWindow::showHelp() {
    QMessageBox helpBox(this);
    helpBox.setWindowTitle("Справка по системе (CUA Standard)");
    QString h =
        "<div style='background-color: #ffffff; color: #000000; padding: 15px; font-family: Segoe UI;'>"
        "<h2 style='color: #2f3640;'>Руководство оператора (Лаб №1 и №2)</h2>"
        "<p>Программа построена по стандарту <b>Common User Access (CUA)</b>. Основное управление клавиатурное.</p>"
        "<h3>1. Горячие клавиши (Hotkeys):</h3>"
        "<table border='1' cellpadding='5' style='border-collapse: collapse; width: 100%;'>"
        "<tr><td><b>Ctrl + A</b></td><td>Add: Добавление новой записи в активную таблицу</td></tr>"
        "<tr><td><b>Ctrl + V</b></td><td>View: Обновление/просмотр данных таблицы</td></tr>"
        "<tr><td><b>Ctrl + U</b></td><td>Update: Редактирование выбранной записи</td></tr>"
        "<tr><td><b>Ctrl + D</b></td><td>Delete: Удаление записи (требуется ввод ID)</td></tr>"
        "<tr><td><b>Ctrl + Q</b></td><td>Queries: Окно выполнения спец. запросов</td></tr>"
        "<tr><td><b>Ctrl + S</b></td><td>Save: Сохранение результатов в JSON файл</td></tr>"
        "<tr><td><b>Ctrl + B</b></td><td>Backup: Создание бэкапа БД (только Admin)</td></tr>"
        "<tr><td><b>Ctrl + E</b></td><td>Exit: Выход из приложения</td></tr>"
        "</table>"
        "<h3>2. Навигация и Меню:</h3>"
        "<ul>"
        "<li><b>F10</b>: Активация главного меню.</li>"
        "<li><b>Alt + F / T / O</b>: Доступ к разделам File, Tables, Operations.</li>"
        "<li><b>Tab / Shift+Tab</b>: Перемещение фокуса между полями.</li>"
        "<li><b>Arrows (Стрелки)</b>: Перемещение внутри таблиц.</li>"
        "<li><b>Enter / Esc</b>: Подтверждение (OK) или Отмена (Cancel) в окнах.</li>"
        "</ul>"
        "<h3>3. Роли доступа:</h3>"
        "<ul>"
        "<li><b>GUEST</b> (Пользователь): Просмотр данных, правка динамических таблиц.</li>"
        "<li><b>ADMIN</b> (Суперпользователь): Полный доступ + изменение справочников + бэкап.</li>"
        "</ul>"
        "<p><i>Пароль администратора по умолчанию: <b>admin</b></i></p>"
        "</div>";
    helpBox.setText(h);
    helpBox.setIcon(QMessageBox::Information);
    helpBox.setStyleSheet("QMessageBox { background-color: #ffffff; min-width: 600px; } QLabel { color: #000000; } QPushButton { background-color: #2f3640; color: #ffffff; padding: 10px; }");
    helpBox.exec();
}

void MainWindow::addRecord() {
    QStringList lookupTables = {"fitness_categories", "commissioners"};
    if (lookupTables.contains(m_activeTable) && !m_dbManager->isSuperuser()) {
        QMessageBox msg(this); msg.setWindowTitle("Отказ"); msg.setText("Справочники может редактировать только суперпользователь!");
        msg.setIcon(QMessageBox::Warning); msg.setStyleSheet("QMessageBox { background-color: #ffffff; } QLabel { color: #000000; }"); msg.exec();
        return;
    }
    RecordDialog d(m_dbManager, m_activeTable, this); if(d.exec()==QDialog::Accepted) applyFilter();
}

void MainWindow::updateRecord() {
    if (!m_mainTable) return;
    int r = m_mainTable->currentRow();
    if(r < 0) return;
    QStringList lookupTables = {"fitness_categories", "commissioners"};
    if (lookupTables.contains(m_activeTable) && !m_dbManager->isSuperuser()) {
        QMessageBox msg(this); msg.setWindowTitle("Отказ"); msg.setText("Изменение справочников запрещено!");
        msg.setIcon(QMessageBox::Warning); msg.setStyleSheet("QMessageBox { background-color: #ffffff; } QLabel { color: #000000; }"); msg.exec();
        return;
    }
    int id = m_mainTable->item(r, 0)->text().toInt();
    RecordDialog d(m_dbManager, m_activeTable, this, id); if(d.exec()==QDialog::Accepted) applyFilter();
}

void MainWindow::deleteRecord() {
    bool ok;
    int id = QInputDialog::getInt(this, "Удаление записи", "Введите id записи, удаляемой из таблицы:", 1, 1, 1000000, 1, &ok);
    if (!ok) return;
    QStringList lookupTables = {"fitness_categories", "commissioners"};
    if (lookupTables.contains(m_activeTable) && !m_dbManager->isSuperuser()) {
        QMessageBox msg(this); msg.setWindowTitle("Отказ"); msg.setText("Удаление из справочников запрещено!");
        msg.setIcon(QMessageBox::Warning); msg.setStyleSheet("QMessageBox { background-color: #ffffff; } QLabel { color: #000000; }"); msg.exec();
        return;
    }
    if(QMessageBox::question(this, "Удаление", QString("Вы уверены? ID: %1").arg(id)) == QMessageBox::Yes) {
        if(m_dbManager->deleteRecordHttp(m_activeTable, id)) applyFilter();
        else QMessageBox::critical(this, "Ошибка", m_dbManager->lastError());
    }
}

void MainWindow::saveQueryResult() {
    if (m_activeTable.isEmpty()) return;
    QJsonArray data = m_dbManager->fetchTableDataHttp(m_activeTable);
    QString fileName = QFileDialog::getSaveFileName(this, "Сохранить результат", "", "JSON (*.json)");
    if (fileName.isEmpty()) return;
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(data).toJson()); file.close();
        QMessageBox msg(this); msg.setWindowTitle("Успех"); msg.setText("Результаты сохранены в:\n" + fileName);
        msg.setStyleSheet("QMessageBox { background-color: #ffffff; } QLabel { color: #000000; }"); msg.exec();
    }
}

void MainWindow::exportAllData() { saveQueryResult(); }

void MainWindow::createBackup() {
    bool ok;
    QString pass = QInputDialog::getText(this, "Бэкап", "Введите пароль администратора:", QLineEdit::Password, "", &ok);
    if (!ok || pass != "admin") { if(ok) QMessageBox::critical(this, "Отказ", "Неверный пароль!"); return; }
    m_dbManager->setAuthToken("admin");
    if (m_dbManager->createBackupHttp()) QMessageBox::information(this, "Успех", "Бэкап успешно создан на сервере!");
}

void MainWindow::restoreFromBackup() {
    QString fileName = QFileDialog::getOpenFileName(this, "Восстановить", "", "SQL (*.sql)");
    if (!fileName.isEmpty()) { BackupManager bm(m_dbManager); if (bm.restoreFromBackup(fileName)) QMessageBox::information(this, "Успех", "Восстановлено!"); }
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_F10) {
        if (menuBar()) {
            menuBar()->setFocus();
            if (!menuBar()->actions().isEmpty()) menuBar()->setActiveAction(menuBar()->actions().first());
        }
        return;
    }
    if (event->key() == Qt::Key_F1) { showHelp(); return; }
    if (event->modifiers() & Qt::ControlModifier) {
        switch(event->key()) {
            case Qt::Key_A: addRecord(); return;
            case Qt::Key_V: viewActiveTable(); return;
            case Qt::Key_D: deleteRecord(); return;
            case Qt::Key_U: updateRecord(); return;
            case Qt::Key_Q: openQueries(); return;
            case Qt::Key_S: saveQueryResult(); return;
            case Qt::Key_B: createBackup(); return;
            case Qt::Key_E: exitApp(); return;
        }
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::setupStyles() {
    if (m_isClassicUI) {
        setStyleSheet(
            "QMainWindow { background-color: #ffffff; }"
            "QMenuBar { background-color: #ffffff; color: #000000; border-bottom: 1px solid #2f3640; font-weight: bold; }"
            "QMenuBar::item:selected { background-color: #2f3640; color: #ffffff; }"
            "QMenu { background-color: #ffffff; color: #000000; border: 1px solid #2f3640; }"
            "QMenu::item:selected { background-color: #2f3640; color: #ffffff; }"
            "QLabel { color: #000000; font-family: 'Segoe UI'; font-weight: bold; }"
            "QLineEdit { background-color: #ffffff; color: #000000; border: 1px solid #2f3640; padding: 2px; }"
            "QLineEdit:focus { border: 2px solid #0000ff; background-color: #ffffcc; }"
            "QComboBox { background-color: #ffffff; color: #000000; border: 1px solid #2f3640; }"
            "QComboBox:focus { border: 2px solid #0000ff; background-color: #ffffcc; }"
            "QTableWidget { background-color: #ffffff; color: #000000; border: 1px solid #dcdde1; selection-background-color: #2f3640; selection-color: #ffffff; }"
            "QTableWidget:focus { border: 2px solid #0000ff; }"
            "QPushButton { background-color: #f5f6fa; color: #000000; border: 1px solid #2f3640; font-weight: bold; padding: 4px; }"
        );
    } else {
        setStyleSheet("QMainWindow { background-color: #f5f6fa; } QLabel { color: #2f3640; font-family: 'Segoe UI'; } QTableWidget { background-color: #ffffff; color: #000000; border: 1px solid #dcdde1; }");
    }
}

void MainWindow::updateConnectionStatus() {
    if (!m_statusLabel) return;
    bool admin = m_dbManager->isSuperuser();
    m_statusLabel->setText(m_dbManager->isConnected() ? (admin ? "● СУПЕРПОЛЬЗОВАТЕЛЬ (ADMIN)" : "● ПОЛЬЗОВАТЕЛЬ (GUEST)") : "○ OFFLINE");
    m_statusLabel->setStyleSheet(admin ? "color: #e67e22; font-weight: bold; background: #ffffff; border: 1px solid #e67e22; padding: 5px; border-radius: 4px;" : "color: #27ae60; font-weight: bold; padding: 5px;");
}

void MainWindow::onTableSelected(const QString &t) { m_activeTable = t; viewActiveTable(); }
void MainWindow::refreshTablesMenu() { if (!m_tablesMenu) return; m_tablesMenu->clear(); QStringList ts = m_dbManager->getTableList(); foreach(const QString &t, ts) m_tablesMenu->addAction(t, [this, t](){ onTableSelected(t); }); }
void MainWindow::openQueries() { if (!m_queriesWindow) m_queriesWindow = new QueriesWindow(m_dbManager); m_queriesWindow->show(); }
void MainWindow::openQueriesWindow() { openQueries(); }
void MainWindow::openTablesWindow() { if(!m_tablesWindow) m_tablesWindow = new TablesWindow(m_dbManager); m_tablesWindow->show(); }
void MainWindow::exitApp() { if(QMessageBox::question(this, "Выход", "Закрыть?") == QMessageBox::Yes) qApp->quit(); }
bool MainWindow::connectToDatabase() { ConfigManager config("config.ini"); m_dbManager->setHttpMode(config.isHttpMode()); return m_dbManager->connectToDatabase(config.getDatabaseHost(), config.getDatabasePort(), config.getDatabaseName(), config.getDatabaseUsername(), config.getDatabasePassword()); }
void MainWindow::clearLayout(QLayout *l) { if(!l) return; while(QLayoutItem *i = l->takeAt(0)) { if(i->widget()) i->widget()->deleteLater(); delete i; } }
void MainWindow::showHighContrastHelp(const QString&, const QString&) {}
