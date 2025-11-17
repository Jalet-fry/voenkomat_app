#include "QueriesWindow.h"
#include "QueryResultWindow.h"
#include "BackupManager.h"
#include <QLabel>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QMenu>
#include <QFileDialog>
#include <QInputDialog>
#include <QTextEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QRegExp>
#include <QLineEdit>
#include <QApplication>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QDebug>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPushButton>
#include <algorithm>

QueriesWindow::QueriesWindow(DatabaseManager *dbManager, QWidget *parent)
    : QWidget(parent)
    , m_dbManager(dbManager)
    , m_layout(nullptr)
    , m_filterLayout(nullptr)
    , m_table(nullptr)
    , m_typeFilter(nullptr)
    , m_searchEdit(nullptr)
{
    setWindowTitle("Запросы");
    setGeometry(150, 150, 1000, 700);
    
    setupUI();
    setupStyles();
    loadQueries();
}

QueriesWindow::~QueriesWindow()
{
}

void QueriesWindow::setupUI()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setSpacing(15);
    m_layout->setContentsMargins(20, 20, 20, 20);

    // Заголовок
    QLabel *title = new QLabel("Выберите запрос", this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 20px; color: black; font-weight: bold;");
    m_layout->addWidget(title);

    // Панель фильтрации
    m_filterLayout = new QHBoxLayout();
    
    QLabel *filterLabel = new QLabel("Тип:", this);
    m_filterLayout->addWidget(filterLabel);
    
    m_typeFilter = new QComboBox(this);
    m_typeFilter->addItem("Все", "");
    m_typeFilter->addItem("Lab5", "Lab5");
    m_typeFilter->addItem("Lab6", "Lab6");
    m_typeFilter->setMinimumWidth(120);
    connect(m_typeFilter, SIGNAL(currentIndexChanged(int)), this, SLOT(onFilterChanged()));
    m_filterLayout->addWidget(m_typeFilter);
    
    m_filterLayout->addSpacing(20);
    
    QLabel *searchLabel = new QLabel("Поиск:", this);
    m_filterLayout->addWidget(searchLabel);
    
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Введите текст для поиска...");
    m_searchEdit->setMinimumWidth(200);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &QueriesWindow::onSearchTextChanged);
    m_filterLayout->addWidget(m_searchEdit);
    
    m_filterLayout->addStretch();
    
    m_layout->addLayout(m_filterLayout);

    // Таблица запросов
    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    QStringList headers;
    headers << "Номер" << "Описание" << "Тип" << "Действие";
    m_table->setHorizontalHeaderLabels(headers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    m_table->setAlternatingRowColors(true);
    
    // Настройка колонок
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->setColumnWidth(0, 80);   // Номер
    m_table->setColumnWidth(1, 400);  // Описание
    m_table->setColumnWidth(2, 100);  // Тип
    m_table->setColumnWidth(3, 120);  // Действие
    
    connect(m_table, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(onTableDoubleClicked(QModelIndex)));
    connect(m_table, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showQueryContextMenu(QPoint)));
    
    m_layout->addWidget(m_table);

    // Кнопки
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    QPushButton *addBtn = new QPushButton("Добавить", this);
    addBtn->setMinimumHeight(40);
    connect(addBtn, &QPushButton::clicked, this, &QueriesWindow::addNewQuery);
    buttonLayout->addWidget(addBtn);
    
    buttonLayout->addStretch();
    
    QPushButton *backBtn = new QPushButton("Назад", this);
    backBtn->setMinimumHeight(40);
    connect(backBtn, &QPushButton::clicked, this, &QueriesWindow::goBack);
    buttonLayout->addWidget(backBtn);
    
    m_layout->addLayout(buttonLayout);
}

void QueriesWindow::setupStyles()
{
    setStyleSheet(
        "QWidget { background-color: #dbffff; }"
        "QPushButton {"
        "    background-color: #FFB6C1;"
        "    font-size: 16px;"
        "    padding: 10px;"
        "    border-radius: 8px;"
        "    color: black;"
        "    border: none;"
        "    min-height: 40px;"
        "    min-width: 60px;"
        "}"
        "QPushButton:hover { background-color: #FF69B4; }"
        "QPushButton:pressed { background-color: #FF1493; }"
        "QTableWidget {"
        "    background-color: white;"
        "    border: 1px solid #ccc;"
        "    gridline-color: #ddd;"
        "}"
        "QTableWidget::item {"
        "    padding: 5px;"
        "}"
        "QTableWidget::item:selected {"
        "    background-color: #E0B0FF;"
        "}"
        "QComboBox, QLineEdit {"
        "    padding: 5px;"
        "    border: 1px solid #ccc;"
        "    border-radius: 4px;"
        "    background-color: white;"
        "}"
    );
}

QString QueriesWindow::findResourcesDirectory() const
{
    // Список возможных путей к ресурсам (в порядке приоритета)
    QStringList possiblePaths;
    
    // 1. Относительный путь от текущей рабочей директории (для разработки)
    QDir currentDir = QDir::current();
    possiblePaths << currentDir.absoluteFilePath("resources/queries");
    
    // 2. Путь в директории приложения (для релизных сборок)
    QString appDir = QApplication::applicationDirPath();
    possiblePaths << QDir(appDir).absoluteFilePath("resources/queries");
    
    // 3. Путь на уровень выше от директории приложения (если приложение в debug/release)
    QDir appDirParent = QDir(appDir);
    if (appDirParent.cdUp()) {
        possiblePaths << appDirParent.absoluteFilePath("resources/queries");
    }
    
    // 4. Проверяем родительские директории от текущей рабочей директории
    QDir parentDir = currentDir;
    for (int i = 0; i < 5; ++i) {
        QString parentPath = parentDir.absoluteFilePath("resources/queries");
        possiblePaths << parentPath;
        if (!parentDir.cdUp()) {
            break;
        }
    }
    
    // 5. Также проверяем относительно директории приложения
    QDir appDirSearch = QDir(appDir);
    for (int i = 0; i < 5; ++i) {
        QString searchPath = appDirSearch.absoluteFilePath("resources/queries");
        possiblePaths << searchPath;
        if (!appDirSearch.cdUp()) {
            break;
        }
    }
    
    // Ищем первый существующий путь
    foreach (const QString &path, possiblePaths) {
        QDir dir(path);
        if (dir.exists() && dir.isReadable()) {
            qDebug() << "Найдена директория ресурсов:" << dir.absolutePath();
            return dir.absolutePath();
        }
    }
    
    // Если ничего не найдено, возвращаем путь относительно текущей директории
    qDebug() << "Директория ресурсов не найдена, используется путь по умолчанию";
    return currentDir.absoluteFilePath("resources/queries");
}

void QueriesWindow::loadQueries()
{
    m_allQueries.clear();
    m_filteredQueries.clear();

    // Находим директорию с ресурсами
    QString resourcesPath = findResourcesDirectory();
    QDir queriesDir(resourcesPath);
    
    qDebug() << "=== Загрузка запросов ===";
    qDebug() << "Путь к ресурсам:" << resourcesPath;
    qDebug() << "Директория существует:" << queriesDir.exists();
    
    if (!queriesDir.exists()) {
        qDebug() << "ОШИБКА: Директория с запросами не найдена:" << resourcesPath;
        QMessageBox::warning(this, "Ошибка", 
            QString("Директория с запросами не найдена:\n%1\n\nПроверьте, что папка resources/queries существует.").arg(resourcesPath));
        refreshTable();
        return;
    }
    
    qDebug() << "Загрузка запросов из:" << queriesDir.absolutePath();

    // Загружаем запросы из Lab5
    QDir lab5Dir(queriesDir.absoluteFilePath("Lab5"));
    qDebug() << "Папка Lab5 существует:" << lab5Dir.exists();
    if (lab5Dir.exists()) {
        qDebug() << "Загрузка из Lab5:" << lab5Dir.absolutePath();
        int beforeCount = m_allQueries.size();
        loadQueriesFromFolder(lab5Dir.absolutePath(), "Lab5");
        qDebug() << "Загружено из Lab5:" << (m_allQueries.size() - beforeCount) << "запросов";
    } else {
        qDebug() << "Папка Lab5 не найдена:" << lab5Dir.absolutePath();
    }

    // Загружаем запросы из Lab6
    QDir lab6Dir(queriesDir.absoluteFilePath("Lab6"));
    qDebug() << "Папка Lab6 существует:" << lab6Dir.exists();
    if (lab6Dir.exists()) {
        qDebug() << "Загрузка из Lab6:" << lab6Dir.absolutePath();
        int beforeCount = m_allQueries.size();
        loadQueriesFromFolder(lab6Dir.absolutePath(), "Lab6");
        qDebug() << "Загружено из Lab6:" << (m_allQueries.size() - beforeCount) << "запросов";
    } else {
        qDebug() << "Папка Lab6 не найдена:" << lab6Dir.absolutePath();
    }

    qDebug() << "Всего загружено запросов:" << m_allQueries.size();

    if (m_allQueries.isEmpty()) {
        qDebug() << "ПРЕДУПРЕЖДЕНИЕ: Не загружено ни одного запроса!";
        QMessageBox::warning(this, "Предупреждение", 
            QString("Не найдено ни одного запроса в папках:\n%1/Lab5\n%1/Lab6\n\nПроверьте наличие файлов *.sql в этих папках.")
            .arg(queriesDir.absolutePath()));
    }

    // Сортируем по номеру
    std::sort(m_allQueries.begin(), m_allQueries.end(), [](const QueryInfo &a, const QueryInfo &b) {
        // Сравниваем сначала по типу, потом по номеру
        if (a.type != b.type) {
            return a.type < b.type;
        }
        // Парсим номер (например, "5.1" -> 5.1, "6.1" -> 6.1)
        QStringList aParts = a.number.split('.');
        QStringList bParts = b.number.split('.');
        if (aParts.size() >= 2 && bParts.size() >= 2) {
            int aLab = aParts[0].toInt();
            int aNum = aParts[1].toInt();
            int bLab = bParts[0].toInt();
            int bNum = bParts[1].toInt();
            if (aLab != bLab) return aLab < bLab;
            return aNum < bNum;
        }
        return a.number < b.number;
    });

    refreshTable();
}

void QueriesWindow::loadQueriesFromFolder(const QString &folderPath, const QString &type)
{
    QDir folderDir(folderPath);
    if (!folderDir.exists()) {
        qDebug() << "Папка не найдена:" << folderPath;
        return;
    }

    QStringList filters;
    filters << "*.sql";
    QFileInfoList files = folderDir.entryInfoList(filters, QDir::Files, QDir::Name);
    
    qDebug() << "Найдено файлов в" << type << ":" << files.size();

    foreach (const QFileInfo &fileInfo, files) {
        // Используем completeBaseName() чтобы получить имя файла с точками (например, "5.1" из "5.1.sql")
        QString fileName = fileInfo.completeBaseName(); // "5.1", "6.1", etc.
        qDebug() << "Обработка файла:" << fileName << "полное имя:" << fileInfo.fileName();
        
        // Проверяем формат имени файла (например, "5.1", "6.1")
        QRegExp rx("^(\\d+)\\.(\\d+)$");
        if (rx.exactMatch(fileName)) {
            QString number = fileName; // Сохраняем как "5.1", "6.1"
            qDebug() << "  Файл соответствует формату, номер:" << number;
            
            QFile file(fileInfo.absoluteFilePath());
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                in.setCodec("UTF-8");
                QString queryText = in.readAll();
                file.close();

                QueryInfo queryInfo;
                queryInfo.number = number;
                queryInfo.description = parseQueryDescription(queryText);
                queryInfo.type = type;
                queryInfo.filePath = fileInfo.absoluteFilePath();
                queryInfo.sqlText = queryText;

                m_allQueries.append(queryInfo);
                qDebug() << "  Запрос добавлен:" << number << "-" << queryInfo.description;
            } else {
                qDebug() << "  ОШИБКА: Не удалось открыть файл:" << fileInfo.absoluteFilePath();
            }
        } else {
            qDebug() << "  Файл не соответствует формату (ожидается N.N):" << fileName;
        }
    }
}

QString QueriesWindow::parseQueryDescription(const QString &queryText) const
{
    // Парсим описание из комментария, убирая номер запроса
    // Форматы: 
    // -- Запрос 5.1: Описание
    // -- Запрос 2.1.1: Описание
    // -- 2.1.10: Описание
    // -- Описание
    
    // Ищем первую строку с комментарием
    QStringList lines = queryText.split('\n');
    foreach (const QString &line, lines) {
        QString trimmedLine = line.trimmed();
        if (trimmedLine.startsWith("--")) {
            // Убираем "--" в начале
            QString content = trimmedLine.mid(2).trimmed();
            
            // Убираем "Запрос" если есть
            if (content.startsWith("Запрос", Qt::CaseInsensitive)) {
                content = content.mid(7).trimmed(); // "Запрос" = 7 символов
            }
            
            // Убираем номер запроса (формат: N.N или N.N.N или N.NN)
            // Паттерн: начинается с цифр, потом точка, потом цифры, потом может быть еще точка и цифры
            QRegExp numberRx("^\\d+\\.\\d+(?:\\.\\d+)?[::]?\\s*");
            content.remove(numberRx);
            
            // Убираем двоеточие в начале если осталось
            if (content.startsWith(":")) {
                content = content.mid(1).trimmed();
            }
            
            if (!content.isEmpty()) {
                return content;
            }
        }
    }
    
    return "Запрос без описания";
}

void QueriesWindow::refreshTable()
{
    // Применяем фильтры
    m_filteredQueries.clear();
    
    QString typeFilter = m_typeFilter->currentData().toString();
    QString searchText = m_searchEdit->text().toLower();
    
    foreach (const QueryInfo &query, m_allQueries) {
        // Фильтр по типу
        if (!typeFilter.isEmpty() && query.type != typeFilter) {
            continue;
        }
        
        // Фильтр по поисковому тексту
        if (!searchText.isEmpty()) {
            if (!query.description.toLower().contains(searchText) &&
                !query.number.contains(searchText) &&
                !query.type.toLower().contains(searchText)) {
                continue;
            }
        }
        
        m_filteredQueries.append(query);
    }
    
    populateTable();
}

void QueriesWindow::populateTable()
{
    m_table->setRowCount(0);
    m_table->setRowCount(m_filteredQueries.size());

    for (int i = 0; i < m_filteredQueries.size(); ++i) {
        const QueryInfo &query = m_filteredQueries[i];

        // Номер
        QTableWidgetItem *numberItem = new QTableWidgetItem(query.number);
        numberItem->setData(Qt::UserRole, i); // Сохраняем индекс для быстрого доступа
        m_table->setItem(i, 0, numberItem);

        // Описание
        QTableWidgetItem *descItem = new QTableWidgetItem(query.description);
        m_table->setItem(i, 1, descItem);

        // Тип
        QTableWidgetItem *typeItem = new QTableWidgetItem(query.type);
        m_table->setItem(i, 2, typeItem);

        // Кнопка "Выполнить"
        QPushButton *runBtn = new QPushButton("Выполнить", this);
        runBtn->setStyleSheet(
            "QPushButton {"
            "    background-color: #4CAF50;"
            "    color: white;"
            "    border: none;"
            "    padding: 5px 10px;"
            "    border-radius: 4px;"
            "    font-size: 12px;"
            "}"
            "QPushButton:hover { background-color: #45a049; }"
            "QPushButton:pressed { background-color: #3d8b40; }"
            "QPushButton:disabled {"
            "    background-color: #cccccc;"
            "    color: #666666;"
            "}"
        );
        connect(runBtn, &QPushButton::clicked, [this, query, runBtn]() {
            // Отключаем кнопку на время выполнения, чтобы предотвратить повторные нажатия
            runBtn->setEnabled(false);
            runBtn->setText("Выполняется...");
            
            // Обрабатываем события, чтобы UI обновился (кнопка стала неактивной)
            QApplication::processEvents();
            
            // Выполняем запрос
            runQuery(query);
            
            // Включаем кнопку обратно сразу после завершения
            runBtn->setEnabled(true);
            runBtn->setText("Выполнить");
        });
        m_table->setCellWidget(i, 3, runBtn);
    }

    // Автоматическое изменение размера колонки описания
    m_table->resizeColumnToContents(1);
}

void QueriesWindow::onTableDoubleClicked(const QModelIndex &index)
{
    int row = index.row();
    if (row >= 0 && row < m_filteredQueries.size()) {
        runQuery(m_filteredQueries[row]);
    }
}

void QueriesWindow::onFilterChanged()
{
    refreshTable();
}

void QueriesWindow::onSearchTextChanged(const QString &text)
{
    Q_UNUSED(text);
    refreshTable();
}

QueryInfo QueriesWindow::getQueryInfoFromRow(int row) const
{
    if (row >= 0 && row < m_filteredQueries.size()) {
        return m_filteredQueries[row];
    }
    return QueryInfo();
}

void QueriesWindow::runQuery(const QueryInfo &queryInfo)
{
    if (queryInfo.sqlText.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Запрос не найден");
        return;
    }

    if (!m_dbManager || !m_dbManager->isConnected()) {
        QMessageBox::critical(this, "Ошибка подключения", 
                             "База данных не подключена.\nПроверьте параметры подключения.");
        return;
    }

    bool ok;
    QSqlQuery query = m_dbManager->executeQuery(queryInfo.sqlText, &ok);

    if (!ok) {
        QString errorMsg = m_dbManager->lastError();
        if (errorMsg.isEmpty()) {
            errorMsg = "Неизвестная ошибка при выполнении запроса";
        }
        QMessageBox::critical(this, "Ошибка выполнения запроса", 
                             QString("Не удалось выполнить запрос:\n\n%1\n\nПроверьте синтаксис SQL запроса.")
                             .arg(errorMsg));
        return;
    }

    // Получаем результаты
    QStringList columnNames;
    if (query.isActive() && query.isSelect()) {
        QSqlRecord record = query.record();
        for (int i = 0; i < record.count(); ++i) {
            columnNames << record.fieldName(i);
        }
    } else {
        // Для не-SELECT запросов (INSERT, UPDATE, DELETE)
        QMessageBox::information(this, "Запрос выполнен", 
                                QString("Запрос успешно выполнен.\nЗатронуто строк: %1")
                                .arg(query.numRowsAffected()));
        return;
    }

    QList<QList<QVariant>> rows;
    while (query.next()) {
        QList<QVariant> row;
        for (int i = 0; i < columnNames.size(); ++i) {
            row << query.value(i);
        }
        rows << row;
    }

    if (rows.isEmpty() && columnNames.isEmpty()) {
        QMessageBox::information(this, "Нет результатов", 
                                "Запрос выполнен успешно, но не вернул данных.");
        return;
    }

    QString queryTitle = QString("%1: %2").arg(queryInfo.number).arg(queryInfo.description);
    QueryResultWindow *resultWindow = new QueryResultWindow(queryTitle, columnNames, rows, m_dbManager, this);
    resultWindow->setWindowFlags(Qt::Window);
    resultWindow->raise();
    resultWindow->activateWindow();
    resultWindow->show();
}

void QueriesWindow::addNewQuery()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Добавление SQL-запроса");
    dialog.setMinimumSize(500, 350);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QLabel *typeLabel = new QLabel("Тип лабораторной работы:", &dialog);
    layout->addWidget(typeLabel);

    QComboBox *typeCombo = new QComboBox(&dialog);
    typeCombo->addItem("Lab5", "Lab5");
    typeCombo->addItem("Lab6", "Lab6");
    layout->addWidget(typeCombo);

    QLabel *nameLabel = new QLabel("Название запроса:", &dialog);
    layout->addWidget(nameLabel);

    QLineEdit *nameEdit = new QLineEdit(&dialog);
    nameEdit->setPlaceholderText("Введите название запроса...");
    layout->addWidget(nameEdit);

    QLabel *queryLabel = new QLabel("SQL-запрос:", &dialog);
    layout->addWidget(queryLabel);

    QTextEdit *queryEdit = new QTextEdit(&dialog);
    queryEdit->setPlaceholderText("Введите SQL-запрос здесь...");
    layout->addWidget(queryEdit);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    if (dialog.exec() == QDialog::Accepted) {
        QString queryTitle = nameEdit->text().trimmed();
        QString queryText = queryEdit->toPlainText().trimmed();
        QString type = typeCombo->currentData().toString();

        if (queryTitle.isEmpty() || queryText.isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Название и запрос не могут быть пустыми!");
            return;
        }

        // Находим следующий номер запроса для выбранного типа
        int nextNumber = 1;
        QString prefix = (type == "Lab5") ? "5" : "6";
        
        foreach (const QueryInfo &q, m_allQueries) {
            if (q.type == type) {
                QStringList parts = q.number.split('.');
                if (parts.size() >= 2 && parts[0] == prefix) {
                    int num = parts[1].toInt();
                    if (num >= nextNumber) {
                        nextNumber = num + 1;
                    }
                }
            }
        }

        QString number = QString("%1.%2").arg(prefix).arg(nextNumber);
        QString resourcesPath = findResourcesDirectory();
        QDir typeDir(QDir(resourcesPath).absoluteFilePath(type));
        
        if (!typeDir.exists()) {
            typeDir.mkpath(".");
        }

        QString fileName = QString("%1.sql").arg(number);
        QString filePath = typeDir.absoluteFilePath(fileName);

        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out.setCodec("UTF-8");
            out << QString("-- Запрос %1: %2\n").arg(number).arg(queryTitle);
            out << queryText;
            file.close();

            // Перезагружаем запросы
            loadQueries();

            // Находим и выполняем новый запрос
            foreach (const QueryInfo &q, m_allQueries) {
                if (q.number == number && q.type == type) {
                    runQuery(q);
                    break;
                }
            }
        } else {
            QMessageBox::critical(this, "Ошибка", "Не удалось сохранить запрос в файл");
        }
    }
}

void QueriesWindow::showQueryContextMenu(const QPoint &pos)
{
    QTableWidgetItem *item = m_table->itemAt(pos);
    if (!item) {
        return;
    }

    int row = item->row();
    if (row < 0 || row >= m_filteredQueries.size()) {
        return;
    }

    QueryInfo queryInfo = m_filteredQueries[row];

    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background-color: white; border: 1px solid #ccc; }"
        "QMenu::item { padding: 5px 25px 5px 20px; color: black; }"
        "QMenu::item:selected { background-color: #E0B0FF; color: white; }"
    );

    QAction *deleteAction = menu.addAction("Удалить запрос");
    QAction *backupAction = menu.addAction("Создать резервную копию");

    QAction *selectedAction = menu.exec(m_table->viewport()->mapToGlobal(pos));

    if (selectedAction == deleteAction) {
        deleteQuery(queryInfo);
    } else if (selectedAction == backupAction) {
        backupQuery(queryInfo);
    }
}

void QueriesWindow::deleteQuery(const QueryInfo &queryInfo)
{
    int ret = QMessageBox::question(this, "Удаление запроса",
        QString("Вы уверены, что хотите удалить запрос %1 (%2)?").arg(queryInfo.number).arg(queryInfo.type),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        // Удаляем файл
        QFile::remove(queryInfo.filePath);

        // Перезагружаем запросы
        loadQueries();

        QMessageBox::information(this, "Успех", QString("Запрос %1 удален").arg(queryInfo.number));
    }
}

void QueriesWindow::backupQuery(const QueryInfo &queryInfo)
{
    // Определяем путь к директории exports через BackupManager
    BackupManager tempBackupManager(m_dbManager);
    QString exportsDir = tempBackupManager.getExportsDirectory();
    QString numberCopy = queryInfo.number;
    QString defaultPath = QDir(exportsDir).absoluteFilePath(QString("query_%1_%2_backup.sql").arg(queryInfo.type).arg(numberCopy.replace('.', '_')));
    
    QString fileName = QFileDialog::getSaveFileName(this,
        "Сохранить резервную копию запроса",
        defaultPath,
        "SQL Files (*.sql)");

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out.setCodec("UTF-8");
            out << QString("-- Резервная копия запроса %1 (%2): %3\n").arg(queryInfo.number).arg(queryInfo.type).arg(queryInfo.description);
            out << queryInfo.sqlText;
            file.close();
            QMessageBox::information(this, "Успех", "Резервная копия создана:\n" + fileName);
        }
    }
}

void QueriesWindow::goBack()
{
    if (parentWidget()) {
        parentWidget()->raise();
        parentWidget()->activateWindow();
    }
    hide();
}
