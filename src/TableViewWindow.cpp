#include "TableViewWindow.h"
#include "RecordDialog.h"
#include "BackupManager.h"
#include "DbConstants.h"
#include <QLabel>
#include <QMessageBox>
#include <QHeaderView>
#include <QSqlQuery>
#include <QSqlError>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QLineEdit>
#include <QComboBox>
#include <QScrollArea>
#include <QGridLayout>
#include <QTimer>
#include <QSqlRecord>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QRegularExpressionValidator>
#else
#include <QRegExpValidator>
#endif
#include "xlsxdocument.h"
#include "xlsxformat.h"
using namespace QXlsx;

TableViewWindow::TableViewWindow(DatabaseManager *dbManager, const QString &tableName, QWidget *parent)
    : QWidget(parent)
    , m_dbManager(dbManager)
    , m_tableName(tableName)
    , m_editBtn(nullptr)
    , m_deleteBtn(nullptr)
    , m_filterScrollArea(nullptr)
    , m_filterWidget(nullptr)
    , m_filterLayout(nullptr)
    , m_clearFiltersBtn(nullptr)
    , m_filterTimer(nullptr)
    , m_filterStatusLabel(nullptr)
{
    setWindowTitle("Таблица: " + getDisplayName(m_tableName));
    resize(1100, 750);
    
    // Инициализация отображаемых имен
    m_fieldDisplayNames["conscripts"] = "Призывники";
    m_fieldDisplayNames["commissioners"] = "Комиссары";
    m_fieldDisplayNames["medical_examinations"] = "Медосмотры";
    m_fieldDisplayNames["fitness_categories"] = "Категории годности";
    m_fieldDisplayNames["military_id_cards"] = "Военные билеты";
    m_fieldDisplayNames["service_record_cards"] = "Учетные карты";
    m_fieldDisplayNames["callup_events"] = "Призывные мероприятия";

    m_fieldDisplayNames["conscript_id"] = "ID призывника";
    m_fieldDisplayNames["full_name"] = "ФИО";
    m_fieldDisplayNames["birth_date"] = "Дата рождения";
    m_fieldDisplayNames["residence_address"] = "Адрес";
    m_fieldDisplayNames["passport_number"] = "Паспорт";

    m_filterTimer = new QTimer(this);
    m_filterTimer->setSingleShot(true);
    m_filterTimer->setInterval(600);
    connect(m_filterTimer, &QTimer::timeout, this, &TableViewWindow::applyFilters);
    
    setupUI();
    setupStyles();

    loadData();
    setupFilters();
}

TableViewWindow::~TableViewWindow() {}

void TableViewWindow::setupUI()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setSpacing(10);
    m_layout->setContentsMargins(15, 15, 15, 15);

    QLabel *headerLabel = new QLabel(getDisplayName(m_tableName), this);
    headerLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #2c3e50;");
    m_layout->addWidget(headerLabel);

    m_filterScrollArea = new QScrollArea(this);
    m_filterScrollArea->setMaximumHeight(150);
    m_filterScrollArea->setWidgetResizable(true);
    m_filterScrollArea->setStyleSheet("QScrollArea { border: 1px solid #bdc3c7; border-radius: 5px; background: #fdfdfd; }");

    m_filterWidget = new QWidget();
    m_filterLayout = new QGridLayout(m_filterWidget);
    m_filterLayout->setContentsMargins(10, 10, 10, 10);
    m_filterLayout->setSpacing(8);
    m_filterScrollArea->setWidget(m_filterWidget);

    m_layout->addWidget(new QLabel("Поиск по колонкам (используйте > < = или \"текст\" для точного поиска):", this));
    m_layout->addWidget(m_filterScrollArea);

    m_filterStatusLabel = new QLabel("Загрузка данных...", this);
    m_filterStatusLabel->setStyleSheet("color: #7f8c8d; font-style: italic;");
    m_layout->addWidget(m_filterStatusLabel);

    m_table = new QTableWidget(this);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    m_table->horizontalHeader()->setStretchLastSection(true);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &TableViewWindow::updateButtonStates);
    m_layout->addWidget(m_table);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *addBtn = new QPushButton("Добавить", this);
    connect(addBtn, &QPushButton::clicked, this, &TableViewWindow::addRecord);
    btnLayout->addWidget(addBtn);
    
    m_editBtn = new QPushButton("Изменить", this);
    m_editBtn->setEnabled(false);
    connect(m_editBtn, &QPushButton::clicked, this, &TableViewWindow::editRecord);
    btnLayout->addWidget(m_editBtn);
    
    m_deleteBtn = new QPushButton("Удалить", this);
    m_deleteBtn->setEnabled(false);
    connect(m_deleteBtn, &QPushButton::clicked, this, &TableViewWindow::deleteRecord);
    btnLayout->addWidget(m_deleteBtn);

    btnLayout->addStretch();

    QPushButton *exportBtn = new QPushButton("Экспорт в Excel", this);
    exportBtn->setStyleSheet("background-color: #27ae60; color: white;");
    connect(exportBtn, &QPushButton::clicked, this, &TableViewWindow::exportToXlsx);
    btnLayout->addWidget(exportBtn);
    
    QPushButton *backBtn = new QPushButton("Назад", this);
    connect(backBtn, &QPushButton::clicked, this, &TableViewWindow::goBack);
    btnLayout->addWidget(backBtn);
    
    m_layout->addLayout(btnLayout);
}

void TableViewWindow::setupFilters()
{
    QLayoutItem *item;
    while ((item = m_filterLayout->takeAt(0)) != nullptr) {
        if (item->widget()) delete item->widget();
        delete item;
    }
    m_filterWidgets.clear();

    QStringList columns = m_dbManager->getColumnList(m_tableName);
    if (columns.isEmpty()) return;

    int row = 0; int col = 0;
    const int MAX_COLS_PER_ROW = 3;

    foreach (const QString &colName, columns) {
        QVBoxLayout *vbox = new QVBoxLayout();
        vbox->setSpacing(2);

        QLabel *lab = new QLabel(getDisplayName(colName), m_filterWidget);
        lab->setStyleSheet("font-weight: bold; color: #34495e; font-size: 11px;");

        QLineEdit *edit = new QLineEdit(m_filterWidget);
        edit->setPlaceholderText("Поиск...");
        edit->setStyleSheet("QLineEdit { color: black; background-color: white; padding: 4px; border: 1px solid #ced4da; border-radius: 3px; }");
        connect(edit, &QLineEdit::textChanged, this, &TableViewWindow::onFilterChanged);

        vbox->addWidget(lab);
        vbox->addWidget(edit);

        m_filterLayout->addLayout(vbox, row, col);
        m_filterWidgets[colName] = edit;

        if (++col >= MAX_COLS_PER_ROW) { col = 0; row++; }
    }
}

void TableViewWindow::onFilterChanged() { m_filterTimer->start(); }

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
        QString colName = i.key();

        // Обработка сложных условий (> 10, <= 100)
        bool hasOperator = val.startsWith(">") || val.startsWith("<") || val.startsWith("=") || val.startsWith("!");

        if (hasOperator) {
            where += QString("%1 %2").arg(colName).arg(val);
        } else if (val.startsWith("\"") && val.endsWith("\"")) {
            // ТОЧНЫЙ ПОИСК (например "Годен")
            QString exact = val.mid(1, val.length()-2).replace("'", "''");
            where += QString("%1::text = '%2'").arg(colName).arg(exact);
        } else {
            // ЧАСТИЧНЫЙ ПОИСК (ILIKE)
            QString escapedVal = val.replace("'", "''");
            where += QString("%1::text ILIKE '%%2%'").arg(colName).arg(escapedVal);
        }
    }
    loadData(where);
}

void TableViewWindow::loadData(const QString &filterClause, const QList<QVariant> &params)
{
    Q_UNUSED(params);
    m_table->setRowCount(0);
    QJsonArray data;
    QStringList columns;

    if (m_dbManager->isHttpMode()) {
        data = m_dbManager->fetchTableDataHttp(m_tableName, filterClause);
        columns = m_dbManager->getColumnList(m_tableName);
    } else {
        QString sql = "SELECT * FROM " + DatabaseManager::escapeIdentifier(m_tableName);
        if (!filterClause.isEmpty()) sql += " WHERE " + filterClause;
        sql += " ORDER BY 1 ASC";
        QSqlQuery q = m_dbManager->executeQuery(sql);
        QSqlRecord rec = q.record();
        for(int i=0; i<rec.count(); ++i) columns << rec.fieldName(i);
        while(q.next()) {
            QJsonObject obj;
            for(int i=0; i<columns.size(); ++i) obj[columns[i]] = QJsonValue::fromVariant(q.value(i));
            data.append(obj);
        }
    }

    if (columns.isEmpty() && !data.isEmpty()) columns = data[0].toObject().keys();
    m_table->setColumnCount(columns.size());
    QStringList headers;
    foreach(const QString &c, columns) headers << getDisplayName(c);
    m_table->setHorizontalHeaderLabels(headers);

    m_table->setRowCount(data.size());
    for (int i = 0; i < data.size(); ++i) {
        QJsonObject obj = data[i].toObject();
        for (int j = 0; j < columns.size(); ++j) {
            QTableWidgetItem *item = new QTableWidgetItem(obj[columns[j]].toVariant().toString());
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            item->setForeground(QBrush(Qt::black));
#else
            item->setTextColor(Qt::black);
#endif
            m_table->setItem(i, j, item);
        }
    }
    m_table->resizeColumnsToContents();
    m_filterStatusLabel->setText(QString("Найдено: %1").arg(data.size()));
}

void TableViewWindow::addRecord() {
    RecordDialog dlg(m_dbManager, m_tableName, this);
    if (dlg.exec() == QDialog::Accepted) loadData();
}

void TableViewWindow::editRecord() {
    int row = m_table->currentRow();
    if (row < 0) return;
    int id = m_table->item(row, 0)->text().toInt();
    RecordDialog dlg(m_dbManager, m_tableName, this, id);
    if (dlg.exec() == QDialog::Accepted) loadData();
}

void TableViewWindow::deleteRecord() {
    int row = m_table->currentRow();
    if (row < 0) return;
    if (QMessageBox::question(this, "Удаление", "Удалить запись?") != QMessageBox::Yes) return;
    int id = m_table->item(row, 0)->text().toInt();
    bool ok = m_dbManager->isHttpMode() ? m_dbManager->deleteRecordHttp(m_tableName, id) : false;
    if (!m_dbManager->isHttpMode()) {
        QString idCol = m_dbManager->getPrimaryKeyColumn(m_tableName);
        QString sql = QString("DELETE FROM %1 WHERE %2 = %3").arg(m_tableName).arg(idCol).arg(id);
        m_dbManager->executeQuery(sql, &ok);
    }
    if (ok) loadData();
    else QMessageBox::critical(this, "Ошибка", m_dbManager->lastError());
}

void TableViewWindow::exportToXlsx() {
    QString path = QFileDialog::getSaveFileName(this, "Экспорт", "", "Excel (*.xlsx)");
    if (path.isEmpty()) return;
    Document xlsx;
    for (int c = 0; c < m_table->columnCount(); ++c) xlsx.write(1, c + 1, m_table->horizontalHeaderItem(c)->text());
    for (int r = 0; r < m_table->rowCount(); ++r)
        for (int c = 0; c < m_table->columnCount(); ++c) xlsx.write(r + 2, c + 1, m_table->item(r, c)->text());
    xlsx.saveAs(path);
}

void TableViewWindow::updateButtonStates() {
    bool sel = m_table->currentRow() >= 0;
    if (m_editBtn) m_editBtn->setEnabled(sel);
    if (m_deleteBtn) m_deleteBtn->setEnabled(sel);
}

QString TableViewWindow::getDisplayName(const QString &f) const { return m_fieldDisplayNames.value(f.toLower(), f); }
void TableViewWindow::goBack() { hide(); }
void TableViewWindow::setupStyles() {
    setStyleSheet("QWidget { background-color: #f5f6fa; color: black; }"
                  "QPushButton { background-color: #3498db; color: white; border-radius: 4px; padding: 8px; font-weight: bold; }"
                  "QTableWidget { background-color: white; color: black; gridline-color: #f1f2f6; }");
}
void TableViewWindow::showContextMenu(const QPoint &) {}
void TableViewWindow::clearFilters() {}
void TableViewWindow::exportToCSV() {}
