#include "TableEditWindow.h"
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QRegExp>
#include <QComboBox>
#include <QLineEdit>

TableEditWindow::TableEditWindow(DatabaseManager *dbManager, const QString &tableName, QWidget *parent)
    : QWidget(parent)
    , m_dbManager(dbManager)
    , m_originalTableName(tableName)
    , m_tableName(tableName)
{
    setWindowTitle(QString("Редактирование таблицы %1").arg(tableName));
    setGeometry(200, 200, 500, 400);
    
    setupUI();
    setupStyles();
    loadTableColumns();
}

TableEditWindow::~TableEditWindow()
{
}

void TableEditWindow::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(10);
    layout->setContentsMargins(20, 20, 20, 20);

    // Поле имени таблицы
    m_tableNameInput = new QLineEdit(this);
    m_tableNameInput->setText(m_tableName);
    m_tableNameInput->setPlaceholderText("Введите имя таблицы...");
    m_tableNameInput->setStyleSheet(
        "QLineEdit { font-size: 14px; padding: 8px; border: 1px solid #ccc; border-radius: 4px; }"
    );
    layout->addWidget(m_tableNameInput);

    // Группа столбцов
    QGroupBox *columnsGroup = new QGroupBox("Столбцы таблицы", this);
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    QWidget *scrollContent = new QWidget();
    m_columnsLayout = new QVBoxLayout(scrollContent);
    m_scrollArea->setWidget(scrollContent);
    
    QVBoxLayout *groupLayout = new QVBoxLayout(columnsGroup);
    groupLayout->addWidget(m_scrollArea);
    layout->addWidget(columnsGroup);

    // Кнопка добавления столбца
    QPushButton *addColumnBtn = new QPushButton("Добавить столбец", this);
    addColumnBtn->setStyleSheet(
        "QPushButton { background-color: #FFB6C1; font-size: 16px; padding: 10px; border-radius: 8px; }"
        "QPushButton:hover { background-color: #FF69B4; }"
    );
    connect(addColumnBtn, &QPushButton::clicked, this, &TableEditWindow::addColumn);
    layout->addWidget(addColumnBtn);

    // Кнопки управления
    QHBoxLayout *btnLayout = new QHBoxLayout();
    
    QPushButton *backBtn = new QPushButton("Назад", this);
    backBtn->setStyleSheet(
        "QPushButton { background-color: #E0B0FF; font-size: 16px; padding: 10px; border-radius: 8px; }"
        "QPushButton:hover { background-color: #c770ff; }"
    );
    connect(backBtn, &QPushButton::clicked, this, &TableEditWindow::goBack);
    btnLayout->addWidget(backBtn);

    QPushButton *saveBtn = new QPushButton("Сохранить изменения", this);
    saveBtn->setStyleSheet(
        "QPushButton { background-color: #5cffda; font-size: 16px; padding: 10px; border-radius: 8px; }"
        "QPushButton:hover { background-color: #00fac1; }"
    );
    connect(saveBtn, &QPushButton::clicked, this, &TableEditWindow::saveChanges);
    btnLayout->addWidget(saveBtn);

    layout->addLayout(btnLayout);
}

void TableEditWindow::setupStyles()
{
    setStyleSheet("QWidget { background-color: #dbffff; }");
}

void TableEditWindow::loadTableColumns()
{
    QList<QPair<QString, QString>> columns = m_dbManager->getColumnInfo(m_tableName);
    
    foreach (const auto &col, columns) {
        if (col.first != "id") { // Пропускаем ID
            createColumnWidget(col.first, col.second);
        }
    }
    
    if (m_columnWidgets.isEmpty()) {
        addColumn();
    }
}

QWidget* TableEditWindow::createColumnWidget(const QString &columnName, const QString &columnType)
{
    QWidget *widget = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);

    QLineEdit *nameEdit = new QLineEdit();
    nameEdit->setText(columnName);
    nameEdit->setPlaceholderText("Имя столбца");
    nameEdit->setStyleSheet("QLineEdit { padding: 4px; border: 1px solid #ccc; border-radius: 4px; }");
    layout->addWidget(nameEdit);

    QComboBox *typeCombo = new QComboBox();
    typeCombo->addItems(QStringList() << "integer" << "varchar(255)" << "text" << "date" << "boolean");
    int index = typeCombo->findText(columnType);
    if (index >= 0) {
        typeCombo->setCurrentIndex(index);
    }
    typeCombo->setStyleSheet("QComboBox { padding: 4px; border: 1px solid #ccc; border-radius: 4px; }");
    layout->addWidget(typeCombo);

    QPushButton *removeBtn = new QPushButton("×");
    removeBtn->setStyleSheet("QPushButton { background-color: white; color: red; border: 1px solid #ccc; border-radius: 4px; }");
    connect(removeBtn, &QPushButton::clicked, [this, widget]() { removeColumn(widget); });
    layout->addWidget(removeBtn);

    m_columnsLayout->addWidget(widget);
    m_columnWidgets << widget;

    return widget;
}

void TableEditWindow::addColumn()
{
    createColumnWidget();
}

void TableEditWindow::removeColumn(QWidget *columnWidget)
{
    if (m_columnWidgets.size() <= 1) {
        QMessageBox::warning(this, "Ошибка", "Таблица должна содержать хотя бы один столбец!");
        return;
    }

    m_columnWidgets.removeAll(columnWidget);
    columnWidget->deleteLater();
}

void TableEditWindow::saveChanges()
{
    QString newTableName = m_tableNameInput->text().trimmed();
    if (newTableName.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Имя таблицы не может быть пустым!");
        return;
    }

    // Собираем информацию о столбцах
    QList<QPair<QString, QString>> newColumns;
    foreach (QWidget *widget, m_columnWidgets) {
        QHBoxLayout *layout = qobject_cast<QHBoxLayout*>(widget->layout());
        if (layout) {
            QLineEdit *nameEdit = qobject_cast<QLineEdit*>(layout->itemAt(0)->widget());
            QComboBox *typeCombo = qobject_cast<QComboBox*>(layout->itemAt(1)->widget());
            if (nameEdit && typeCombo) {
                QString name = nameEdit->text().trimmed();
                if (!name.isEmpty()) {
                    newColumns << qMakePair(name, typeCombo->currentText());
                }
            }
        }
    }

    if (newColumns.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Добавьте хотя бы один столбец!");
        return;
    }

    // Получаем текущие столбцы
    QList<QPair<QString, QString>> existingColumns = m_dbManager->getColumnInfo(m_tableName);
    QMap<QString, QString> existingMap;
    foreach (const auto &col, existingColumns) {
        if (col.first != "id") {
            existingMap[col.first] = col.second;
        }
    }

    QStringList sqlCommands;

    // Переименование таблицы
    if (newTableName != m_originalTableName) {
        sqlCommands << QString("ALTER TABLE \"%1\" RENAME TO \"%2\"").arg(m_originalTableName).arg(newTableName);
        m_tableName = newTableName;
    }

    // Удаление столбцов
    foreach (const QString &colName, existingMap.keys()) {
        bool found = false;
        foreach (const auto &newCol, newColumns) {
            if (newCol.first == colName) {
                found = true;
                break;
            }
        }
        if (!found) {
            sqlCommands << QString("ALTER TABLE \"%1\" DROP COLUMN \"%2\"").arg(m_tableName).arg(colName);
        }
    }

    // Добавление новых столбцов
    foreach (const auto &newCol, newColumns) {
        if (!existingMap.contains(newCol.first)) {
            sqlCommands << QString("ALTER TABLE \"%1\" ADD COLUMN \"%2\" %3").arg(m_tableName).arg(newCol.first).arg(newCol.second);
        } else if (existingMap[newCol.first] != newCol.second) {
            // Изменение типа столбца
            sqlCommands << QString("ALTER TABLE \"%1\" ALTER COLUMN \"%2\" TYPE %3").arg(m_tableName).arg(newCol.first).arg(newCol.second);
        }
    }

    // Выполняем команды
    if (!m_dbManager->beginTransaction()) {
        QMessageBox::critical(this, "Ошибка", "Не удалось начать транзакцию");
        return;
    }

    bool allOk = true;
    foreach (const QString &sql, sqlCommands) {
        bool ok;
        m_dbManager->executeQuery(sql, &ok);
        if (!ok) {
            allOk = false;
            break;
        }
    }

    if (allOk) {
        m_dbManager->commitTransaction();
        QMessageBox::information(this, "Успех", "Таблица успешно обновлена!");
        goBack();
    } else {
        m_dbManager->rollbackTransaction();
        QMessageBox::critical(this, "Ошибка", "Не удалось обновить таблицу:\n" + m_dbManager->lastError());
    }
}

void TableEditWindow::goBack()
{
    if (parentWidget()) {
        parentWidget()->raise();
        parentWidget()->activateWindow();
    }
    hide();
}

