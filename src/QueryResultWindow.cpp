#include "QueryResultWindow.h"
#include <QLabel>
#include <QHeaderView>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QFile>

QueryResultWindow::QueryResultWindow(const QString &title,
                                     const QStringList &columnNames,
                                     const QList<QList<QVariant>> &rows,
                                     QWidget *parent)
    : QWidget(parent)
    , m_title(title)
    , m_columnNames(columnNames)
    , m_rows(rows)
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
    
    QPushButton *exportBtn = new QPushButton("Экспорт в CSV", this);
    exportBtn->setMinimumHeight(40);
    connect(exportBtn, &QPushButton::clicked, this, &QueryResultWindow::exportToCSV);
    buttonLayout->addWidget(exportBtn);
    
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
    close();
}

