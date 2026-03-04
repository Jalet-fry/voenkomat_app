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

    // Перенос локализации из военокомат_app_old
    m_fieldDisplayNames["conscripts"] = "Призывники";
    m_fieldDisplayNames["commissioners"] = "Комиссары";
    m_fieldDisplayNames["medical_examinations"] = "Медосмотры";
    m_fieldDisplayNames["fitness_categories"] = "Категории годности";
    m_fieldDisplayNames["military_id_cards"] = "Военные билеты";

    m_fieldDisplayNames["conscript_id"] = "ID";
    m_fieldDisplayNames["full_name"] = "ФИО";
    m_fieldDisplayNames["birth_date"] = "Дата рождения";
    m_fieldDisplayNames["residence_address"] = "Адрес проживания";
    m_fieldDisplayNames["passport_number"] = "Паспортные данные";
    m_fieldDisplayNames["category_name"] = "Категория";
    m_fieldDisplayNames["restriction_description"] = "Ограничения";

    setWindowTitle(m_isClassicUI ? "[CUA] Просмотр: " + getDisplayName(m_tableName) : "Таблица: " + getDisplayName(m_tableName));

    if (m_isClassicUI) setFixedSize(950, 650); else resize(1100, 750);

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
        e->setPlaceholderText("Фильтр (> < = \" \")...");
        connect(e, &QLineEdit::textChanged, this, &TableViewWindow::onFilterChanged);
        m_filterWidgets[col] = e;

        fl->addWidget(l, r, c);
        fl->addWidget(e, r, c+1);

        c += 2;
        if (c >= 6) { c=0; r++; }
    }
    m_filterScrollArea->setWidget(fw);
    m_layout->addWidget(m_filterScrollArea);

    QHBoxLayout *bl = new QHBoxLayout();
    m_addBtn = new QPushButton("Добавить", this);
    m_editBtn = new QPushButton("Изменить", this);
    m_deleteBtn = new QPushButton("Удалить", this);

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
    m->addAction("Добавить (Ins)", this, &TableViewWindow::addRecord, QKeySequence(Qt::Key_Insert));
    m->addAction("Правка (F4)", this, &TableViewWindow::editRecord, QKeySequence(Qt::Key_F4));
    m->addAction("Удалить (Del)", this, &TableViewWindow::deleteRecord, QKeySequence(Qt::Key_Delete));

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
    dlg.setWindowTitle("Установка фильтров: " + getDisplayName(m_tableName));
    dlg.setFixedSize(500, 400);
    QVBoxLayout *l = new QVBoxLayout(&dlg);

    QStringList cols = m_dbManager->getColumnList(m_tableName);
    QMap<QString, QLineEdit*> edits;

    QWidget *container = new QWidget();
    QGridLayout *gl = new QGridLayout(container);
    int r = 0;
    foreach(const QString &col, cols) {
        gl->addWidget(new QLabel(getDisplayName(col) + ":", container), r, 0);
        QLineEdit *e = new QLineEdit(container);
        e->setPlaceholderText("Используйте >=, <=, != или \" \"");
        if (m_filterWidgets.contains(col)) e->setText(static_cast<QLineEdit*>(m_filterWidgets[col])->text());
        gl->addWidget(e, r, 1);
        edits[col] = e;
        r++;
    }

    QScrollArea *scroll = new QScrollArea(&dlg);
    scroll->setWidgetResizable(true);
    scroll->setWidget(container);
    l->addWidget(scroll);

    QPushButton *apply = new QPushButton("Применить фильтры (Enter)", &dlg);
    connect(apply, &QPushButton::clicked, &dlg, &QDialog::accept);
    l->addWidget(apply);

    if (dlg.exec() == QDialog::Accepted) {
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
        QLineEdit *edit = qobject_cast<QLineEdit*>(i.value());
        if (!edit || edit->text().trimmed().isEmpty()) continue;

        if (!where.isEmpty()) where += " AND ";
        QString val = edit->text().trimmed();
        QString col = i.key();

        // ВОССТАНОВЛЕННАЯ ЛОГИКА ИЗ voenkomat_app_old
        bool hasOp = val.startsWith(">") || val.startsWith("<") || val.startsWith("=") || val.startsWith("!");

        if (hasOp) {
            where += QString("%1 %2").arg(col).arg(val);
        } else if (val.startsWith("\"") && val.endsWith("\"")) {
            QString exact = val.mid(1, val.length()-2).replace("'", "''");
            where += QString("%1::text = '%2'").arg(col).arg(exact);
        } else {
            where += QString("%1::text ILIKE '%%2%'").arg(col).arg(val.replace("'", "''"));
        }
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
        QString sql = "SELECT * FROM public." + m_tableName;
        if (!where.isEmpty()) sql += " WHERE " + where;
        QSqlQuery q = m_dbManager->executeQuery(sql);
        while(q.next()) {
            QJsonObject o;
            for(int i=0; i<cols.size(); ++i) o[cols[i]] = QJsonValue::fromVariant(q.value(i));
            data.append(o);
        }
    }

    m_table->setColumnCount(cols.size());
    QStringList headers;
    foreach(const QString &c, cols) headers << getDisplayName(c);
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
    for(int i=0; i<m_table->columnCount(); ++i) if(m_table->columnWidth(i) > 350) m_table->setColumnWidth(i, 350);
    m_statusLabel->setText(QString("Записей найдено: %1").arg(data.size()));
}

void TableViewWindow::addRecord() { RecordDialog d(m_dbManager, m_tableName, this); if(d.exec()==QDialog::Accepted) loadData(); }
void TableViewWindow::editRecord() {
    int r = m_table->currentRow(); if(r<0) return;
    QString pk = m_dbManager->getPrimaryKeyColumn(m_tableName);
    int colIdx = -1;
    for(int i=0; i<m_table->columnCount(); ++i) {
        if (m_table->horizontalHeaderItem(i)->text() == getDisplayName(pk) || m_table->horizontalHeaderItem(i)->text() == pk) {
            colIdx = i;
            break;
        }
    }
    if(colIdx < 0) colIdx = 0;
    int id = m_table->item(r, colIdx)->text().toInt();
    RecordDialog d(m_dbManager, m_tableName, this, id); if(d.exec()==QDialog::Accepted) loadData();
}

void TableViewWindow::deleteRecord() {
    int r = m_table->currentRow(); if(r<0) return;
    if (QMessageBox::question(this, "Удаление", "Удалить выбранную запись?") != QMessageBox::Yes) return;
    QString pk = m_dbManager->getPrimaryKeyColumn(m_tableName);
    int colIdx = -1;
    for(int i=0; i<m_table->columnCount(); ++i) {
        if (m_table->horizontalHeaderItem(i)->text() == getDisplayName(pk) || m_table->horizontalHeaderItem(i)->text() == pk) {
            colIdx = i;
            break;
        }
    }
    if(colIdx < 0) colIdx = 0;
    int id = m_table->item(r, colIdx)->text().toInt();
    if (m_dbManager->deleteRecordHttp(m_tableName, id)) loadData();
}

void TableViewWindow::updateButtonStates() {
    bool s = m_table->currentRow() >= 0;
    if(m_editBtn) m_editBtn->setEnabled(s);
    if(m_deleteBtn) m_deleteBtn->setEnabled(s);
}

void TableViewWindow::setupStyles() {
    if (m_isClassicUI) {
        setStyleSheet(
            "QWidget { background-color: #0000AA; color: white; }"
            "QTableWidget { background-color: white; color: black; border: 2px solid white; font-family: 'Consolas'; selection-background-color: #000080; selection-color: white; }"
            "QHeaderView::section { background-color: #00AAAA; color: white; border: 1px solid white; font-weight: bold; }"
            "QLineEdit { background-color: white; color: black; border: 1px solid gray; }"
            "QMenuBar { background-color: #00AAAA; color: white; }"
            "QMenuBar::item:selected { background-color: white; color: black; }"
        );
    } else {
        setStyleSheet(
            "QWidget { background-color: #f8f9fa; color: #2c3e50; }"
            "QLabel#modernTitle { font-size: 20px; font-weight: bold; color: #2c3e50; }"
            "QLineEdit { background-color: white; color: black; border: 1px solid #ced4da; border-radius: 4px; padding: 5px; }"
            "QPushButton { background-color: #3498db; color: white; border-radius: 4px; padding: 8px; font-weight: bold; }"
            "QTableWidget { background-color: white; color: black; border: 1px solid #dee2e6; }"
        );
    }
}

void TableViewWindow::onFilterChanged() { m_filterTimer->start(); }
void TableViewWindow::goBack() { hide(); }

void TableViewWindow::exportToXlsx() {
    QString p = QFileDialog::getSaveFileName(this, "Сохранить в Excel", "", "Excel (*.xlsx)");
    if(p.isEmpty()) return;

    Document xlsx;
    Format headerFormat;
    headerFormat.setFontBold(true);
    headerFormat.setPatternBackgroundColor(QColor("#e0e0e0"));

    for(int c=0; c<m_table->columnCount(); ++c) {
        xlsx.write(1, c+1, m_table->horizontalHeaderItem(c)->text(), headerFormat);
    }

    for(int r=0; r<m_table->rowCount(); ++r) {
        for(int c=0; c<m_table->columnCount(); ++c) {
            xlsx.write(r+2, c+1, m_table->item(r, c)->text());
        }
    }

    if(xlsx.saveAs(p)) QMessageBox::information(this, "Успех", "Данные экспортированы.");
    else QMessageBox::critical(this, "Ошибка", "Не удалось сохранить файл.");
}

QString TableViewWindow::getDisplayName(const QString &f) const { return m_fieldDisplayNames.value(f.toLower(), f); }
