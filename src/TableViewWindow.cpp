#include "TableViewWindow.h"
#include "RecordDialog.h"
#include "ConfigManager.h"
#include <QLabel>
#include <QMessageBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMenuBar>
#include <QDialog>
#include <QLineEdit>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonObject>
#include <QSqlRecord>
#include <QScrollArea>
#include <QTimer>
#include "xlsxdocument.h"

using namespace QXlsx;

TableViewWindow::TableViewWindow(DatabaseManager *dbManager, const QString &tableName, QWidget *parent)
    : QWidget(parent)
    , m_dbManager(dbManager)
    , m_tableName(tableName)
{
    ConfigManager config("config.ini");
    m_isClassicUI = config.isClassicUI();

    setWindowTitle(m_isClassicUI ? "Просмотр: " + tableName : "Таблица: " + tableName);

    // LABS REQUIREMENT: Fixed sizes
    if (m_isClassicUI) setFixedSize(900, 600); else resize(1100, 750);

    m_filterTimer = new QTimer(this);
    m_filterTimer->setSingleShot(true);
    m_filterTimer->setInterval(600);
    connect(m_filterTimer, &QTimer::timeout, this, &TableViewWindow::applyFilters);

    setupUI();
    setupStyles();
    loadData();
}

TableViewWindow::~TableViewWindow() {}

void TableViewWindow::setupUI()
{
    m_layout = new QVBoxLayout(this);

    if (m_isClassicUI) {
        setupClassicUI();
    } else {
        setupModernUI();
    }

    m_table = new QTableWidget(this);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &TableViewWindow::updateButtonStates);
    m_layout->addWidget(m_table);

    m_statusLabel = new QLabel("Загрузка...", this);
    m_layout->addWidget(m_statusLabel);

    if (m_isClassicUI) {
        m_footerHint = new QLabel(" [Ins] Добавить | [F4] Изменить | [Del] Удалить | [F3] Фильтр | [Esc] Назад ", this);
        m_footerHint->setStyleSheet("background-color: #000080; color: white; font-family: 'Consolas'; font-size: 11px;");
        m_layout->addWidget(m_footerHint);
    }
}

void TableViewWindow::setupModernUI()
{
    m_layout->setContentsMargins(15, 15, 15, 15);
    
    QLabel *title = new QLabel(m_tableName.toUpper(), this);
    title->setObjectName("modernTitle");
    m_layout->addWidget(title);

    m_filterScrollArea = new QScrollArea(this);
    m_filterScrollArea->setMaximumHeight(150);
    m_filterScrollArea->setWidgetResizable(true);
    m_filterScrollArea->setObjectName("filterArea");

    QWidget *fw = new QWidget();
    fw->setObjectName("filterContent");
    QGridLayout *fl = new QGridLayout(fw);
    
    QStringList cols = m_dbManager->getColumnList(m_tableName);
    int r=0, c=0;
    foreach(const QString &col, cols) {
        QLabel *l = new QLabel(col + ":", fw);
        QLineEdit *e = new QLineEdit(fw);
        e->setPlaceholderText("Поиск...");
        connect(e, &QLineEdit::textChanged, this, &TableViewWindow::onFilterChanged);
        m_filterWidgets[col] = e;

        fl->addWidget(l, r, c);
        fl->addWidget(e, r, c+1);

        c += 2;
        if (c >= 6) { c=0; r++; }
    }
    m_filterScrollArea->setWidget(fw);

    QLabel *filterHeader = new QLabel("Фильтры поиска:", this);
    filterHeader->setObjectName("filterHeader");
    m_layout->addWidget(filterHeader);
    m_layout->addWidget(m_filterScrollArea);

    QHBoxLayout *bl = new QHBoxLayout();
    m_addBtn = new QPushButton("Добавить", this);
    m_editBtn = new QPushButton("Изменить", this);
    m_deleteBtn = new QPushButton("Удалить", this);

    m_addBtn->setMinimumHeight(35);
    m_editBtn->setMinimumHeight(35);
    m_deleteBtn->setMinimumHeight(35);

    connect(m_addBtn, &QPushButton::clicked, this, &TableViewWindow::addRecord);
    connect(m_editBtn, &QPushButton::clicked, this, &TableViewWindow::editRecord);
    connect(m_deleteBtn, &QPushButton::clicked, this, &TableViewWindow::deleteRecord);

    bl->addWidget(m_addBtn); bl->addWidget(m_editBtn); bl->addWidget(m_deleteBtn);
    bl->addStretch();
    
    QPushButton *back = new QPushButton("Назад", this);
    back->setMinimumHeight(35);
    back->setStyleSheet("background-color: #95a5a6;");
    connect(back, &QPushButton::clicked, this, &TableViewWindow::goBack);
    bl->addWidget(back);
    m_layout->addLayout(bl);
}

void TableViewWindow::setupClassicUI()
{
    m_layout->setContentsMargins(2, 2, 2, 2);
    m_layout->setSpacing(0);

    m_menuBar = new QMenuBar(this);
    QMenu *m = m_menuBar->addMenu("&Запись");
    m->addAction("Добавить (Ins)", this, &TableViewWindow::addRecord, QKeySequence(Qt::Key_Insert));
    m->addAction("Изменить (F4)", this, &TableViewWindow::editRecord, QKeySequence(Qt::Key_F4));
    m->addAction("Удалить (Del)", this, &TableViewWindow::deleteRecord, QKeySequence(Qt::Key_Delete));

    QMenu *v = m_menuBar->addMenu("&Вид");
    v->addAction("Фильтры (F3)", this, &TableViewWindow::showFiltersDialog, QKeySequence(Qt::Key_F3));
    v->addAction("Обновить (F5)", this, [this](){ loadData(); }, QKeySequence(Qt::Key_F5));

    QMenu *e = m_menuBar->addMenu("&Экспорт");
    e->addAction("В Excel", this, &TableViewWindow::exportToXlsx);

    m_layout->setMenuBar(m_menuBar);
}

void TableViewWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) { goBack(); return; }
    if (m_isClassicUI) {
        if (event->key() == Qt::Key_Insert) { addRecord(); return; }
        if (event->key() == Qt::Key_F4) { editRecord(); return; }
        if (event->key() == Qt::Key_Delete) { deleteRecord(); return; }
        if (event->key() == Qt::Key_F3) { showFiltersDialog(); return; }
    }
    QWidget::keyPressEvent(event);
}

void TableViewWindow::showFiltersDialog()
{
    QDialog dlg(this);
    dlg.setWindowTitle("Поиск по таблице " + m_tableName);
    dlg.setFixedSize(400, 300);
    QVBoxLayout *l = new QVBoxLayout(&dlg);

    QStringList cols = m_dbManager->getColumnList(m_tableName);
    QMap<QString, QLineEdit*> edits;

    foreach(const QString &col, cols) {
        QHBoxLayout *row = new QHBoxLayout();
        row->addWidget(new QLabel(col + ":", &dlg));
        QLineEdit *e = new QLineEdit(&dlg);
        // Если уже был введен фильтр, показываем его
        if (m_filterWidgets.contains(col)) e->setText(static_cast<QLineEdit*>(m_filterWidgets[col])->text());
        row->addWidget(e);
        edits[col] = e;
        l->addLayout(row);
    }

    QPushButton *apply = new QPushButton("Применить (Enter)", &dlg);
    connect(apply, &QPushButton::clicked, &dlg, &QDialog::accept);
    l->addWidget(apply);

    if (dlg.exec() == QDialog::Accepted) {
        // Переносим значения из диалога в наши скрытые/реальные виджеты фильтров
        foreach(const QString &col, cols) {
            if (!m_filterWidgets.contains(col)) m_filterWidgets[col] = new QLineEdit(this);
            static_cast<QLineEdit*>(m_filterWidgets[col])->setText(edits[col]->text());
        }
        applyFilters();
    }
}

void TableViewWindow::applyFilters()
{
    QString where;
    QMapIterator<QString, QWidget*> i(m_filterWidgets);
    while (i.hasNext()) {
        i.next();
        QLineEdit *e = qobject_cast<QLineEdit*>(i.value());
        if (!e || e->text().isEmpty()) continue;
        if (!where.isEmpty()) where += " AND ";
        where += QString("%1::text ILIKE '%%2%'").arg(i.key()).arg(e->text().replace("'", "''"));
    }
    loadData(where);
}

void TableViewWindow::loadData(const QString &where)
{
    m_table->setRowCount(0);
    QJsonArray data;
    QStringList cols = m_dbManager->getColumnList(m_tableName);

    if (m_dbManager->isHttpMode()) {
        data = m_dbManager->fetchTableDataHttp(m_tableName, where);
    } else {
        QString sql = "SELECT * FROM " + m_tableName;
        if (!where.isEmpty()) sql += " WHERE " + where;
        QSqlQuery q = m_dbManager->executeQuery(sql);
        while(q.next()) {
            QJsonObject o;
            for(int i=0; i<cols.size(); ++i) o[cols[i]] = QJsonValue::fromVariant(q.value(i));
            data.append(o);
        }
    }

    m_table->setColumnCount(cols.size());
    m_table->setHorizontalHeaderLabels(cols);
    m_table->setRowCount(data.size());

    for(int i=0; i<data.size(); ++i) {
        QJsonObject o = data[i].toObject();
        for(int j=0; j<cols.size(); ++j) {
            QTableWidgetItem *item = new QTableWidgetItem(o[cols[j]].toVariant().toString());
            item->setForeground(QBrush(Qt::black));
            m_table->setItem(i, j, item);
        }
    }
    m_table->resizeColumnsToContents();
    m_statusLabel->setText(QString("Записей: %1").arg(data.size()));
}

void TableViewWindow::addRecord() { RecordDialog d(m_dbManager, m_tableName, this); if(d.exec()==QDialog::Accepted) loadData(); }
void TableViewWindow::editRecord() {
    int r = m_table->currentRow(); if(r<0) return;
    int id = m_table->item(r, 0)->text().toInt();
    RecordDialog d(m_dbManager, m_tableName, this, id); if(d.exec()==QDialog::Accepted) loadData();
}
void TableViewWindow::deleteRecord() {
    int r = m_table->currentRow(); if(r<0) return;
    if (QMessageBox::question(this, "Удаление", "Удалить?") != QMessageBox::Yes) return;
    int id = m_table->item(r, 0)->text().toInt();
    if (m_dbManager->deleteRecordHttp(m_tableName, id)) loadData();
}

void TableViewWindow::updateButtonStates() {
    bool s = m_table->currentRow() >= 0;
    if(m_editBtn) m_editBtn->setEnabled(s);
    if(m_deleteBtn) m_deleteBtn->setEnabled(s);
}

void TableViewWindow::setupStyles() {
    if (m_isClassicUI) {
        setStyleSheet("QWidget { background-color: #c0c0c0; color: black; }"
                      "QTableWidget { background-color: white; border: 2px inset gray; font-family: 'Consolas'; color: black; }"
                      "QTableWidget::item { color: black; }"
                      "QHeaderView::section { background-color: #c0c0c0; color: black; border: 1px solid black; }"
                      "QMenuBar { background-color: #c0c0c0; border-bottom: 1px solid black; color: black; }");
    } else {
        setStyleSheet(
            "QWidget { background-color: #f8f9fa; color: #2c3e50; }"
            "QLabel#modernTitle { font-size: 20px; font-weight: bold; color: #2c3e50; }"
            "QLabel#filterHeader { font-weight: bold; margin-top: 10px; color: #34495e; }"
            "QScrollArea#filterArea { border: 1px solid #dee2e6; background-color: white; border-radius: 4px; }"
            "QWidget#filterContent { background-color: white; }"
            "QLineEdit { background-color: white; color: #2c3e50; border: 1px solid #ced4da; border-radius: 4px; padding: 5px; selection-background-color: #3498db; }"
            "QLineEdit:focus { border: 1px solid #3498db; }"
            "QPushButton { background-color: #3498db; color: white; border-radius: 4px; padding: 8px; font-weight: bold; border: none; }"
            "QPushButton:hover { background-color: #2980b9; }"
            "QPushButton:pressed { background-color: #21618c; }"
            "QTableWidget { background-color: white; color: black; border: 1px solid #dee2e6; gridline-color: #f1f1f1; selection-background-color: #e3f2fd; selection-color: #1976d2; }"
            "QTableWidget::item { padding: 5px; color: black; }"
            "QHeaderView::section { background-color: #f1f3f5; color: #495057; border: none; border-bottom: 2px solid #dee2e6; padding: 5px; font-weight: bold; }"
            "QScrollBar:vertical { border: none; background: #f1f1f1; width: 10px; margin: 0px; }"
            "QScrollBar::handle:vertical { background: #ced4da; min-height: 20px; border-radius: 5px; }"
            "QScrollBar::handle:vertical:hover { background: #adb5bd; }"
        );
    }
}

void TableViewWindow::onFilterChanged() { m_filterTimer->start(); }
void TableViewWindow::goBack() { hide(); }
void TableViewWindow::exportToXlsx() {
    QString p = QFileDialog::getSaveFileName(this, "Save", "", "Excel (*.xlsx)");
    if(p.isEmpty()) return;
    Document xlsx;
    for(int c=0; c<m_table->columnCount(); ++c) xlsx.write(1, c+1, m_table->horizontalHeaderItem(c)->text());
    for(int r=0; r<m_table->rowCount(); ++r)
        for(int c=0; c<m_table->columnCount(); ++c) xlsx.write(r+2, c+1, m_table->item(r, c)->text());
    xlsx.saveAs(p);
}

QString TableViewWindow::getDisplayName(const QString &f) const { return f; }
