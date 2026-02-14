#include "QueriesWindow.h"
#include "QueryResultWindow.h"
#include "EmbeddedQueries.h"
#include "DbConstants.h"
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
#include <QDesktopWidget>
#include <QJsonArray>
#include <QJsonObject>
#include <algorithm>

QueriesWindow::QueriesWindow(DatabaseManager *dbManager, QWidget *parent)
    : QWidget(parent)
    , m_dbManager(dbManager)
{
    setWindowTitle("Управление запросами (Military DB)");
    resize(1000, 750);

    setGeometry(
        QStyle::alignedRect(
            Qt::LeftToRight,
            Qt::AlignCenter,
            size(),
            qApp->desktop()->availableGeometry()
        )
    );

    setupUI();
    setupStyles();
    loadQueries();
}

QueriesWindow::~QueriesWindow() {}

void QueriesWindow::setupUI()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(20, 20, 20, 20);
    m_layout->setSpacing(15);

    QLabel *title = new QLabel("Список всех SQL запросов (Лабораторные 5 и 6)", this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 22px; font-weight: bold; color: #2c3e50;");
    m_layout->addWidget(title);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels(QStringList() << "Номер" << "Лаб." << "Описание запроса" << "Действие");
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);

    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_table->setColumnWidth(0, 80);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_table->setColumnWidth(1, 80);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_table->setColumnWidth(3, 130);

    m_layout->addWidget(m_table);

    QPushButton *backBtn = new QPushButton("Назад в меню", this);
    backBtn->setMinimumHeight(45);
    connect(backBtn, &QPushButton::clicked, this, &QueriesWindow::goBack);
    m_layout->addWidget(backBtn);
}

void QueriesWindow::setupStyles()
{
    setStyleSheet(
        "QWidget { background-color: #f5f6fa; }"
        "QTableWidget { background-color: white; border: 1px solid #dcdde1; border-radius: 6px; gridline-color: #f1f2f6; }"
        "QHeaderView::section { background-color: #2f3640; color: white; padding: 8px; font-weight: bold; border: none; }"
        "QPushButton { background-color: #0097e6; color: white; border-radius: 4px; font-weight: bold; border: none; }"
        "QPushButton:hover { background-color: #00a8ff; }"
        "QPushButton#runBtn { background-color: #44bd32; }"
        "QPushButton#runBtn:hover { background-color: #4cd137; }"
    );
}

QString QueriesWindow::parseQueryDescription(const QString &sqlText) const
{
    QStringList lines = sqlText.split('\n');
    foreach(QString line, lines) {
        QString t = line.trimmed();
        if (t.startsWith("--")) {
            QString desc = t.mid(2).trimmed();
            desc.remove(QRegExp("^(Запрос|Задание)\\s*\\d+\\.\\d+(\\.\\d+)?[:]?\\s*", Qt::CaseInsensitive));
            desc.remove(QRegExp("^\\d+\\.\\d+(\\.\\d+)?[:]?\\s*"));
            if (desc.isEmpty()) continue;
            return desc;
        }
    }
    return "Описание отсутствует";
}

void QueriesWindow::loadQueries()
{
    m_allQueries.clear();
    m_allQueries = getEmbeddedQueries();

    QDir baseDir(qApp->applicationDirPath());
    baseDir.cdUp();
    QString queriesPath = baseDir.absoluteFilePath("resources/queries");

    QStringList subDirs; subDirs << "Lab5" << "Lab6";
    foreach(const QString &sub, subDirs) {
        QDir dir(queriesPath + "/" + sub);
        if (!dir.exists()) continue;

        foreach(const QFileInfo &fi, dir.entryInfoList(QStringList() << "*.sql", QDir::Files)) {
            bool found = false;
            for(int i=0; i < m_allQueries.size(); ++i) {
                if(m_allQueries[i].number == fi.completeBaseName()) {
                    found = true;
                    break;
                }
            }
            if(!found) {
                QFile f(fi.absoluteFilePath());
                if(f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QString sql = QTextStream(&f).readAll();
                    QueryInfo qi;
                    qi.number = fi.completeBaseName();
                    qi.description = parseQueryDescription(sql);
                    qi.sqlText = sql;
                    qi.type = sub;
                    m_allQueries.append(qi);
                }
            }
        }
    }

    std::sort(m_allQueries.begin(), m_allQueries.end(), [](const QueryInfo &a, const QueryInfo &b) {
        QStringList aParts = a.number.split('.');
        QStringList bParts = b.number.split('.');
        for (int i = 0; i < qMin(aParts.size(), bParts.size()); ++i) {
            int aVal = aParts[i].toInt();
            int bVal = bParts[i].toInt();
            if (aVal != bVal) return aVal < bVal;
        }
        return aParts.size() < bParts.size();
    });

    refreshTable();
}

void QueriesWindow::refreshTable()
{
    m_table->setRowCount(m_allQueries.size());
    for (int i = 0; i < m_allQueries.size(); ++i) {
        const QueryInfo &q = m_allQueries[i];
        m_table->setItem(i, 0, new QTableWidgetItem(q.number));
        m_table->setItem(i, 1, new QTableWidgetItem(q.type));
        m_table->setItem(i, 2, new QTableWidgetItem(q.description));
        QPushButton *runBtn = new QPushButton("Выполнить", this);
        runBtn->setObjectName("runBtn");
        runBtn->setMinimumHeight(30);
        connect(runBtn, &QPushButton::clicked, [this, q]() { runQuery(q); });
        m_table->setCellWidget(i, 3, runBtn);
    }
}

void QueriesWindow::runQuery(const QueryInfo &queryInfo)
{
    if (!m_dbManager->isConnected()) return;

    QStringList cols;
    QList<QList<QVariant>> rows;

    if (m_dbManager->isHttpMode()) {
        bool ok;
        QJsonArray data = m_dbManager->executeCustomQueryHttp(queryInfo.sqlText, &ok);
        if (!ok) {
            QMessageBox::critical(this, "Ошибка API", "Запрос не выполнен:\n" + m_dbManager->lastError());
            return;
        }
        if (data.isEmpty()) {
            QMessageBox::information(this, "Результат", "Запрос выполнен, данных нет.");
            return;
        }
        // Получаем колонки из первого объекта
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
        QSqlQuery query = m_dbManager->executeQuery(queryInfo.sqlText, &ok);
        if (!ok) {
            QMessageBox::critical(this, "Ошибка SQL", "Запрос не выполнен:\n" + m_dbManager->lastError());
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

    QueryResultWindow *res = new QueryResultWindow(queryInfo.number + ": " + queryInfo.description, cols, rows, m_dbManager, this);
    res->resize(900, 600);
    res->show();
}

void QueriesWindow::goBack() { hide(); }
void QueriesWindow::addNewQuery() {}
void QueriesWindow::showQueryContextMenu(const QPoint &) {}
void QueriesWindow::deleteQuery(const QueryInfo &) {}
void QueriesWindow::backupQuery(const QueryInfo &) {}
void QueriesWindow::onTableDoubleClicked(const QModelIndex &) {}
void QueriesWindow::onFilterChanged() {}
void QueriesWindow::onSearchTextChanged(const QString &) {}
void QueriesWindow::loadQueriesFromFolder(const QString &, const QString &) {}
