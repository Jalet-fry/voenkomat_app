#include "QueryResultWindow.h"
#include "BackupManager.h"
#include "DatabaseManager.h"
#include <QLabel>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include "xlsxdocument.h"
#include "xlsxformat.h"
using namespace QXlsx;

QueryResultWindow::QueryResultWindow(const QString &title,
                                     const QStringList &columnNames,
                                     const QList<QList<QVariant>> &rows,
                                     DatabaseManager *dbManager,
                                     QWidget *parent)
    : QDialog(parent)
    , m_title(title)
    , m_columnNames(columnNames)
    , m_rows(rows)
    , m_dbManager(dbManager)
{
    setWindowTitle(QString("Результаты запроса: %1").arg(title));
    setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);
    resize(900, 600);

    // Инициализация русских названий полей (те же, что в TableViewWindow)
    m_fieldDisplayNames["id_prizivnik"] = "ID призывника";
    m_fieldDisplayNames["fio"] = "ФИО";
    m_fieldDisplayNames["data_rozhdeniya"] = "Дата рождения";
    m_fieldDisplayNames["adres_prozhivaniya"] = "Адрес проживания";
    m_fieldDisplayNames["nomer_pasporta"] = "Номер паспорта";
    m_fieldDisplayNames["kategoria_godnosti"] = "Категория годности";
    m_fieldDisplayNames["age"] = "Возраст";

    setupUI();
    setupStyles();
}

QueryResultWindow::~QueryResultWindow()
{
}

void QueryResultWindow::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(15);
    layout->setContentsMargins(20, 20, 20, 20);

    // Заголовок
    QLabel *titleLabel = new QLabel(m_title, this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #333;");
    layout->addWidget(titleLabel);

    // Таблица
    m_table = new QTableWidget(this);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setRowCount(m_rows.size());
    m_table->setColumnCount(m_columnNames.size());

    QStringList displayHeaders;
    foreach (const QString &col, m_columnNames) {
        displayHeaders << getDisplayName(col);
    }
    m_table->setHorizontalHeaderLabels(displayHeaders);

    // Заполнение данными
    for (int i = 0; i < m_rows.size(); ++i) {
        for (int j = 0; j < m_columnNames.size() && j < m_rows[i].size(); ++j) {
            QTableWidgetItem *item = new QTableWidgetItem(
                m_rows[i][j].isNull() ? "—" : m_rows[i][j].toString());
            item->setTextAlignment(Qt::AlignCenter);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            item->setTextColor(Qt::black);
#else
            item->setForeground(QBrush(Qt::black));
#endif
            m_table->setItem(i, j, item);
        }
    }

    m_table->resizeColumnsToContents();
    layout->addWidget(m_table);

    // Кнопки
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    QPushButton *exportCsvBtn = new QPushButton("Экспорт в CSV", this);
    exportCsvBtn->setMinimumHeight(40);
    connect(exportCsvBtn, &QPushButton::clicked, this, &QueryResultWindow::exportToCSV);
    buttonLayout->addWidget(exportCsvBtn);
    
    QPushButton *exportXlsxBtn = new QPushButton("Экспорт в Excel", this);
    exportXlsxBtn->setMinimumHeight(40);
    connect(exportXlsxBtn, &QPushButton::clicked, this, &QueryResultWindow::exportToXlsx);
    buttonLayout->addWidget(exportXlsxBtn);
    
    QPushButton *backBtn = new QPushButton("Закрыть", this);
    backBtn->setMinimumHeight(40);
    connect(backBtn, &QPushButton::clicked, this, &QueryResultWindow::goBack);
    buttonLayout->addWidget(backBtn);
    
    layout->addLayout(buttonLayout);
}

void QueryResultWindow::setupStyles()
{
    setStyleSheet(
        "QDialog { background-color: #dbffff; }"
        "QTableWidget {"
        "    background-color: white;"
        "    border: 2px solid #FFB6C1;"
        "    border-radius: 5px;"
        "    color: black;"
        "}"
        "QTableWidget::item { color: black; }"
        "QHeaderView::section {"
        "    background-color: #FFB6C1;"
        "    padding: 5px;"
        "    border: 1px solid #FF69B4;"
        "    font-weight: bold;"
        "    color: black;"
        "}"
        "QPushButton {"
        "    background-color: #E0B0FF;"
        "    font-size: 16px;"
        "    padding: 10px;"
        "    border-radius: 8px;"
        "    color: black;"
        "    border: none;"
        "    min-height: 40px;"
        "}"
        "QPushButton:hover { background-color: #c770ff; }"
        "QPushButton:pressed { background-color: #a314ff; }"
    );
}

QString QueryResultWindow::getDisplayName(const QString &fieldName) const
{
    return m_fieldDisplayNames.value(fieldName, fieldName);
}

void QueryResultWindow::goBack()
{
    accept();
}

void QueryResultWindow::exportToCSV()
{
    QString queriesDir;
    if (m_dbManager) {
        BackupManager backupManager(m_dbManager);
        queriesDir = backupManager.getQueriesExportPath("csv");
    } else {
        QDir dir("exports/queries/csv");
        if (!dir.exists()) dir.mkpath(".");
        queriesDir = dir.absolutePath();
    }
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    QString defaultPath = QDir(queriesDir).absoluteFilePath(QString("query_%1.csv").arg(timestamp));
    
    QString fileName = QFileDialog::getSaveFileName(this, "Сохранить в CSV", defaultPath, "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;
    
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Ошибка", file.errorString());
        return;
    }
    
    QTextStream out(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    out.setCodec("UTF-8");
#endif
    out << "\xEF\xBB\xBF"; // BOM
    
    QStringList headers;
    foreach (const QString &col, m_columnNames) headers << getDisplayName(col);
    out << headers.join(",") << "\n";
    
    foreach (const QList<QVariant> &row, m_rows) {
        QStringList values;
        for (int i = 0; i < m_columnNames.size() && i < row.size(); ++i) {
            QString val = row[i].isNull() ? "" : row[i].toString();
            if (val.contains(",") || val.contains("\"") || val.contains("\n")) {
                val.replace("\"", "\"\"");
                val = "\"" + val + "\"";
            }
            values << val;
        }
        out << values.join(",") << "\n";
    }
    file.close();
    QMessageBox::information(this, "Успех", "Файл сохранен");
}

void QueryResultWindow::exportToXlsx()
{
    QString queriesDir;
    if (m_dbManager) {
        BackupManager backupManager(m_dbManager);
        queriesDir = backupManager.getQueriesExportPath("xlsx");
    } else {
        QDir dir("exports/queries/xlsx");
        if (!dir.exists()) dir.mkpath(".");
        queriesDir = dir.absolutePath();
    }
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    QString defaultPath = QDir(queriesDir).absoluteFilePath(QString("query_%1.xlsx").arg(timestamp));
    
    QString fileName = QFileDialog::getSaveFileName(this, "Сохранить в Excel", defaultPath, "Excel Files (*.xlsx)");
    if (fileName.isEmpty()) return;
    
    Document xlsx;
    for (int col = 0; col < m_columnNames.size(); ++col) {
        xlsx.write(1, col + 1, getDisplayName(m_columnNames[col]));
    }
    
    Format headerFormat;
    headerFormat.setFontBold(true);
    headerFormat.setPatternBackgroundColor(QColor(200, 200, 200));
    for (int col = 1; col <= m_columnNames.size(); ++col) {
        xlsx.write(1, col, xlsx.read(1, col), headerFormat);
    }
    
    for (int row = 0; row < m_rows.size(); ++row) {
        for (int col = 0; col < m_columnNames.size() && col < m_rows[row].size(); ++col) {
            xlsx.write(row + 2, col + 1, m_rows[row][col]);
        }
    }
    
    if (xlsx.saveAs(fileName)) QMessageBox::information(this, "Успех", "Файл сохранен");
    else QMessageBox::critical(this, "Ошибка", "Не удалось сохранить");
}
