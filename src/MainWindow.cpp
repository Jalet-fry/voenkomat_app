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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_dbManager(new DatabaseManager(this))
{
    ConfigManager config("config.ini");
    m_isClassicUI = config.isClassicUI();
    
    setWindowTitle(m_isClassicUI ? "ИС Военкомат (CUA)" : "ИС Военкомат (Modern UI)");
    if (m_isClassicUI) setFixedSize(950, 700); else setFixedSize(450, 680);

    setupUI();
    setupStyles();

    if (connectToDatabase()) {
        updateConnectionStatus();
        m_activeTable = "conscripts";
        if (m_isClassicUI) {
            refreshTablesMenu();
            viewActiveTable();
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
    } else {
        setFixedSize(450, 680);
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
    m_titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #2f3640; margin-bottom: 10px;");
    m_layout->addWidget(m_titleLabel);

    m_tablesBtn = new QPushButton("📜 Список Таблиц", this);
    m_queriesBtn = new QPushButton("🔍 Спец. Запросы", this);
    m_exportBtn = new QPushButton("📤 Экспорт БД", this);

    QPushButton *loginBtn = new QPushButton("🔐 Войти как Admin", this);
    loginBtn->setStyleSheet("background-color: #e67e22; color: #ffffff; font-weight: bold; border: 1px solid #d35400; padding: 12px; border-radius: 6px;");

    QPushButton *logoutBtn = new QPushButton("🔓 Выйти из Admin", this);
    logoutBtn->setStyleSheet("background-color: #95a5a6; color: #ffffff; font-weight: bold; border: 1px solid #7f8c8d; padding: 12px; border-radius: 6px;");

    m_helpBtn = new QPushButton("ℹ️ Справка (F1)", this);
    m_switchModeBtn = new QPushButton("⚙️ Перейти в CUA", this);
    m_exitBtn = new QPushButton("Выход", this);
    m_exitBtn->setStyleSheet("background-color: #c0392b; color: #ffffff; font-weight: bold; padding: 10px; border-radius: 6px;");

    m_layout->addWidget(m_tablesBtn);
    m_layout->addWidget(m_queriesBtn);
    m_layout->addWidget(m_exportBtn);
    m_layout->addWidget(loginBtn);
    m_layout->addWidget(logoutBtn);
    m_layout->addWidget(m_helpBtn);
    m_layout->addSpacing(10);
    m_layout->addWidget(m_switchModeBtn);
    m_layout->addWidget(m_exitBtn);

    connect(m_tablesBtn, &QPushButton::clicked, this, &MainWindow::openTablesWindow);
    connect(m_queriesBtn, &QPushButton::clicked, this, &MainWindow::openQueriesWindow);
    connect(m_exportBtn, &QPushButton::clicked, this, &MainWindow::exportAllData);
    connect(m_helpBtn, &QPushButton::clicked, this, &MainWindow::showHelp);
    connect(m_switchModeBtn, &QPushButton::clicked, this, &MainWindow::switchMode);
    connect(m_exitBtn, &QPushButton::clicked, this, &MainWindow::exitApp);

    // СТИЛЬ ДЛЯ ГАРАНТИРОВАННОГО КОНТРАСТА (Белый фон, Черный текст)
    QString dialogStyle =
        "QDialog, QMessageBox, QInputDialog { background-color: #ffffff; border: 2px solid #2f3640; }"
        "QLabel { color: #000000; font-weight: bold; font-size: 14px; }"
        "QLineEdit { background-color: #ffffff; color: #000000; border: 2px solid #2f3640; padding: 8px; border-radius: 4px; }"
        "QPushButton { background-color: #2f3640; color: #ffffff; font-weight: bold; padding: 8px 16px; border-radius: 4px; min-width: 80px; }";

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

                QMessageBox msg(this);
                msg.setWindowTitle("Успех");
                msg.setText("Режим администратора включен!");
                msg.setIcon(QMessageBox::Information);
                msg.setStyleSheet(dialogStyle);
                msg.exec();
            } else {
                QMessageBox msg(this);
                msg.setWindowTitle("Ошибка");
                msg.setText("Неверный пароль!");
                msg.setIcon(QMessageBox::Critical);
                msg.setStyleSheet(dialogStyle);
                msg.exec();
            }
        }
    });

    connect(logoutBtn, &QPushButton::clicked, this, [this, dialogStyle](){
        QMessageBox confirm(this);
        confirm.setWindowTitle("Выход");
        confirm.setText("Выйти из режима администратора?");
        confirm.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        confirm.setIcon(QMessageBox::Question);
        confirm.setStyleSheet(dialogStyle);

        if (confirm.exec() == QMessageBox::Yes) {
            m_dbManager->setAuthToken("");
            updateConnectionStatus();
            if(m_isClassicUI) viewActiveTable();

            QMessageBox msg(this);
            msg.setWindowTitle("Статус");
            msg.setText("Вы перешли в режим ОБЫЧНОГО ПОЛЬЗОВАТЕЛЯ.");
            msg.setIcon(QMessageBox::Information);
            msg.setStyleSheet(dialogStyle);
            msg.exec();
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
    fileMenu->addAction("Login (Admin)", this, [this](){
        bool ok;
        QString pass = QInputDialog::getText(this, "Вход", "Пароль:", QLineEdit::Password, "", &ok);
        if (ok && pass == "admin") { m_dbManager->setAuthToken("admin"); updateConnectionStatus(); viewActiveTable(); }
    });
    fileMenu->addAction("Logout", this, [this](){ m_dbManager->setAuthToken(""); updateConnectionStatus(); viewActiveTable(); });
    fileMenu->addSeparator();
    fileMenu->addAction("Exit", QKeySequence("Ctrl+E"), this, &MainWindow::exitApp);

    m_tablesMenu = m_menuBar->addMenu("&Tables");
    refreshTablesMenu();

    QMenu *opsMenu = m_menuBar->addMenu("&Operations");
    opsMenu->addAction("View", QKeySequence("Ctrl+V"), this, &MainWindow::viewActiveTable);
    opsMenu->addAction("Add", QKeySequence("Ctrl+A"), this, &MainWindow::addRecord);
    opsMenu->addAction("Update", QKeySequence("Ctrl+U"), this, &MainWindow::updateRecord);
    opsMenu->addAction("Delete", QKeySequence("Ctrl+D"), this, &MainWindow::deleteRecord);
    opsMenu->addSeparator();
    opsMenu->addAction("Backup", QKeySequence("Ctrl+B"), this, &MainWindow::createBackup);

    m_menuBar->addMenu("&View")->addAction("Modern UI", this, &MainWindow::switchMode);
    m_menuBar->addMenu("&Help")->addAction("Help Content", QKeySequence("F1"), this, &MainWindow::showHelp);

    QHBoxLayout *fL = new QHBoxLayout();
    m_filterColumnCombo = new QComboBox(this);
    m_filterValueEdit = new QLineEdit(this);
    m_applyFilterBtn = new QPushButton("Apply", this);
    m_applyFilterBtn->setFixedWidth(80);
    connect(m_applyFilterBtn, &QPushButton::clicked, this, &MainWindow::applyFilter);
    fL->addWidget(new QLabel("Field:")); fL->addWidget(m_filterColumnCombo);
    fL->addWidget(m_filterValueEdit); fL->addWidget(m_applyFilterBtn);
    m_layout->addLayout(fL);

    m_activeTableLabel = new QLabel(this);
    m_layout->addWidget(m_activeTableLabel);

    m_mainTable = new QTableWidget(this);
    m_mainTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_mainTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_layout->addWidget(m_mainTable);

    m_classicFooter = new QLabel(" F1-Help | Admin Password: admin ", this);
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
    if (m_mainTable) { m_mainTable->setColumnCount(cols.size()); m_mainTable->setHorizontalHeaderLabels(cols); applyFilter(); }
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

void MainWindow::addRecord() {
    QStringList lookupTables = {"fitness_categories", "commissioners"};
    if (lookupTables.contains(m_activeTable) && !m_dbManager->isSuperuser()) {
        QMessageBox msg(this);
        msg.setWindowTitle("Отказ");
        msg.setText("Рядовому пользователю запрещено изменять справочники!");
        msg.setIcon(QMessageBox::Warning);
        msg.setStyleSheet("QMessageBox { background-color: #ffffff; } QLabel { color: #000000; font-weight: bold; } QPushButton { background-color: #2f3640; color: #ffffff; }");
        msg.exec();
        return;
    }
    RecordDialog d(m_dbManager, m_activeTable, this);
    if(d.exec()==QDialog::Accepted) applyFilter();
}

void MainWindow::updateRecord() {
    if (!m_mainTable) return;
    int r = m_mainTable->currentRow();
    if(r < 0) return;
    QStringList lookupTables = {"fitness_categories", "commissioners"};
    if (lookupTables.contains(m_activeTable) && !m_dbManager->isSuperuser()) {
        QMessageBox::warning(this, "Отказ", "Изменение справочников запрещено!");
        return;
    }
    int id = m_mainTable->item(r, 0)->text().toInt();
    RecordDialog d(m_dbManager, m_activeTable, this, id);
    if(d.exec()==QDialog::Accepted) applyFilter();
}

void MainWindow::deleteRecord() {
    if (!m_mainTable) return;
    int r = m_mainTable->currentRow();
    if(r < 0) return;
    QStringList lookupTables = {"fitness_categories", "commissioners"};
    if (lookupTables.contains(m_activeTable) && !m_dbManager->isSuperuser()) {
        QMessageBox::warning(this, "Отказ", "Удаление из справочников запрещено!");
        return;
    }
    if(QMessageBox::question(this, "Удаление", "Вы уверены?") == QMessageBox::Yes) {
        int id = m_mainTable->item(r, 0)->text().toInt();
        if(m_dbManager->deleteRecordHttp(m_activeTable, id)) applyFilter();
        else QMessageBox::critical(this, "Ошибка", m_dbManager->lastError());
    }
}

void MainWindow::createBackup() {
    if (!m_dbManager->isSuperuser()) {
        QMessageBox::warning(this, "Отказ", "Бэкап может делать только админ!");
        return;
    }
    if (m_dbManager->createBackupHttp()) QMessageBox::information(this, "Успех", "Бэкап сохранен на сервере!");
}

void MainWindow::setupStyles()
{
    if (m_isClassicUI) {
        setStyleSheet(
            "QMainWindow { background-color: #c0c0c0; }"
            "QLabel { color: #000000; font-family: 'Segoe UI'; }"
            "QTableWidget { background-color: #ffffff; color: #000000; border: 2px inset #808080; selection-background-color: #000080; }"
            "QPushButton { background-color: #c0c0c0; color: #000000; border: 2px outset #808080; font-weight: bold; padding: 4px; }"
            "QPushButton:pressed { border: 2px inset #808080; }"
        );
    } else {
        setStyleSheet(
            "QMainWindow { background-color: #f5f6fa; }"
            "QLabel { color: #2f3640; font-family: 'Segoe UI'; }"
            "QPushButton { background-color: #2f3640; color: #ffffff; border-radius: 6px; padding: 10px; font-weight: bold; border: 1px solid #2f3640; }"
            "QPushButton:hover { background-color: #353b48; }"
            "QTableWidget { background-color: #ffffff; color: #000000; border: 1px solid #dcdde1; }"
        );
    }
}

void MainWindow::updateConnectionStatus() {
    if (!m_statusLabel) return;
    bool admin = m_dbManager->isSuperuser();
    m_statusLabel->setText(m_dbManager->isConnected() ? (admin ? "● СУПЕРПОЛЬЗОВАТЕЛЬ (ADMIN)" : "● ПОЛЬЗОВАТЕЛЬ (GUEST)") : "○ OFFLINE");
    m_statusLabel->setStyleSheet(admin ? "color: #e67e22; font-weight: bold; padding: 5px; background-color: #fdf2e9; border-radius: 4px;"
                                      : "color: #27ae60; font-weight: bold; padding: 5px;");
}

void MainWindow::onTableSelected(const QString &t) { m_activeTable = t; viewActiveTable(); }
void MainWindow::refreshTablesMenu() { if (!m_tablesMenu) return; m_tablesMenu->clear(); QStringList ts = m_dbManager->getTableList(); foreach(const QString &t, ts) m_tablesMenu->addAction(t, [this, t](){ onTableSelected(t); }); }
void MainWindow::keyPressEvent(QKeyEvent *e) { if (e->key() == Qt::Key_F1) showHelp(); QMainWindow::keyPressEvent(e); }
void MainWindow::showHelp() { QMessageBox::information(this, "Help", "User: View all, Edit dynamic tables.\nAdmin (admin): Full access, Backups."); }
void MainWindow::openQueries() { if (!m_queriesWindow) m_queriesWindow = new QueriesWindow(m_dbManager); m_queriesWindow->show(); }
void MainWindow::openQueriesWindow() { openQueries(); }
void MainWindow::openTablesWindow() { if(!m_tablesWindow) m_tablesWindow = new TablesWindow(m_dbManager); m_tablesWindow->show(); }
void MainWindow::exportAllData() { BackupManager bm(m_dbManager); bm.exportAllTables(); }
void MainWindow::exitApp() { qApp->quit(); }
bool MainWindow::connectToDatabase() { ConfigManager config("config.ini"); m_dbManager->setHttpMode(config.isHttpMode()); return m_dbManager->connectToDatabase(config.getDatabaseHost(), config.getDatabasePort(), config.getDatabaseName(), config.getDatabaseUsername(), config.getDatabasePassword()); }
void MainWindow::saveQueryResult() {}
void MainWindow::restoreFromBackup() {}
void MainWindow::showHighContrastHelp(const QString&, const QString&) {}
void MainWindow::clearLayout(QLayout *l) { if(!l) return; while(QLayoutItem *i = l->takeAt(0)) { if(i->widget()) i->widget()->deleteLater(); delete i; } }
