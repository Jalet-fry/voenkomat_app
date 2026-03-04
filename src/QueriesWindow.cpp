#include "QueriesWindow.h"
#include "QueryResultWindow.h"
#include "EmbeddedQueries.h"
#include "ConfigManager.h"
#include <QLabel>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDebug>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPushButton>
#include <QApplication>
#include <QSqlRecord>
#include <QKeyEvent>
#include <QMenuBar>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QRegularExpression>
#else
#include <QRegExp>
#endif
#include <algorithm>

QueriesWindow::QueriesWindow(DatabaseManager *dbManager, QWidget *parent)
    : QWidget(parent)
    , m_dbManager(dbManager)
{
    ConfigManager config("config.ini");
    m_isClassicUI = config.isClassicUI();

    setWindowTitle(m_isClassicUI ? "Запросы (CUA)" : "Управление запросами");
    if (m_isClassicUI) setFixedSize(900, 600); else resize(1000, 750);

    setupUI();
    setupStyles();
    loadQueries();
}

QueriesWindow::~QueriesWindow() {}

void QueriesWindow::setupUI()
{
    m_layout = new QVBoxLayout(this);
    if (m_isClassicUI) setupClassicUI(); else setupModernUI();

    m_table = new QTableWidget(this);
    m_table->setColumnCount(m_isClassicUI ? 3 : 4);
    QStringList headers;
    headers << "Номер" << "Лаб." << "Описание запроса";
    if (!m_isClassicUI) headers << "Действие";
    m_table->setHorizontalHeaderLabels(headers);

    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    m_table->horizontalHeader()->setStretchLastSection(true);

    connect(m_table, &QTableWidget::itemDoubleClicked, this, &QueriesWindow::runSelectedQuery);
    m_layout->addWidget(m_table);

    if (m_isClassicUI) {
        m_footerHint = new QLabel(" [Enter] Выполнить | [F3] Фильтр | [Esc] Назад ", this);
        m_footerHint->setStyleSheet("background-color: #000080; color: white; font-family: 'Consolas'; font-size: 11px;");
        m_layout->addWidget(m_footerHint);
    }
}

void QueriesWindow::setupModernUI()
{
    m_layout->setContentsMargins(20, 20, 20, 20);
    m_layout->setSpacing(15);

    QLabel *title = new QLabel("Список всех SQL запросов", this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 22px; font-weight: bold; color: #2c3e50;");
    m_layout->addWidget(title);

    QHBoxLayout *filterL = new QHBoxLayout();
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Поиск по описанию...");
    connect(m_searchEdit, &QLineEdit::textChanged, this, &QueriesWindow::onFilterChanged);
    filterL->addWidget(new QLabel("Поиск:"));
    filterL->addWidget(m_searchEdit);
    m_layout->addLayout(filterL);
}

void QueriesWindow::setupClassicUI()
{
    m_layout->setContentsMargins(2, 2, 2, 2);
    m_layout->setSpacing(0);

    m_menuBar = new QMenuBar(this);
    QMenu *m = m_menuBar->addMenu("&Запрос");
    m->addAction("Выполнить (Enter)", this, &QueriesWindow::runSelectedQuery);
    m->addAction("Назад (Esc)", this, &QueriesWindow::goBack);

    m_layout->setMenuBar(m_menuBar);
}

void QueriesWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) { goBack(); return; }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) { runSelectedQuery(); return; }
    QWidget::keyPressEvent(event);
}

void QueriesWindow::onFilterChanged() { refreshTable(); }

void QueriesWindow::loadQueries()
{
    m_allQueries = getEmbeddedQueries();
    refreshTable();
}

void QueriesWindow::refreshTable()
{
    m_table->setRowCount(0);
    QString searchText = m_searchEdit ? m_searchEdit->text().toLower() : "";

    int row = 0;
    for (const auto &q : m_allQueries) {
        if (!searchText.isEmpty() && !q.description.toLower().contains(searchText)) continue;

        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(q.number));
        m_table->setItem(row, 1, new QTableWidgetItem(q.type));
        m_table->setItem(row, 2, new QTableWidgetItem(q.description));

        if (!m_isClassicUI) {
            QPushButton *btn = new QPushButton("Выполнить", this);
            btn->setStyleSheet("background-color: #44bd32; color: white;");
            connect(btn, &QPushButton::clicked, this, &QueriesWindow::runSelectedQuery);
            m_table->setCellWidget(row, 3, btn);
        }

        for(int c=0; c<m_table->columnCount(); ++c) {
            if(m_table->item(row, c)) m_table->item(row, c)->setForeground(QBrush(Qt::black));
        }
        row++;
    }
    m_table->resizeColumnsToContents();
}

void QueriesWindow::runSelectedQuery()
{
    int r = m_table->currentRow();
    if (r < 0) return;

    QString num = m_table->item(r, 0)->text();
    QueryInfo target;
    for(const auto &q : m_allQueries) if(q.number == num) { target = q; break; }

    if (target.sqlText.isEmpty()) return;

    QStringList cols;
    QList<QList<QVariant>> rows;

    if (m_dbManager->isHttpMode()) {
        bool ok;
        QJsonArray data = m_dbManager->executeCustomQueryHttp(target.sqlText, &ok);
        if (data.isEmpty()) return;
        QJsonObject first = data[0].toObject();
        cols = first.keys();
        for (int i = 0; i < data.size(); ++i) {
            QList<QVariant> row;
            QJsonObject obj = data[i].toObject();
            foreach (const QString &col, cols) row << obj[col].toVariant();
            rows << row;
        }
    } else {
        bool ok;
        QSqlQuery query = m_dbManager->executeQuery(target.sqlText, &ok);
        QSqlRecord rec = query.record();
        for (int i = 0; i < rec.count(); ++i) cols << rec.fieldName(i);
        while (query.next()) {
            QList<QVariant> row;
            for (int i = 0; i < cols.size(); ++i) row << query.value(i);
            rows << row;
        }
    }

    QueryResultWindow *res = new QueryResultWindow(target.number + ": " + target.description, cols, rows, m_dbManager, this);
    res->show();
}

void QueriesWindow::goBack() { hide(); }

void QueriesWindow::setupStyles() {
    if (m_isClassicUI) {
        setStyleSheet("QWidget { background-color: #c0c0c0; color: black; }"
                      "QTableWidget { background-color: white; color: black; border: 2px inset gray; font-family: 'Consolas'; }"
                      "QHeaderView::section { background-color: #c0c0c0; color: black; border: 1px solid black; }");
    } else {
        setStyleSheet("QPushButton { background-color: #0097e6; color: white; border-radius: 4px; padding: 5px; }");
    }
}
