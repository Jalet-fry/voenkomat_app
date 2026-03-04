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

    if (m_isClassicUI) setFixedSize(950, 700); else setFixedSize(450, 650);

    setupUI();
    setupStyles();

    if (connectToDatabase()) {
        updateConnectionStatus();
        m_activeTable = "conscripts";
        if (m_isClassicUI) {
            refreshTablesMenu();
            viewActiveTable();
            if (m_filterValueEdit) m_filterValueEdit->setFocus();
        }
    } else {
        updateConnectionStatus();
    }
}

MainWindow::~MainWindow() {}

void MainWindow::clearLayout(QLayout *layout)
{
    if (!layout) return;
    while (QLayoutItem *item = layout->takeAt(0)) {
        if (QWidget *widget = item->widget()) widget->deleteLater();
        else if (QLayout *childLayout = item->layout()) clearLayout(childLayout);
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
        setFixedSize(450, 650);
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
    m_layout->setSpacing(12);
    m_layout->setContentsMargins(30, 30, 30, 30);

    m_titleLabel = new QLabel("Система Военкомат", this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #2c3e50;");
    m_layout->addWidget(m_titleLabel);

    m_tablesBtn = new QPushButton("📜 Список Таблиц", this);
    m_queriesBtn = new QPushButton("🔍 Спец. Запросы", this);
    m_exportBtn = new QPushButton("📤 Экспорт БД", this);
    m_helpBtn = new QPushButton("ℹ️ Справка (F1)", this);
    m_switchModeBtn = new QPushButton("⚙️ Перейти в CUA", this);
    m_exitBtn = new QPushButton("Выход (Ctrl+E)", this);

    m_layout->addWidget(m_tablesBtn);
    m_layout->addWidget(m_queriesBtn);
    m_layout->addWidget(m_exportBtn);
    m_layout->addWidget(m_helpBtn);
    m_layout->addStretch();
    m_layout->addWidget(m_switchModeBtn);
    m_layout->addWidget(m_exitBtn);

    connect(m_tablesBtn, &QPushButton::clicked, this, &MainWindow::openTablesWindow);
    connect(m_queriesBtn, &QPushButton::clicked, this, &MainWindow::openQueriesWindow);
    connect(m_exportBtn, &QPushButton::clicked, this, &MainWindow::exportAllData);
    connect(m_helpBtn, &QPushButton::clicked, this, &MainWindow::showHelp);
    connect(m_switchModeBtn, &QPushButton::clicked, this, &MainWindow::switchMode);
    connect(m_exitBtn, &QPushButton::clicked, this, &MainWindow::exitApp);
}

void MainWindow::setupClassicUI()
{
    m_layout->setContentsMargins(5, 5, 5, 5);
    m_layout->setSpacing(4);

    m_menuBar = menuBar();
    if (!m_menuBar) { m_menuBar = new QMenuBar(this); setMenuBar(m_menuBar); }
    m_menuBar->show();
    m_menuBar->clear();
    m_menuBar->setFocusPolicy(Qt::TabFocus);

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
    opsMenu->addAction("Save Result", QKeySequence("Ctrl+S"), this, &MainWindow::saveQueryResult);
    opsMenu->addAction("Backup", QKeySequence("Ctrl+B"), this, &MainWindow::createBackup);

    m_menuBar->addMenu("&View")->addAction("Modern UI", this, &MainWindow::switchMode);
    m_menuBar->addMenu("&Help")->addAction("Help Content", QKeySequence("F1"), this, &MainWindow::showHelp);

    QHBoxLayout *fL = new QHBoxLayout();
    m_filterColumnCombo = new QComboBox(this);
    m_filterValueEdit = new QLineEdit(this);
    m_filterValueEdit->setPlaceholderText("Filter (use > < or \"text\")...");
    m_applyFilterBtn = new QPushButton("Apply", this);
    m_applyFilterBtn->setFixedWidth(100);
    connect(m_applyFilterBtn, &QPushButton::clicked, this, &MainWindow::applyFilter);

    fL->addWidget(new QLabel("Field:")); fL->addWidget(m_filterColumnCombo);
    fL->addWidget(m_filterValueEdit); fL->addWidget(m_applyFilterBtn);
    m_layout->addLayout(fL);

    m_activeTableLabel = new QLabel("Active Table: [ " + m_activeTable.toUpper() + " ]", this);
    m_activeTableLabel->setStyleSheet("background: #000080; color: #ffff00; padding: 3px; font-weight: bold;");
    m_layout->addWidget(m_activeTableLabel);

    m_mainTable = new QTableWidget(this);
    m_mainTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_mainTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_mainTable->setAlternatingRowColors(true);
    m_mainTable->setFocusPolicy(Qt::StrongFocus);
    m_layout->addWidget(m_mainTable);

    m_classicFooter = new QLabel(" F1-Help | F10-Menu | Alt+T-Tables | Tab-Focus | Ctrl+V-View | Ctrl+E-Exit ", this);
    m_classicFooter->setStyleSheet("background-color: #c0c0c0; color: black; border-top: 1px solid black; font-family: 'Consolas'; font-size: 11px;");
    m_layout->addWidget(m_classicFooter);

    setTabOrder(m_filterColumnCombo, m_filterValueEdit);
    setTabOrder(m_filterValueEdit, m_applyFilterBtn);
    setTabOrder(m_applyFilterBtn, m_mainTable);
    setTabOrder(m_mainTable, m_menuBar);
}

void MainWindow::refreshTablesMenu()
{
    if (!m_tablesMenu || !m_dbManager->isConnected()) return;
    m_tablesMenu->clear();
    QStringList tables = m_dbManager->getTableList();
    foreach(const QString &t, tables) {
        m_tablesMenu->addAction(t, [this, t](){ onTableSelected(t); });
    }
}

void MainWindow::onTableSelected(const QString &t) { m_activeTable = t; viewActiveTable(); }

void MainWindow::viewActiveTable()
{
    if (m_activeTable.isEmpty() || !m_dbManager->isConnected()) return;
    QStringList cols = m_dbManager->getColumnList(m_activeTable);
    if (m_filterColumnCombo) {
        m_filterColumnCombo->clear();
        m_filterColumnCombo->addItems(cols);
    }
    if (m_mainTable) {
        m_mainTable->setColumnCount(cols.size());
        m_mainTable->setHorizontalHeaderLabels(cols);
        applyFilter();
    }
    if (m_activeTableLabel) m_activeTableLabel->setText("Active Table: [ " + m_activeTable.toUpper() + " ]");
}

void MainWindow::applyFilter()
{
    if (!m_mainTable || m_activeTable.isEmpty()) return;
    QString where = "";
    if (m_filterColumnCombo && m_filterValueEdit && !m_filterValueEdit->text().isEmpty()) {
        QString val = m_filterValueEdit->text().trimmed();
        QString col = m_filterColumnCombo->currentText();
        bool hasOp = val.startsWith(">") || val.startsWith("<") || val.startsWith("=") || val.startsWith("!");
        if (hasOp) where = QString("%1 %2").arg(col).arg(val);
        else if (val.startsWith("\"") && val.endsWith("\"")) where = QString("%1::text = '%2'").arg(col).arg(val.mid(1, val.length()-2).replace("'", "''"));
        else where = QString("%1::text ILIKE '%%2%'").arg(col).arg(val.replace("'", "''"));
    }

    QJsonArray data;
    if (m_dbManager->isHttpMode()) {
        data = m_dbManager->fetchTableDataHttp(m_activeTable, where);
    } else {
        QString sql = "SELECT * FROM " + m_activeTable;
        if (!where.isEmpty()) sql += " WHERE " + where;
        QSqlQuery q = m_dbManager->executeQuery(sql);
        QStringList cols = m_dbManager->getColumnList(m_activeTable);
        while(q.next()) {
            QJsonObject o;
            for(int i=0; i<cols.size(); ++i) o[cols[i]] = QJsonValue::fromVariant(q.value(i));
            data.append(o);
        }
    }

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
    RecordDialog d(m_dbManager, m_activeTable, this);
    if(d.exec()==QDialog::Accepted) applyFilter();
}

void MainWindow::updateRecord() {
    if (!m_mainTable) return;
    int r = m_mainTable->currentRow();
    if(r < 0) return;
    int id = m_mainTable->item(r, 0)->text().toInt();
    RecordDialog d(m_dbManager, m_activeTable, this, id);
    if(d.exec()==QDialog::Accepted) applyFilter();
}

void MainWindow::deleteRecord() {
    if (!m_mainTable) return;
    int r = m_mainTable->currentRow();
    if(r < 0) return;
    if(QMessageBox::question(this, "Delete", "Confirm?") == QMessageBox::Yes) {
        int id = m_mainTable->item(r, 0)->text().toInt();
        if(m_dbManager->deleteRecordHttp(m_activeTable, id)) applyFilter();
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_F10) { if (menuBar()) menuBar()->setFocus(); return; }
    if (event->key() == Qt::Key_F1) { showHelp(); return; }
    if (event->key() == Qt::Key_F2) { addRecord(); return; }
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

void MainWindow::setupStyles()
{
    if (m_isClassicUI) {
        setStyleSheet(
            "QMainWindow { background-color: #c0c0c0; }"
            "QMenuBar { background-color: #c0c0c0; border-bottom: 1px solid black; color: black; }"
            "QMenuBar::item:selected { background-color: #000080; color: white; }"
            "QTableWidget { background-color: white; border: 2px inset gray; color: black; font-family: 'Consolas'; font-size: 13px; }"
            "QLabel { color: black; font-family: 'Consolas'; }"
        );
    } else {
        setStyleSheet("QMainWindow { background-color: #f0f3f5; }"
                      "QPushButton { background-color: #2c3e50; color: white; border-radius: 6px; padding: 10px; }");
    }
}

void MainWindow::showHelp() {
    QString h = "<h2>CUA COMMANDS</h2>"
                "<b>Alt+F / T / O</b>: Open Menu<br>"
                "<b>F10</b>: Menu Focus<br>"
                "<b>Tab</b>: Change Focus<br><br>"
                "<b>Hotkeys:</b><br>"
                "- <b>Ctrl+V</b>: View Data<br>"
                "- <b>F2 / Ctrl+A</b>: Add Record<br>"
                "- <b>Ctrl+D</b>: Delete<br>"
                "- <b>Ctrl+U</b>: Update<br>"
                "- <b>Ctrl+E</b>: Exit";
    showHighContrastHelp("Help", h);
}

void MainWindow::showHighContrastHelp(const QString &title, const QString &content) {
    QDialog d(this); d.setWindowTitle(title); d.setFixedSize(500, 450);
    QVBoxLayout *l = new QVBoxLayout(&d);
    QLabel *t = new QLabel(content, &d); t->setTextFormat(Qt::RichText);
    t->setStyleSheet("background: #ffffcc; border: 1px solid black; padding: 15px; font-family: 'Consolas'; color: black;");
    l->addWidget(t);
    QPushButton *ok = new QPushButton("Close (Enter)", &d); connect(ok, &QPushButton::clicked, &d, &QDialog::accept);
    l->addWidget(ok, 0, Qt::AlignCenter); d.exec();
}

void MainWindow::openQueries() {
    if (!m_queriesWindow) m_queriesWindow = new QueriesWindow(m_dbManager);
    m_queriesWindow->show();
}

void MainWindow::openQueriesWindow() { openQueries(); }

void MainWindow::openTablesWindow() {
    if(!m_tablesWindow) m_tablesWindow = new TablesWindow(m_dbManager);
    m_tablesWindow->show();
}

void MainWindow::exportAllData() {
    BackupManager bm(m_dbManager);
    if (bm.exportAllTables()) QMessageBox::information(this, "OK", "Export OK");
}

void MainWindow::exitApp() {
    if (QMessageBox::question(this, "Exit", "Close Application?") == QMessageBox::Yes) qApp->quit();
}

void MainWindow::updateConnectionStatus() {
    if (!m_statusLabel) return;
    m_statusLabel->setText(m_dbManager->isConnected() ? "● ONLINE" : "○ OFFLINE");
    m_statusLabel->setStyleSheet(m_dbManager->isConnected() ? "color: green;" : "color: red;");
}

bool MainWindow::connectToDatabase() {
    ConfigManager config("config.ini");
    m_dbManager->setHttpMode(config.isHttpMode());
    return m_dbManager->connectToDatabase(config.getDatabaseHost(), config.getDatabasePort(), config.getDatabaseName(), config.getDatabaseUsername(), config.getDatabasePassword());
}

void MainWindow::saveQueryResult() {
    BackupManager bm(m_dbManager);
    if (bm.exportAllTables()) QMessageBox::information(this, "OK", "Saved");
}

void MainWindow::createBackup() { exportAllData(); }
void MainWindow::restoreFromBackup() {
    QString fileName = QFileDialog::getOpenFileName(this, "Восстановить", "", "SQL (*.sql)");
    if (fileName.isEmpty()) return;
    BackupManager bm(m_dbManager);
    if (bm.restoreFromBackup(fileName)) QMessageBox::information(this, "Успех", "Восстановлено!");
}
