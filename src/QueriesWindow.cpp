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
#include <algorithm>

QueriesWindow::QueriesWindow(DatabaseManager *dbManager, QWidget *parent)
    : QWidget(parent)
    , m_dbManager(dbManager)
{
    setWindowTitle("Запросы");
    setGeometry(150, 150, 500, 400);
    
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

    // Сетка кнопок
    m_gridLayout = new QGridLayout();
    m_layout->addLayout(m_gridLayout);

    m_layout->addStretch();

    // Кнопка "Добавить"
    QPushButton *addBtn = new QPushButton("Добавить", this);
    addBtn->setMinimumHeight(40);
    connect(addBtn, &QPushButton::clicked, this, &QueriesWindow::addNewQuery);
    m_layout->addWidget(addBtn);

    // Кнопка "Назад"
    QPushButton *backBtn = new QPushButton("Назад", this);
    backBtn->setMinimumHeight(40);
    connect(backBtn, &QPushButton::clicked, this, &QueriesWindow::goBack);
    m_layout->addWidget(backBtn);
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
    m_queries.clear();
    m_queryTitles.clear();

    // Находим директорию с ресурсами
    QString resourcesPath = findResourcesDirectory();
    QDir queriesDir(resourcesPath);
    
    if (!queriesDir.exists()) {
        qDebug() << "Директория с запросами не найдена:" << resourcesPath;
        return;
    }
    
    qDebug() << "Загрузка запросов из:" << queriesDir.absolutePath();

    QStringList filters;
    filters << "2.1.*.sql";
    QFileInfoList files = queriesDir.entryInfoList(filters, QDir::Files, QDir::Name);

    foreach (const QFileInfo &fileInfo, files) {
        QString fileName = fileInfo.baseName(); // "2.1.1"
        QRegExp rx("2\\.1\\.(\\d+)");
        if (rx.exactMatch(fileName)) {
            int queryNumber = rx.cap(1).toInt();

            QFile file(fileInfo.absoluteFilePath());
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                in.setCodec("UTF-8");
                QString queryText = in.readAll();
                file.close();

                m_queries[queryNumber] = queryText;
                m_queryTitles[queryNumber] = parseQueryTitle(queryText);
            }
        }
    }

    refreshQueryButtons();
}

QString QueriesWindow::parseQueryTitle(const QString &queryText) const
{
    // Парсим название из комментария: -- Запрос 2.1.1: Название
    QRegExp rx("--\\s*(?:Запрос\\s+\\d+\\.\\d+\\.\\d+[::]?\\s*)?(.+)");
    if (rx.indexIn(queryText) != -1) {
        QString title = rx.cap(1).trimmed();
        if (title.isEmpty()) {
            return "Запрос";
        }
        return title;
    }
    return "Запрос";
}

QString QueriesWindow::getQueryFilePath(int queryNumber) const
{
    // Используем ту же логику поиска, что и в loadQueries()
    QString resourcesPath = findResourcesDirectory();
    QDir queriesDir(resourcesPath);
    
    // Если директория не существует, пытаемся создать её
    if (!queriesDir.exists()) {
        queriesDir.mkpath(".");
    }
    
    return queriesDir.absoluteFilePath(QString("2.1.%1.sql").arg(queryNumber));
}

void QueriesWindow::refreshQueryButtons()
{
    // Очистка старых кнопок
    QLayoutItem *item;
    while ((item = m_gridLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    // Создание кнопок
    QList<int> queryNumbers = m_queries.keys();
    std::sort(queryNumbers.begin(), queryNumbers.end());

    int buttonsPerRow = 6;
    int row = 0, col = 0;

    foreach (int queryNum, queryNumbers) {
        QPushButton *btn = new QPushButton(QString::number(queryNum), this);
        QString title = m_queryTitles.value(queryNum, "Запрос");
        btn->setToolTip(title);
        btn->setContextMenuPolicy(Qt::CustomContextMenu);

        connect(btn, &QPushButton::clicked, [this, queryNum]() {
            runQuery(queryNum);
        });

        connect(btn, &QPushButton::customContextMenuRequested, [this, btn, queryNum](const QPoint &pos) {
            showQueryContextMenu(btn->mapToGlobal(pos), queryNum);
        });

        m_gridLayout->addWidget(btn, row, col);
        col++;
        if (col >= buttonsPerRow) {
            col = 0;
            row++;
        }
    }
}

void QueriesWindow::runQuery(int queryNumber)
{
    if (!m_queries.contains(queryNumber)) {
        QMessageBox::warning(this, "Ошибка", "Запрос не найден");
        return;
    }

    QString queryText = m_queries[queryNumber];
    QString queryTitle = m_queryTitles.value(queryNumber, QString("Запрос %1").arg(queryNumber));

    if (!m_dbManager || !m_dbManager->isConnected()) {
        QMessageBox::critical(this, "Ошибка подключения", 
                             "База данных не подключена.\nПроверьте параметры подключения.");
        return;
    }

    bool ok;
    QSqlQuery query = m_dbManager->executeQuery(queryText, &ok);

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

    QueryResultWindow *resultWindow = new QueryResultWindow(queryTitle, columnNames, rows, this);
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

        if (queryTitle.isEmpty() || queryText.isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Название и запрос не могут быть пустыми!");
            return;
        }

        // Находим следующий номер запроса
        int newQueryNumber = 1;
        if (!m_queries.isEmpty()) {
            newQueryNumber = m_queries.keys().last() + 1;
        }

        // Сохраняем запрос в файл
        QString filePath = getQueryFilePath(newQueryNumber);
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out.setCodec("UTF-8");
            out << QString("-- Запрос %1: %2\n").arg(newQueryNumber).arg(queryTitle);
            out << queryText;
            file.close();

            // Добавляем в список
            m_queries[newQueryNumber] = queryText;
            m_queryTitles[newQueryNumber] = queryTitle;
            refreshQueryButtons();

            // Выполняем запрос
            runQuery(newQueryNumber);
        } else {
            QMessageBox::critical(this, "Ошибка", "Не удалось сохранить запрос в файл");
        }
    }
}

void QueriesWindow::showQueryContextMenu(const QPoint &pos, int queryNumber)
{
    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background-color: white; border: 1px solid #ccc; }"
        "QMenu::item { padding: 5px 25px 5px 20px; color: black; }"
        "QMenu::item:selected { background-color: #E0B0FF; color: white; }"
    );

    QAction *deleteAction = menu.addAction("Удалить запрос");
    QAction *backupAction = menu.addAction("Создать резервную копию");

    QAction *selectedAction = menu.exec(pos);

    if (selectedAction == deleteAction) {
        deleteQuery(queryNumber);
    } else if (selectedAction == backupAction) {
        backupQuery(queryNumber);
    }
}

void QueriesWindow::deleteQuery(int queryNumber)
{
    int ret = QMessageBox::question(this, "Удаление запроса",
        QString("Вы уверены, что хотите удалить запрос %1?").arg(queryNumber),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        // Удаляем файл
        QString filePath = getQueryFilePath(queryNumber);
        QFile::remove(filePath);

        // Удаляем из списка
        m_queries.remove(queryNumber);
        m_queryTitles.remove(queryNumber);
        refreshQueryButtons();

        QMessageBox::information(this, "Успех", QString("Запрос %1 удален").arg(queryNumber));
    }
}

void QueriesWindow::backupQuery(int queryNumber)
{
    if (!m_queries.contains(queryNumber)) {
        return;
    }

    // Определяем путь к директории exports через BackupManager
    BackupManager tempBackupManager(m_dbManager);
    QString exportsDir = tempBackupManager.getExportsDirectory();
    QString defaultPath = QDir(exportsDir).absoluteFilePath(QString("query_%1_backup.sql").arg(queryNumber));
    
    QString fileName = QFileDialog::getSaveFileName(this,
        "Сохранить резервную копию запроса",
        defaultPath,
        "SQL Files (*.sql)");

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out.setCodec("UTF-8");
            out << QString("-- Резервная копия запроса %1: %2\n").arg(queryNumber).arg(m_queryTitles[queryNumber]);
            out << m_queries[queryNumber];
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

