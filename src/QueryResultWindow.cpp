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
    setWindowTitle(QString("Результат: %1").arg(title));
    setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);
    resize(1000, 700);

    // Расширенная локализация полей
    m_fieldDisplayNames["conscript_id"] = "ID Призывника";
    m_fieldDisplayNames["full_name"] = "ФИО";
    m_fieldDisplayNames["birth_date"] = "Дата рождения";
    m_fieldDisplayNames["residence_address"] = "Адрес";
    m_fieldDisplayNames["passport_number"] = "Паспорт";
    m_fieldDisplayNames["category_name"] = "Категория годности";
    m_fieldDisplayNames["age"] = "Возраст";
    m_fieldDisplayNames["ticket_number"] = "№ Военного билета";
    m_fieldDisplayNames["military_rank"] = "Звание";
    m_fieldDisplayNames["count"] = "Количество";
    m_fieldDisplayNames["examination_date"] = "Дата осмотра";
    m_fieldDisplayNames["med_count"] = "Кол-во медосмотров";
    m_fieldDisplayNames["avg_age"] = "Средний возраст";

    setupUI();
    setupStyles();
}

QueryResultWindow::~QueryResultWindow() {}

void QueryResultWindow::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(10);
    layout->setContentsMargins(15, 15, 15, 15);

    QLabel *titleLabel = new QLabel(m_title, this);
    titleLabel->setAlignment(Qt::AlignLeft);
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #2c3e50; margin-bottom: 5px;");
    layout->addWidget(titleLabel);

    m_table = new QTableWidget(this);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setRowCount(m_rows.size());
    m_table->setColumnCount(m_columnNames.size());
    m_table->setAlternatingRowColors(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    QStringList displayHeaders;
    foreach (const QString &col, m_columnNames) displayHeaders << getDisplayName(col);
    m_table->setHorizontalHeaderLabels(displayHeaders);

    for (int i = 0; i < m_rows.size(); ++i) {
        for (int j = 0; j < m_columnNames.size() && j < m_rows[i].size(); ++j) {
            QString val = m_rows[i][j].isNull() ? "—" : m_rows[i][j].toString();
            if (m_rows[i][j].type() == QVariant::Date) val = m_rows[i][j].toDate().toString("dd.MM.yyyy");

            QTableWidgetItem *item = new QTableWidgetItem(val);
            item->setTextAlignment(Qt::AlignCenter);
            item->setForeground(QBrush(QColor("#212529")));
            m_table->setItem(i, j, item);
        }
    }

    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->resizeColumnsToContents();
    layout->addWidget(m_table);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    QPushButton *exportCsvBtn = new QPushButton("CSV", this);
    QPushButton *exportXlsxBtn = new QPushButton("Excel", this);
    QPushButton *backBtn = new QPushButton("Закрыть", this);
    
    exportCsvBtn->setFixedWidth(120);
    exportXlsxBtn->setFixedWidth(120);
    backBtn->setFixedWidth(120);

    connect(exportCsvBtn, &QPushButton::clicked, this, &QueryResultWindow::exportToCSV);
    connect(exportXlsxBtn, &QPushButton::clicked, this, &QueryResultWindow::exportToXlsx);
    connect(backBtn, &QPushButton::clicked, this, &QueryResultWindow::goBack);

    buttonLayout->addStretch();
    buttonLayout->addWidget(exportCsvBtn);
    buttonLayout->addWidget(exportXlsxBtn);
    buttonLayout->addWidget(backBtn);
    
    layout->addLayout(buttonLayout);
}

void QueryResultWindow::setupStyles()
{
    // Профессиональный современный стиль (без ядовито-голубого фона)
    setStyleSheet(
        "QDialog { background-color: #f8f9fa; }"
        "QTableWidget {"
        "    background-color: white;"
        "    border: 1px solid #dee2e6;"
        "    gridline-color: #e9ecef;"
        "    selection-background-color: #3498db;"
        "    selection-color: white;"
        "    border-radius: 4px;"
        "}"
        "QHeaderView::section {"
        "    background-color: #e9ecef;"
        "    padding: 8px;"
        "    border: none;"
        "    border-right: 1px solid #dee2e6;"
        "    border-bottom: 2px solid #dee2e6;"
        "    font-weight: bold;"
        "    color: #495057;"
        "}"
        "QPushButton {"
        "    background-color: #2c3e50;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 4px;"
        "    padding: 8px 15px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #34495e; }"
        "QPushButton:pressed { background-color: #1a252f; }"
    );
}

QString QueryResultWindow::getDisplayName(const QString &fieldName) const
{
    return m_fieldDisplayNames.value(fieldName, fieldName);
}

void QueryResultWindow::goBack() { accept(); }

void QueryResultWindow::exportToCSV()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Экспорт CSV", "", "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;
    
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Ошибка", file.errorString());
        return;
    }
    
    QTextStream out(&file);
    out.setGenerateByteOrderMark(true);
    
    QStringList headers;
    foreach (const QString &col, m_columnNames) headers << getDisplayName(col);
    out << headers.join(";") << "\n";
    
    foreach (const QList<QVariant> &row, m_rows) {
        QStringList values;
        for (int i = 0; i < m_columnNames.size() && i < row.size(); ++i) {
            QString val = row[i].isNull() ? "" : row[i].toString();
            val.replace(";", ",");
            values << val;
        }
        out << values.join(";") << "\n";
    }
    file.close();
    QMessageBox::information(this, "Успех", "Файл успешно сохранен");
}

void QueryResultWindow::exportToXlsx()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Экспорт Excel", "", "Excel Files (*.xlsx)");
    if (fileName.isEmpty()) return;
    
    Document xlsx;
    Format headerFormat;
    headerFormat.setFontBold(true);
    headerFormat.setPatternBackgroundColor(QColor("#e9ecef"));
    headerFormat.setHorizontalAlignment(Format::AlignHCenter);

    for (int col = 0; col < m_columnNames.size(); ++col) {
        xlsx.write(1, col + 1, getDisplayName(m_columnNames[col]), headerFormat);
    }
    
    for (int row = 0; row < m_rows.size(); ++row) {
        for (int col = 0; col < m_columnNames.size() && col < m_rows[row].size(); ++col) {
            xlsx.write(row + 2, col + 1, m_rows[row][col]);
        }
    }
    
    if (xlsx.saveAs(fileName)) QMessageBox::information(this, "Успех", "Файл успешно сохранен");
    else QMessageBox::critical(this, "Ошибка", "Не удалось сохранить файл");
}
