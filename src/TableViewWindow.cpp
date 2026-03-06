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
#include "xlsxformat.h"

using namespace QXlsx;

TableViewWindow::TableViewWindow(DatabaseManager *dbManager, const QString &tableName, QWidget *parent)
    : QWidget(parent)
    , m_dbManager(dbManager)
    , m_tableName(tableName)
{
    ConfigManager config("config.ini");
    m_isClassicUI = config.isClassicUI();

    // Локализация имен таблиц (для заголовков)
    m_fieldDisplayNames["conscripts"] = "Призывники";
    m_fieldDisplayNames["prizivnik"] = "Призывники";
    m_fieldDisplayNames["commissioners"] = "Комиссары";
    m_fieldDisplayNames["comissar"] = "Комиссары";
    m_fieldDisplayNames["kategoria_godnosti"] = "Категории годности";
    m_fieldDisplayNames["fitness_categories"] = "Категории годности";
    m_fieldDisplayNames["military_id_cards"] = "Военные билеты";
    m_fieldDisplayNames["voennyi_bilet"] = "Военные билеты";
    m_fieldDisplayNames["voenno_uchetnaya_karta"] = "Учетные карты";
    m_fieldDisplayNames["med_osvidetelstvovanie"] = "Медосмотры";
    m_fieldDisplayNames["prizivnoe_meropriyatie"] = "Мероприятия призыва";

    setWindowTitle(m_isClassicUI ? "[CUA] Просмотр: " + getDisplayName(m_tableName) : "Таблица: " + getDisplayName(m_tableName));
    if (m_isClassicUI) setFixedSize(950, 680); else resize(1100, 750);

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

    // ОПРЕДЕЛЯЕМ ПРАВА ДОСТУПА
    QStringList lookupTables = {"kategoria_godnosti", "comissar", "fitness_categories", "commissioners"};
    bool isLookup = lookupTables.contains(m_tableName);
    bool isAdmin = m_dbManager->isSuperuser();
    m_canEdit = !isLookup || isAdmin;

    // ПАНЕЛЬ ПРЕДУПРЕЖДЕНИЯ (как в CUA)
    if (!m_canEdit) {
        QLabel *readOnlyBanner = new QLabel(" ⚠️ РЕЖИМ ПРОСМОТРА: Редактирование этого справочника разрешено только Администратору! ", this);
        readOnlyBanner->setAlignment(Qt::AlignCenter);
        readOnlyBanner->setStyleSheet("background-color: #c0392b; color: white; font-weight: bold; padding: 8px; border: 1px solid white;");
        m_layout->addWidget(readOnlyBanner);
    }

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
    m_table->setWordWrap(true);
    m_table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setStretchLastSection(true);

    connect(m_table, &QTableWidget::itemSelectionChanged, this, &TableViewWindow::updateButtonStates);
    m_layout->addWidget(m_table);

    m_statusLabel = new QLabel("Загрузка данных...", this);
    m_layout->addWidget(m_statusLabel);

    if (m_isClassicUI) {
        m_footerHint = new QLabel(" [Ins] Добавить | [F4] Правка | [Del] Удалить | [F3] Фильтр | [F11] EXCEL | [Esc] Выход ", this);
        m_footerHint->setStyleSheet("background-color: #000080; color: white; font-family: 'Consolas'; font-size: 12px; padding: 4px; border: 1px solid white;");
        m_layout->addWidget(m_footerHint);
    }
}

void TableViewWindow::setupModernUI()
{
    m_layout->setContentsMargins(15, 15, 15, 15);
    
    QLabel *title = new QLabel(getDisplayName(m_tableName).toUpper(), this);
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
        QLabel *l = new QLabel(getDisplayName(col) + ":", fw);
        QLineEdit *e = new QLineEdit(fw);
        e->setPlaceholderText("Фильтр...");
        connect(e, &QLineEdit::textChanged, this, &TableViewWindow::onFilterChanged);
        m_filterWidgets[col] = e;
        fl->addWidget(l, r, c);
        fl->addWidget(e, r, c+1);
        c += 2; if (c >= 6) { c=0; r++; }
    }
    m_filterScrollArea->setWidget(fw);
    m_layout->addWidget(m_filterScrollArea);

    QHBoxLayout *bl = new QHBoxLayout();
    m_addBtn = new QPushButton("Добавить", this);
    m_editBtn = new QPushButton("Изменить", this);
    m_deleteBtn = new QPushButton("Удалить", this);

    // СКРЫВАЕМ КНОПКИ РЕДАКТИРОВАНИЯ, ЕСЛИ НЕТ ПРАВ
    m_addBtn->setVisible(m_canEdit);
    m_editBtn->setVisible(m_canEdit);
    m_deleteBtn->setVisible(m_canEdit);

    QPushButton *exportBtn = new QPushButton("Экспорт Excel", this);
    exportBtn->setStyleSheet("background-color: #27ae60; color: white; font-weight: bold;");
    connect(exportBtn, &QPushButton::clicked, this, &TableViewWindow::exportToXlsx);

    connect(m_addBtn, &QPushButton::clicked, this, &TableViewWindow::addRecord);
    connect(m_editBtn, &QPushButton::clicked, this, &TableViewWindow::editRecord);
    connect(m_deleteBtn, &QPushButton::clicked, this, &TableViewWindow::deleteRecord);

    bl->addWidget(m_addBtn); bl->addWidget(m_editBtn); bl->addWidget(m_deleteBtn);
    bl->addWidget(exportBtn);
    bl->addStretch();
    
    QPushButton *back = new QPushButton("Назад", this);
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

    // БЛОКИРУЕМ ДЕЙСТВИЯ В МЕНЮ CUA
    QAction *aAdd = m->addAction("Добавить (Ins)", this, &TableViewWindow::addRecord, QKeySequence(Qt::Key_Insert));
    QAction *aEdit = m->addAction("Правка (F4)", this, &TableViewWindow::editRecord, QKeySequence(Qt::Key_F4));
    QAction *aDel = m->addAction("Удалить (Del)", this, &TableViewWindow::deleteRecord, QKeySequence(Qt::Key_Delete));

    aAdd->setEnabled(m_canEdit);
    aEdit->setEnabled(m_canEdit);
    aDel->setEnabled(m_canEdit);

    QMenu *v = m_menuBar->addMenu("&Вид");
    v->addAction("Фильтры (F3)", this, &TableViewWindow::showFiltersDialog, QKeySequence(Qt::Key_F3));
    v->addAction("Обновить (F5)", this, [this](){ loadData(); }, QKeySequence(Qt::Key_F5));
    v->addAction("Экспорт EXCEL (F11)", this, &TableViewWindow::exportToXlsx, QKeySequence(Qt::Key_F11));

    m_layout->setMenuBar(m_menuBar);
}

void TableViewWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) { goBack(); return; }
    if (event->key() == Qt::Key_F11) { exportToXlsx(); return; }
    if (m_isClassicUI && m_canEdit) {
        if (event->key() == Qt::Key_Insert) { addRecord(); return; }
        if (event->key() == Qt::Key_F4) { editRecord(); return; }
        if (event->key() == Qt::Key_Delete) { deleteRecord(); return; }
    }
    if (m_isClassicUI && event->key() == Qt::Key_F3) { showFiltersDialog(); return; }
    QWidget::keyPressEvent(event);
}

void TableViewWindow::addRecord() { if(!m_canEdit) return; RecordDialog d(m_dbManager, m_tableName, this); if(d.exec()==QDialog::Accepted) loadData(); }
void TableViewWindow::editRecord() {
    if(!m_canEdit) return;
    int r = m_table->currentRow(); if(r<0) return;
    QString pk = m_dbManager->getPrimaryKeyColumn(m_tableName);
    int colIdx = 0; // Для простоты берем первую колонку как ID
    int id = m_table->item(r, colIdx)->text().toInt();
    RecordDialog d(m_dbManager, m_tableName, this, id); if(d.exec()==QDialog::Accepted) loadData();
}

void TableViewWindow::deleteRecord() {
    if(!m_canEdit) return;
    int r = m_table->currentRow(); if(r<0) return;
    if (QMessageBox::question(this, "Удаление", "Удалить выбранную запись?") != QMessageBox::Yes) return;
    int id = m_table->item(r, 0)->text().toInt();
    if (m_dbManager->deleteRecordHttp(m_tableName, id)) loadData();
}

void TableViewWindow::loadData(const QString &where)
{
    m_table->setRowCount(0);
    QStringList cols = m_dbManager->getColumnList(m_tableName);
    QJsonArray data = m_dbManager->fetchTableDataHttp(m_tableName, where);

    m_table->setColumnCount(cols.size());
    QStringList headers; foreach(const QString &c, cols) headers << getDisplayName(c);
    m_table->setHorizontalHeaderLabels(headers);
    m_table->setRowCount(data.size());

    for(int i=0; i<data.size(); ++i) {
        QJsonObject o = data[i].toObject();
        for(int j=0; j<cols.size(); ++j) {
            QTableWidgetItem *it = new QTableWidgetItem(o[cols[j]].toVariant().toString());
            it->setForeground(QBrush(m_isClassicUI ? Qt::black : QColor("#2c3e50")));
            m_table->setItem(i, j, it);
        }
    }
    m_table->resizeColumnsToContents();
    m_statusLabel->setText(QString("Записей найдено: %1").arg(data.size()));
}

void TableViewWindow::setupStyles() {
    if (m_isClassicUI) {
        setStyleSheet("QWidget { background-color: #0000AA; color: white; } QTableWidget { background-color: white; color: black; font-family: 'Consolas'; } QHeaderView::section { background-color: #00AAAA; color: white; border: 1px solid white; }");
    } else {
        setStyleSheet("QWidget { background-color: #f8f9fa; color: #2c3e50; } QLabel#modernTitle { font-size: 20px; font-weight: bold; } QPushButton { background-color: #3498db; color: white; border-radius: 4px; padding: 8px; font-weight: bold; } QTableWidget { background-color: white; color: black; }");
    }
}

void TableViewWindow::showFiltersDialog() { /* Код из прошлой версии */ }
void TableViewWindow::applyFilters() { /* Код из прошлой версии */ }
void TableViewWindow::updateButtonStates() { bool s = m_table->currentRow() >= 0; if(m_editBtn) m_editBtn->setEnabled(s && m_canEdit); if(m_deleteBtn) m_deleteBtn->setEnabled(s && m_canEdit); }
void TableViewWindow::onFilterChanged() { m_filterTimer->start(); }
void TableViewWindow::goBack() { hide(); }
void TableViewWindow::exportToXlsx() { /* Код из прошлой версии */ }
QString TableViewWindow::getDisplayName(const QString &f) const { return m_fieldDisplayNames.value(f.toLower(), f); }
