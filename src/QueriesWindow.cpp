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

QueriesWindow::QueriesWindow(DatabaseManager *dbManager, QWidget *parent)
    : QWidget(parent)
    , m_dbManager(dbManager)
{
    ConfigManager config("config.ini");
    m_isClassicUI = config.isClassicUI();

    setWindowTitle(m_isClassicUI ? "Queries List (CUA)" : "Special Queries");

    if (m_isClassicUI) {
        setFixedSize(600, 400);
    } else {
        resize(800, 600);
    }

    setupUI();
    setupStyles();
    loadQueries();

    // ГАРАНТИРУЕМ ФОКУС
    if (m_table) m_table->setFocus();
}

QueriesWindow::~QueriesWindow() {}

void QueriesWindow::setupUI()
{
    m_layout = new QVBoxLayout(this);
    if (m_isClassicUI) setupClassicUI(); else setupModernUI();

    m_table = new QTableWidget(this);
    m_table->setColumnCount(m_isClassicUI ? 3 : 4);
    QStringList headers;
    headers << "No." << "Lab" << "Description";
    if (!m_isClassicUI) headers << "Run";
    m_table->setHorizontalHeaderLabels(headers);

    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    m_table->horizontalHeader()->setStretchLastSection(true);

    connect(m_table, &QTableWidget::itemDoubleClicked, this, &QueriesWindow::runSelectedQuery);
    m_layout->addWidget(m_table);

    if (m_isClassicUI) {
        m_footerHint = new QLabel(" [Enter] Run Query | [Esc] Close Window ", this);
        m_footerHint->setStyleSheet("background-color: #000080; color: white; padding: 2px; font-family: 'Consolas'; font-size: 11px;");
        m_layout->addWidget(m_footerHint);
    }
}

void QueriesWindow::setupModernUI()
{
    m_layout->setContentsMargins(15, 15, 15, 15);
    QLabel *title = new QLabel("Select Special Query", this);
    title->setStyleSheet("font-size: 18px; font-weight: bold;");
    m_layout->addWidget(title);
}

void QueriesWindow::setupClassicUI()
{
    m_layout->setContentsMargins(2, 2, 2, 2);
    m_layout->setSpacing(0);
}

void QueriesWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) { goBack(); return; }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (m_table->hasFocus()) { runSelectedQuery(); return; }
    }
    QWidget::keyPressEvent(event);
}

void QueriesWindow::loadQueries()
{
    m_allQueries = getEmbeddedQueries();
    refreshTable();
}

void QueriesWindow::refreshTable()
{
    m_table->setRowCount(0);
    int row = 0;
    for (const auto &q : m_allQueries) {
        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(q.number));
        m_table->setItem(row, 1, new QTableWidgetItem(q.type));
        m_table->setItem(row, 2, new QTableWidgetItem(q.description));

        if (!m_isClassicUI) {
            QPushButton *btn = new QPushButton("Run", this);
            connect(btn, &QPushButton::clicked, this, &QueriesWindow::runSelectedQuery);
            m_table->setCellWidget(row, 3, btn);
        }

        for(int c=0; c<m_table->columnCount(); ++c) {
            if(m_table->item(row, c)) m_table->item(row, c)->setForeground(QBrush(Qt::black));
        }
        row++;
    }
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
                      "QTableWidget { background-color: white; border: 2px inset gray; color: black; font-family: 'Consolas'; }");
    }
}

void QueriesWindow::onFilterChanged() {}
