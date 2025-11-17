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
    : QWidget(parent)
    , m_title(title)
    , m_columnNames(columnNames)
    , m_rows(rows)
    , m_dbManager(dbManager)
{
    setWindowTitle(QString("Результаты запроса: %1").arg(title));
    setGeometry(200, 200, 800, 600);

    // Инициализация русских названий полей (те же, что в TableViewWindow)
    m_fieldDisplayNames["id_prizivnik"] = "ID призывника";
    m_fieldDisplayNames["fio"] = "ФИО";
    m_fieldDisplayNames["data_rozhdeniya"] = "Дата рождения";
    m_fieldDisplayNames["adres_prozhivaniya"] = "Адрес проживания";
    m_fieldDisplayNames["nomer_pasporta"] = "Номер паспорта";
    m_fieldDisplayNames["kategoria_godnosti"] = "Категория годности";
    m_fieldDisplayNames["age"] = "Возраст";
    // Добавить другие поля по необходимости

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
    
    QPushButton *backBtn = new QPushButton("Назад", this);
    backBtn->setMinimumHeight(40);
    connect(backBtn, &QPushButton::clicked, this, &QueryResultWindow::goBack);
    buttonLayout->addWidget(backBtn);
    
    layout->addLayout(buttonLayout);
}

void QueryResultWindow::setupStyles()
{
    setStyleSheet(
        "QWidget { background-color: #dbffff; }"
        "QTableWidget {"
        "    background-color: white;"
        "    border: 2px solid #FFB6C1;"
        "    border-radius: 5px;"
        "}"
        "QHeaderView::section {"
        "    background-color: #FFB6C1;"
        "    padding: 5px;"
        "    border: 1px solid #FF69B4;"
        "    font-weight: bold;"
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
    if (parentWidget()) {
        parentWidget()->raise();
        parentWidget()->activateWindow();
    }
    hide();
}

void QueryResultWindow::exportToCSV()
{
    // Получаем путь к директории для экспорта запросов в CSV
    QString queriesDir;
    if (m_dbManager) {
        BackupManager backupManager(m_dbManager);
        queriesDir = backupManager.getQueriesExportPath("csv");
    } else {
        QDir dir("exports/queries/csv");
        if (!dir.exists()) {
            dir.mkpath(".");
        }
        queriesDir = dir.absolutePath();
    }
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    QString defaultFileName = QString("query_%1.csv").arg(timestamp);
    QString defaultPath = QDir(queriesDir).absoluteFilePath(defaultFileName);
    
    QString fileName = QFileDialog::getSaveFileName(this,
        "Сохранить результаты в CSV",
        defaultPath,
        "CSV Files (*.csv);;All Files (*)");
    
    if (fileName.isEmpty()) {
        return;
    }
    
    // Добавляем расширение .csv если его нет
    if (!fileName.endsWith(".csv", Qt::CaseInsensitive)) {
        fileName += ".csv";
    }
    
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Ошибка", 
                            QString("Не удалось создать файл:\n%1").arg(file.errorString()));
        return;
    }
    
    QTextStream out(&file);
    out.setCodec("UTF-8");
    
    // Добавляем BOM для UTF-8 (необходимо для правильного отображения в Excel)
    out << "\xEF\xBB\xBF";
    
    // Записываем заголовки
    QStringList headers;
    foreach (const QString &col, m_columnNames) {
        headers << getDisplayName(col);
    }
    out << headers.join(",") << "\n";
    
    // Записываем данные
    foreach (const QList<QVariant> &row, m_rows) {
        QStringList values;
        for (int i = 0; i < m_columnNames.size() && i < row.size(); ++i) {
            QString value = row[i].isNull() ? "" : row[i].toString();
            // Экранируем кавычки и запятые для CSV
            if (value.contains(",") || value.contains("\"") || value.contains("\n")) {
                value.replace("\"", "\"\""); // Экранируем двойные кавычки
                value = "\"" + value + "\""; // Оборачиваем в кавычки
            }
            values << value;
        }
        out << values.join(",") << "\n";
    }
    
    file.close();
    QMessageBox::information(this, "Успех", 
                            QString("Данные успешно экспортированы в файл:\n%1").arg(fileName));
}

void QueryResultWindow::exportToXlsx()
{
    // Получаем путь к директории для экспорта запросов в Excel
    QString queriesDir;
    if (m_dbManager) {
        BackupManager backupManager(m_dbManager);
        queriesDir = backupManager.getQueriesExportPath("xlsx");
    } else {
        QDir dir("exports/queries/xlsx");
        if (!dir.exists()) {
            dir.mkpath(".");
        }
        queriesDir = dir.absolutePath();
    }
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    QString defaultFileName = QString("query_%1.xlsx").arg(timestamp);
    QString defaultPath = QDir(queriesDir).absoluteFilePath(defaultFileName);
    
    QString fileName = QFileDialog::getSaveFileName(this,
        "Сохранить результаты в Excel",
        defaultPath,
        "Excel Files (*.xlsx);;All Files (*)");
    
    if (fileName.isEmpty()) {
        return;
    }
    
    // Добавляем расширение .xlsx если его нет
    if (!fileName.endsWith(".xlsx", Qt::CaseInsensitive)) {
        fileName += ".xlsx";
    }
    
    // Создаем Excel документ
    Document xlsx;
    
    // Записываем заголовки
    for (int col = 0; col < m_columnNames.size(); ++col) {
        QString header = getDisplayName(m_columnNames[col]);
        xlsx.write(1, col + 1, header);
    }
    
    // Форматируем заголовки
    Format headerFormat;
    headerFormat.setFontBold(true);
    headerFormat.setFillPattern(Format::PatternSolid);
    headerFormat.setPatternBackgroundColor(QColor(200, 200, 200));
    for (int col = 1; col <= m_columnNames.size(); ++col) {
        xlsx.write(1, col, xlsx.read(1, col), headerFormat);
    }
    
    // Записываем данные
    for (int row = 0; row < m_rows.size(); ++row) {
        for (int col = 0; col < m_columnNames.size() && col < m_rows[row].size(); ++col) {
            QVariant value = m_rows[row][col];
            if (!value.isNull()) {
                xlsx.write(row + 2, col + 1, value);
            }
        }
    }
    
    // Автоматически подгоняем ширину колонок
    for (int col = 1; col <= m_columnNames.size(); ++col) {
        xlsx.setColumnWidth(col, 15);
    }
    
    // Сохраняем файл
    if (xlsx.saveAs(fileName)) {
        QMessageBox::information(this, "Успех", 
                                QString("Данные успешно экспортированы в Excel:\n%1").arg(fileName));
    } else {
        QMessageBox::critical(this, "Ошибка", 
                            QString("Не удалось сохранить файл:\n%1").arg(fileName));
    }
}

