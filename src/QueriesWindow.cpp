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

QueriesWindow::QueriesWindow(DatabaseManager *dbManager, QWidget *parent)
    : QWidget(parent)
    , m_dbManager(dbManager)
{
    ConfigManager config("config.ini");
    m_isClassicUI = config.isClassicUI();

    setWindowTitle(m_isClassicUI ? "[CUA] Запросы" : "Специальные запросы");
    if (m_isClassicUI) resize(850, 600); else resize(1000, 750);

    setupUI();
    setupStyles();
    loadQueries();

    if (m_table) m_table->setFocus();
}

QueriesWindow::~QueriesWindow() {}

void QueriesWindow::setupUI()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(15, 15, 15, 15);
    m_layout->setSpacing(10);

    QLabel *title = new QLabel("СПИСОК SQL ЗАПРОСОВ (LAB 5 / LAB 6)", this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(m_isClassicUI ? "font-size: 18px; font-weight: bold; color: yellow; background: blue; padding: 5px;" : "font-size: 22px; font-weight: bold; color: #2c3e50;");
    m_layout->addWidget(title);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels(QStringList() << "Номер" << "Лабораторная" << "Описание (из файла)");
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_layout->addWidget(m_table);

    connect(m_table, &QTableWidget::itemActivated, this, &QueriesWindow::runSelectedQuery);
    connect(m_table, &QTableWidget::itemDoubleClicked, this, &QueriesWindow::runSelectedQuery);

    QHBoxLayout *bl = new QHBoxLayout();
    QPushButton *backBtn = new QPushButton(m_isClassicUI ? "[ ESC - НАЗАД ]" : "← Вернуться", this);
    connect(backBtn, &QPushButton::clicked, this, &QueriesWindow::goBack);
    bl->addStretch();
    bl->addWidget(backBtn);
    m_layout->addLayout(bl);

    if (m_isClassicUI) {
        m_footerHint = new QLabel(" [ENTER] - Выполнить выделенный запрос ", this);
        m_footerHint->setStyleSheet("background-color: #000080; color: white; padding: 4px; font-family: 'Consolas';");
        m_layout->addWidget(m_footerHint);
    }
}

void QueriesWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) goBack();
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) runSelectedQuery();
    QWidget::keyPressEvent(event);
}

void QueriesWindow::loadQueries()
{
    m_allQueries.clear();

    // 1. Поиск папки ресурсов (более агрессивный)
    QDir dirSearch(qApp->applicationDirPath());
    QString foundPath = "";

    // Ищем в текущей, на один, на два и на три уровня вверх (для разных типов сборок)
    for(int i=0; i<4; ++i) {
        QString check = dirSearch.absoluteFilePath("resources/queries");
        if (QDir(check).exists()) {
            foundPath = check;
            break;
        }
        dirSearch.cdUp();
    }

    if (foundPath.isEmpty()) {
        qDebug() << "CRITICAL: resources/queries folder not found!";
    } else {
        qDebug() << "Loading queries from:" << foundPath;
        QStringList labs = {"Lab5", "Lab6"};
        foreach (const QString &lab, labs) {
            QDir labDir(foundPath + "/" + lab);
            foreach(const QFileInfo &fi, labDir.entryInfoList({"*.sql"}, QDir::Files)) {
                QFile f(fi.absoluteFilePath());
                if(f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QString sql = QTextStream(&f).readAll();
                    QueryInfo qi;
                    qi.number = fi.completeBaseName();
                    qi.type = lab;
                    qi.sqlText = sql;

                    QStringList lines = sql.split('\n');
                    QString firstLine = lines.isEmpty() ? "" : lines.first().trimmed();
                    qi.description = firstLine.startsWith("--") ? firstLine.mid(2).trimmed() : "Запрос: " + fi.fileName();

                    m_allQueries.append(qi);
                    f.close();
                }
            }
        }
    }

    // 2. Добавляем встроенные, если таких еще нет в списке
    QList<QueryInfo> embedded = getEmbeddedQueries();
    foreach(const auto &eq, embedded) {
        bool exists = false;
        for(const auto &q : m_allQueries) if(q.number == eq.number) { exists = true; break; }
        if (!exists) m_allQueries.append(eq);
    }

    // Сортировка по номерам
    std::sort(m_allQueries.begin(), m_allQueries.end(), [](const QueryInfo &a, const QueryInfo &b) {
        QStringList ap = a.number.split('.');
        QStringList bp = b.number.split('.');
        if (ap[0] != bp[0]) return ap[0].toInt() < bp[0].toInt();
        if (ap.size() > 1 && bp.size() > 1) return ap[1].toInt() < bp[1].toInt();
        return ap.size() < bp.size();
    });

    refreshTable();
}

void QueriesWindow::refreshTable()
{
    m_table->setRowCount(m_allQueries.size());
    for (int i = 0; i < m_allQueries.size(); ++i) {
        const auto &q = m_allQueries[i];
        m_table->setItem(i, 0, new QTableWidgetItem(q.number));
        m_table->setItem(i, 1, new QTableWidgetItem(q.type));
        m_table->setItem(i, 2, new QTableWidgetItem(q.description));
        for(int c=0; c<3; ++c) m_table->item(i, c)->setForeground(QBrush(Qt::black));
    }
    if (m_table->rowCount() > 0) m_table->selectRow(0);
}

void QueriesWindow::runSelectedQuery()
{
    int r = m_table->currentRow();
    if (r < 0) return;

    QueryInfo target = m_allQueries[r];
    QStringList cols;
    QList<QList<QVariant>> rows;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    bool ok = false;

    if (m_dbManager->isHttpMode()) {
        QJsonArray data = m_dbManager->executeCustomQueryHttp(target.sqlText, &ok);
        if (ok) {
            if (!data.isEmpty()) {
                cols = data[0].toObject().keys();
                for (int i = 0; i < data.size(); ++i) {
                    QList<QVariant> row;
                    QJsonObject obj = data[i].toObject();
                    foreach (const QString &col, cols) row << obj[col].toVariant();
                    rows << row;
                }
            } else {
                // Если данные пустые, попробуем хотя бы показать пустую таблицу
                // (колонки в этом режиме без данных не получить без доп. запроса метаданных)
            }
        }
    } else {
        QSqlQuery q = m_dbManager->executeQuery(target.sqlText, &ok);
        if (ok) {
            for (int i = 0; i < q.record().count(); ++i) cols << q.record().fieldName(i);
            while (q.next()) {
                QList<QVariant> row;
                for (int i = 0; i < cols.size(); ++i) row << q.value(i);
                rows << row;
            }
        }
    }
    QApplication::restoreOverrideCursor();

    if (!ok) {
        QMessageBox::critical(this, "Ошибка", "Запрос не выполнен:\n" + m_dbManager->lastError());
        return;
    }

    // ВСЕГДА открываем окно, чтобы пользователь видел результат (даже пустой)
    QueryResultWindow *res = new QueryResultWindow(target.number + ": " + target.description, cols, rows, m_dbManager, this);
    res->show();
}

void QueriesWindow::goBack() { hide(); }

void QueriesWindow::setupStyles() {
    if (m_isClassicUI) {
        setStyleSheet("QWidget { background-color: #c0c0c0; color: black; }"
                      "QTableWidget { background-color: white; border: 2px inset gray; font-family: 'Consolas'; color: black; }"
                      "QHeaderView::section { background-color: #c0c0c0; border: 1px solid black; color: black; font-weight: bold; }");
    } else {
        setStyleSheet("QWidget { background-color: #f8f9fa; color: #2c3e50; }"
                      "QTableWidget { background-color: white; border: 1px solid #dee2e6; color: black; }"
                      "QPushButton { background-color: #3498db; color: white; border-radius: 4px; padding: 8px; font-weight: bold; }");
    }
}

void QueriesWindow::onFilterChanged() {}
