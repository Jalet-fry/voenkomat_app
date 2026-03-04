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
        setFixedSize(650, 450);
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

    // itemActivated срабатывает и на DoubleClick, и на Enter
    connect(m_table, &QTableWidget::itemActivated, this, &QueriesWindow::runSelectedQuery);
    // Для обратной совместимости
    connect(m_table, &QTableWidget::itemDoubleClicked, this, &QueriesWindow::runSelectedQuery);

    m_layout->addWidget(m_table);

    if (m_isClassicUI) {
        m_footerHint = new QLabel(" [ENTER] Запуск запроса | [ESC] Назад ", this);
        m_footerHint->setStyleSheet("background-color: #000080; color: white; padding: 4px; font-family: 'Consolas'; font-size: 12px; font-weight: bold;");
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
    // QTableWidget перехватывает Enter сам по себе для активации,
    // но на всякий случай оставим обработку здесь для окна
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        runSelectedQuery();
        return;
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

        QTableWidgetItem *it0 = new QTableWidgetItem(q.number);
        QTableWidgetItem *it1 = new QTableWidgetItem(q.type);
        QTableWidgetItem *it2 = new QTableWidgetItem(q.description);

        // В классическом интерфейсе CUA все должно быть максимально контрастным
        it0->setForeground(QBrush(Qt::black));
        it1->setForeground(QBrush(Qt::black));
        it2->setForeground(QBrush(Qt::black));

        m_table->setItem(row, 0, it0);
        m_table->setItem(row, 1, it1);
        m_table->setItem(row, 2, it2);

        if (!m_isClassicUI) {
            QPushButton *btn = new QPushButton("Run", this);
            connect(btn, &QPushButton::clicked, this, &QueriesWindow::runSelectedQuery);
            m_table->setCellWidget(row, 3, btn);
        }
        row++;
    }
    if (m_table->rowCount() > 0) m_table->selectRow(0);
}

void QueriesWindow::runSelectedQuery()
{
    int r = m_table->currentRow();
    if (r < 0) return;

    QString num = m_table->item(r, 0)->text();
    QueryInfo target;
    bool found = false;
    for(const auto &q : m_allQueries) {
        if(q.number == num) { target = q; found = true; break; }
    }

    if (!found || target.sqlText.isEmpty()) return;

    QStringList cols;
    QList<QList<QVariant>> rows;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    if (m_dbManager->isHttpMode()) {
        bool ok;
        QJsonArray data = m_dbManager->executeCustomQueryHttp(target.sqlText, &ok);
        if (data.isEmpty() && !ok) {
            QApplication::restoreOverrideCursor();
            QMessageBox::critical(this, "Ошибка", "Запрос не вернул данных или произошла ошибка.");
            return;
        }
        if (!data.isEmpty()) {
            QJsonObject first = data[0].toObject();
            cols = first.keys();
            for (int i = 0; i < data.size(); ++i) {
                QList<QVariant> row;
                QJsonObject obj = data[i].toObject();
                foreach (const QString &col, cols) row << obj[col].toVariant();
                rows << row;
            }
        }
    } else {
        bool ok;
        QSqlQuery query = m_dbManager->executeQuery(target.sqlText, &ok);
        if (!ok) {
            QApplication::restoreOverrideCursor();
            QMessageBox::critical(this, "Ошибка БД", m_dbManager->lastError());
            return;
        }
        QSqlRecord rec = query.record();
        for (int i = 0; i < rec.count(); ++i) cols << rec.fieldName(i);
        while (query.next()) {
            QList<QVariant> row;
            for (int i = 0; i < cols.size(); ++i) row << query.value(i);
            rows << row;
        }
    }
    QApplication::restoreOverrideCursor();

    if (cols.isEmpty()) {
        QMessageBox::information(this, "Результат", "Запрос выполнен успешно, но не вернул строк.");
        return;
    }

    QueryResultWindow *res = new QueryResultWindow(target.number + ": " + target.description, cols, rows, m_dbManager, this);
    res->show();
}

void QueriesWindow::goBack() { hide(); }

void QueriesWindow::setupStyles() {
    if (m_isClassicUI) {
        setStyleSheet("QWidget { background-color: #c0c0c0; color: black; }"
                      "QTableWidget { background-color: white; border: 2px inset gray; color: black; font-family: 'Consolas'; selection-background-color: #000080; selection-color: white; }"
                      "QHeaderView::section { background-color: #c0c0c0; color: black; border: 1px solid black; }");
    }
}

void QueriesWindow::onFilterChanged() {}
