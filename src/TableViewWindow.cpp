#include "TableViewWindow.h"
#include "RecordDialog.h"
#include "BackupManager.h"
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
#include "xlsxdocument.h"
#include "xlsxformat.h"
using namespace QXlsx;

TableViewWindow::TableViewWindow(DatabaseManager *dbManager, const QString &tableName, QWidget *parent)
    : QWidget(parent)
    , m_dbManager(dbManager)
    , m_tableName(tableName)
{
    setWindowTitle(m_tableName);
    setGeometry(100, 100, 800, 600);
    
    // Инициализация русских названий полей
    m_fieldDisplayNames["id_prizivnik"] = "ID призывника";
    m_fieldDisplayNames["fio"] = "ФИО";
    m_fieldDisplayNames["data_rozhdeniya"] = "Дата рождения";
    m_fieldDisplayNames["adres_prozhivaniya"] = "Адрес проживания";
    m_fieldDisplayNames["nomer_pasporta"] = "Номер паспорта";
    m_fieldDisplayNames["id_comissar"] = "ID комиссара";
    m_fieldDisplayNames["dolzhnost"] = "Должность";
    m_fieldDisplayNames["stazh_raboty"] = "Стаж работы";
    m_fieldDisplayNames["kontaktnyi_telefon"] = "Контактный телефон";
    m_fieldDisplayNames["id_kategorii"] = "ID категории";
    m_fieldDisplayNames["nazvanie_kategorii"] = "Название категории";
    m_fieldDisplayNames["opisanie_ogranichenii"] = "Описание ограничений";
    m_fieldDisplayNames["index_kategorii"] = "Индекс категории";
    m_fieldDisplayNames["osnovanie_dlya_kategorii"] = "Основание для категории";
    m_fieldDisplayNames["id_osvidetelstvovania"] = "ID освидетельствования";
    m_fieldDisplayNames["data_provedeniya"] = "Дата проведения";
    m_fieldDisplayNames["rezultaty_obsledovania"] = "Результаты обследования";
    m_fieldDisplayNames["fio_vracha"] = "ФИО врача";
    m_fieldDisplayNames["zaklyuchenie"] = "Заключение";
    m_fieldDisplayNames["id_prizivnika"] = "ID призывника";
    m_fieldDisplayNames["id_bileta"] = "ID билета";
    m_fieldDisplayNames["nomer_bileta"] = "Номер билета";
    m_fieldDisplayNames["data_vydachi"] = "Дата выдачи";
    m_fieldDisplayNames["voinskoe_zvanie"] = "Воинское звание";
    m_fieldDisplayNames["kategoria"] = "Категория";
    m_fieldDisplayNames["id_karty"] = "ID карты";
    m_fieldDisplayNames["nomer_karty"] = "Номер карты";
    m_fieldDisplayNames["data_postanovki_na_uchet"] = "Дата постановки на учёт";
    m_fieldDisplayNames["istoriya_otsrochek"] = "История отсрочек";
    m_fieldDisplayNames["voenno_uchetnaya_specialnost"] = "Военно-учётная специальность";
    m_fieldDisplayNames["id_meropriyatiya"] = "ID мероприятия";
    m_fieldDisplayNames["tip_meropriyatiya"] = "Тип мероприятия";
    m_fieldDisplayNames["data_provedeniya"] = "Дата проведения";
    m_fieldDisplayNames["mesto_provedeniya"] = "Место проведения";
    m_fieldDisplayNames["fio_comissara"] = "ФИО комиссара";
    m_fieldDisplayNames["data_vzaimodeistviya"] = "Дата взаимодействия";
    m_fieldDisplayNames["nomer_kabineta"] = "Номер кабинета";
    
    setupUI();
    setupStyles();
    loadData();
}

TableViewWindow::~TableViewWindow()
{
}

void TableViewWindow::setupUI()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setSpacing(10);
    m_layout->setContentsMargins(20, 20, 20, 20);

    // Заголовок
    QLabel *title = new QLabel(m_tableName, this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 18px; color: #333; font-weight: bold;");
    m_layout->addWidget(title);

    // Таблица
    m_table = new QTableWidget(this);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_table, &QTableWidget::customContextMenuRequested, this, &TableViewWindow::showContextMenu);
    m_layout->addWidget(m_table);

    // Кнопки
    QHBoxLayout *btnLayout = new QHBoxLayout();
    
    QPushButton *addBtn = new QPushButton("Добавить", this);
    addBtn->setMinimumHeight(40);
    connect(addBtn, &QPushButton::clicked, this, &TableViewWindow::addRecord);
    btnLayout->addWidget(addBtn);
    
    QPushButton *exportCsvBtn = new QPushButton("Экспорт в CSV", this);
    exportCsvBtn->setMinimumHeight(40);
    connect(exportCsvBtn, &QPushButton::clicked, this, &TableViewWindow::exportToCSV);
    btnLayout->addWidget(exportCsvBtn);
    
    QPushButton *exportXlsxBtn = new QPushButton("Экспорт в Excel", this);
    exportXlsxBtn->setMinimumHeight(40);
    connect(exportXlsxBtn, &QPushButton::clicked, this, &TableViewWindow::exportToXlsx);
    btnLayout->addWidget(exportXlsxBtn);
    
    QPushButton *backBtn = new QPushButton("Назад", this);
    backBtn->setMinimumHeight(40);
    connect(backBtn, &QPushButton::clicked, this, &TableViewWindow::goBack);
    btnLayout->addWidget(backBtn);
    
    m_layout->addLayout(btnLayout);
}

void TableViewWindow::setupStyles()
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
        "    background-color: #5cffda;"
        "    font-size: 16px;"
        "    padding: 10px;"
        "    border-radius: 8px;"
        "    color: black;"
        "    border: none;"
        "    min-height: 40px;"
        "}"
        "QPushButton:hover { background-color: #00fac1; }"
        "QPushButton:pressed { background-color: #00c79a; }"
    );
}

void TableViewWindow::loadData()
{
    if (!m_dbManager || !m_dbManager->isConnected()) {
        return;
    }

    bool ok;
    QSqlQuery query = m_dbManager->executeQuery(QString("SELECT * FROM %1").arg(m_tableName), &ok);
    
    if (!ok) {
        QMessageBox::critical(this, "Ошибка", "Не удалось загрузить данные:\n" + m_dbManager->lastError());
        return;
    }

    // Получаем названия столбцов
    QStringList columns = m_dbManager->getColumnList(m_tableName);
    m_table->setColumnCount(columns.size());
    
    QStringList displayHeaders;
    foreach (const QString &col, columns) {
        displayHeaders << getDisplayName(col);
    }
    m_table->setHorizontalHeaderLabels(displayHeaders);

    // Получаем данные
    QList<QList<QVariant>> rows;
    while (query.next()) {
        QList<QVariant> row;
        for (int i = 0; i < columns.size(); ++i) {
            row << query.value(i);
        }
        rows << row;
    }

    m_table->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); ++i) {
        for (int j = 0; j < columns.size(); ++j) {
            QTableWidgetItem *item = new QTableWidgetItem(
                rows[i][j].isNull() ? "" : rows[i][j].toString());
            m_table->setItem(i, j, item);
        }
    }

    m_table->resizeColumnsToContents();
}

QString TableViewWindow::getDisplayName(const QString &fieldName) const
{
    return m_fieldDisplayNames.value(fieldName, fieldName);
}

void TableViewWindow::addRecord()
{
    RecordDialog dialog(m_dbManager, m_tableName, this);
    if (dialog.exec() == QDialog::Accepted) {
        loadData();
    }
}

void TableViewWindow::editRecord()
{
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите запись для редактирования");
        return;
    }

    QStringList columns = m_dbManager->getColumnList(m_tableName);
    if (columns.isEmpty()) {
        return;
    }

    // Для составных ключей используем специальный идентификатор
    // В RecordDialog будет использоваться первый первичный ключ для идентификации записи
    QStringList primaryKeys = m_dbManager->getPrimaryKeys(m_tableName);
    if (primaryKeys.isEmpty()) {
        QMessageBox::critical(this, "Ошибка", "Не удалось определить первичный ключ таблицы");
        return;
    }
    
    // Находим индекс первой колонки первичного ключа
    QString firstPkColumn = primaryKeys.first();
    int pkColumnIndex = columns.indexOf(firstPkColumn);
    if (pkColumnIndex < 0) {
        QMessageBox::critical(this, "Ошибка", "Колонка первичного ключа не найдена");
        return;
    }
    
    QTableWidgetItem *idItem = m_table->item(row, pkColumnIndex);
    if (!idItem) {
        return;
    }

    // Для составных ключей передаем -1, чтобы RecordDialog загрузил запись по всем ключам
    int recordId = -1;
    if (primaryKeys.size() == 1) {
        // Одиночный ключ - используем его значение
        bool ok;
        recordId = idItem->text().toInt(&ok);
        if (!ok) {
            recordId = -1;
        }
    }
    
    RecordDialog dialog(m_dbManager, m_tableName, this, recordId);
    if (dialog.exec() == QDialog::Accepted) {
        loadData();
    }
}

void TableViewWindow::deleteRecord()
{
    // Проверяем подключение к БД
    if (!m_dbManager || !m_dbManager->isConnected()) {
        QMessageBox::critical(this, "Ошибка", "База данных не подключена");
        return;
    }

    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите запись для удаления");
        return;
    }

    // Получаем первичные ключи (может быть составной ключ)
    QStringList primaryKeys = m_dbManager->getPrimaryKeys(m_tableName);
    if (primaryKeys.isEmpty()) {
        QMessageBox::critical(this, "Ошибка", "Не удалось определить первичный ключ таблицы");
        return;
    }

    // Находим индексы колонок с первичными ключами
    QStringList columns = m_dbManager->getColumnList(m_tableName);
    QStringList whereClauses;
    
    bool allFound = true;
    foreach (const QString &pkColumn, primaryKeys) {
        int pkColumnIndex = columns.indexOf(pkColumn);
        if (pkColumnIndex < 0) {
            QMessageBox::critical(this, "Ошибка", QString("Колонка первичного ключа '%1' не найдена в таблице").arg(pkColumn));
            allFound = false;
            break;
        }
        
        QTableWidgetItem *pkItem = m_table->item(row, pkColumnIndex);
        if (!pkItem) {
            QMessageBox::warning(this, "Ошибка", QString("Не удалось получить значение первичного ключа '%1'").arg(pkColumn));
            allFound = false;
            break;
        }
        
        whereClauses << QString("%1 = :%2").arg(pkColumn).arg(pkColumn);
    }
    
    if (!allFound) {
        return;
    }

    int ret = QMessageBox::question(this, "Удаление",
        "Вы уверены, что хотите удалить эту запись?",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        // Начинаем транзакцию
        if (!m_dbManager->beginTransaction()) {
            QMessageBox::critical(this, "Ошибка", "Не удалось начать транзакцию");
            return;
        }

        // Формируем WHERE clause для составного ключа
        QString sql = QString("DELETE FROM %1 WHERE %2").arg(m_tableName).arg(whereClauses.join(" AND "));
        QSqlQuery query = m_dbManager->prepareQuery(sql);
        
        // Привязываем значения
        foreach (const QString &pkColumn, primaryKeys) {
            int pkColumnIndex = columns.indexOf(pkColumn);
            QTableWidgetItem *pkItem = m_table->item(row, pkColumnIndex);
            QString pkValue = pkItem->text();
            query.bindValue(QString(":%1").arg(pkColumn), pkValue);
        }

        bool success = m_dbManager->executePreparedQuery(query);

        if (success) {
            if (m_dbManager->commitTransaction()) {
                QMessageBox::information(this, "Успех", "Запись удалена");
                loadData();
            } else {
                QMessageBox::critical(this, "Ошибка", "Не удалось зафиксировать транзакцию:\n" + m_dbManager->lastError());
                m_dbManager->rollbackTransaction();
            }
        } else {
            QMessageBox::critical(this, "Ошибка", "Не удалось удалить запись:\n" + m_dbManager->lastError());
            m_dbManager->rollbackTransaction();
        }
    }
}

void TableViewWindow::showContextMenu(const QPoint &pos)
{
    if (m_table->currentRow() < 0) {
        return;
    }

    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background-color: white; border: 1px solid #ccc; }"
        "QMenu::item { padding: 5px 25px 5px 20px; color: black; }"
        "QMenu::item:selected { background-color: #E0B0FF; color: white; }"
    );

    QAction *editAction = menu.addAction("Изменить");
    QAction *deleteAction = menu.addAction("Удалить");

    QAction *selectedAction = menu.exec(m_table->viewport()->mapToGlobal(pos));

    if (selectedAction == editAction) {
        editRecord();
    } else if (selectedAction == deleteAction) {
        deleteRecord();
    }
}

void TableViewWindow::goBack()
{
    if (parentWidget()) {
        parentWidget()->raise();
        parentWidget()->activateWindow();
    }
    hide();
}

void TableViewWindow::exportToCSV()
{
    if (!m_table || m_table->rowCount() == 0) {
        QMessageBox::information(this, "Информация", "Нет данных для экспорта");
        return;
    }
    
    // Получаем путь к директории для экспорта таблиц в CSV
    BackupManager backupManager(m_dbManager);
    QString tablesDir = backupManager.getTablesExportPath("csv");
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    QString defaultFileName = QString("table_%1_%2.csv").arg(m_tableName).arg(timestamp);
    QString defaultPath = QDir(tablesDir).absoluteFilePath(defaultFileName);
    
    QString fileName = QFileDialog::getSaveFileName(this,
        QString("Экспорт таблицы %1 в CSV").arg(m_tableName),
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
    for (int col = 0; col < m_table->columnCount(); ++col) {
        QString header = m_table->horizontalHeaderItem(col) ? 
                        m_table->horizontalHeaderItem(col)->text() : 
                        QString("Column %1").arg(col + 1);
        headers << header;
    }
    out << headers.join(",") << "\n";
    
    // Записываем данные
    for (int row = 0; row < m_table->rowCount(); ++row) {
        QStringList values;
        for (int col = 0; col < m_table->columnCount(); ++col) {
            QTableWidgetItem *item = m_table->item(row, col);
            QString value = item ? item->text() : "";
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
                            QString("Таблица успешно экспортирована в файл:\n%1").arg(fileName));
}

void TableViewWindow::exportToXlsx()
{
    if (!m_table || m_table->rowCount() == 0) {
        QMessageBox::information(this, "Информация", "Нет данных для экспорта");
        return;
    }
    
    // Получаем путь к директории для экспорта таблиц в Excel
    BackupManager backupManager(m_dbManager);
    QString tablesDir = backupManager.getTablesExportPath("xlsx");
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    QString defaultFileName = QString("table_%1_%2.xlsx").arg(m_tableName).arg(timestamp);
    QString defaultPath = QDir(tablesDir).absoluteFilePath(defaultFileName);
    
    QString fileName = QFileDialog::getSaveFileName(this,
        QString("Экспорт таблицы %1 в Excel").arg(m_tableName),
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
    for (int col = 0; col < m_table->columnCount(); ++col) {
        QString header = m_table->horizontalHeaderItem(col) ? 
                        m_table->horizontalHeaderItem(col)->text() : 
                        QString("Column %1").arg(col + 1);
        xlsx.write(1, col + 1, header);
    }
    
    // Форматируем заголовки
    Format headerFormat;
    headerFormat.setFontBold(true);
    headerFormat.setFillPattern(Format::PatternSolid);
    headerFormat.setPatternBackgroundColor(QColor(200, 200, 200));
    for (int col = 1; col <= m_table->columnCount(); ++col) {
        xlsx.write(1, col, xlsx.read(1, col), headerFormat);
    }
    
    // Записываем данные
    for (int row = 0; row < m_table->rowCount(); ++row) {
        for (int col = 0; col < m_table->columnCount(); ++col) {
            QTableWidgetItem *item = m_table->item(row, col);
            if (item) {
                QVariant value = item->text();
                xlsx.write(row + 2, col + 1, value);
            }
        }
    }
    
    // Автоматически подгоняем ширину колонок
    for (int col = 1; col <= m_table->columnCount(); ++col) {
        xlsx.setColumnWidth(col, 15);
    }
    
    // Сохраняем файл
    if (xlsx.saveAs(fileName)) {
        QMessageBox::information(this, "Успех", 
                                QString("Таблица успешно экспортирована в Excel:\n%1").arg(fileName));
    } else {
        QMessageBox::critical(this, "Ошибка", 
                            QString("Не удалось сохранить файл:\n%1").arg(fileName));
    }
}

