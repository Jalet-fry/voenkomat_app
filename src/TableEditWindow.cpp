#include "TableEditWindow.h"
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QRegExp>
#include <QComboBox>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QMap>

TableEditWindow::TableEditWindow(DatabaseManager *dbManager, const QString &tableName, QWidget *parent)
    : QWidget(parent)
    , m_dbManager(dbManager)
    , m_originalTableName(tableName)
    , m_tableName(tableName)
    , m_foreignKeysGroup(nullptr)
    , m_fkScrollArea(nullptr)
    , m_foreignKeysLayout(nullptr)
{
    setWindowTitle(QString("Редактирование таблицы %1").arg(tableName));
    setGeometry(200, 200, 600, 600);
    
    setupUI();
    setupStyles();
    loadTableColumns();
    loadForeignKeys();
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

    // Группа внешних ключей
    m_foreignKeysGroup = new QGroupBox("Внешние ключи", this);
    m_fkScrollArea = new QScrollArea(this);
    m_fkScrollArea->setWidgetResizable(true);
    QWidget *fkScrollContent = new QWidget();
    m_foreignKeysLayout = new QVBoxLayout(fkScrollContent);
    m_fkScrollArea->setWidget(fkScrollContent);
    
    QVBoxLayout *fkGroupLayout = new QVBoxLayout(m_foreignKeysGroup);
    fkGroupLayout->addWidget(m_fkScrollArea);
    layout->addWidget(m_foreignKeysGroup);

    // Кнопка добавления внешнего ключа
    QPushButton *addForeignKeyBtn = new QPushButton("Добавить внешний ключ", this);
    addForeignKeyBtn->setStyleSheet(
        "QPushButton { background-color: #B6C1FF; font-size: 16px; padding: 10px; border-radius: 8px; }"
        "QPushButton:hover { background-color: #6980FF; }"
    );
    connect(addForeignKeyBtn, &QPushButton::clicked, this, &TableEditWindow::addForeignKey);
    layout->addWidget(addForeignKeyBtn);

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

    // Работа с внешними ключами
    // Получаем существующие foreign keys
    QList<DatabaseManager::ForeignKeyInfo> existingFKs = m_dbManager->getForeignKeyInfo(m_tableName);
    
    // Создаем карту существующих foreign keys по constraint_name
    QMap<QString, DatabaseManager::ForeignKeyInfo> existingFKMap;
    foreach (const DatabaseManager::ForeignKeyInfo &fk, existingFKs) {
        existingFKMap[fk.constraintName] = fk;
    }
    
    // Создаем карту новых foreign keys по комбинации columnName + referencedTable + referencedColumn
    QMap<QString, DatabaseManager::ForeignKeyInfo> newFKMap;
    foreach (const DatabaseManager::ForeignKeyInfo &fk, m_currentForeignKeys) {
        QString key = QString("%1|%2|%3").arg(fk.columnName).arg(fk.referencedTable).arg(fk.referencedColumn);
        newFKMap[key] = fk;
    }
    
    // Удаляем foreign keys, которых нет в новом списке
    foreach (const DatabaseManager::ForeignKeyInfo &existingFK, existingFKs) {
        QString key = QString("%1|%2|%3").arg(existingFK.columnName).arg(existingFK.referencedTable).arg(existingFK.referencedColumn);
        if (!newFKMap.contains(key)) {
            sqlCommands << QString("ALTER TABLE \"%1\" DROP CONSTRAINT \"%2\"")
                .arg(m_tableName)
                .arg(existingFK.constraintName);
        }
    }
    
    // Добавляем новые foreign keys
    foreach (const DatabaseManager::ForeignKeyInfo &newFK, m_currentForeignKeys) {
        QString key = QString("%1|%2|%3").arg(newFK.columnName).arg(newFK.referencedTable).arg(newFK.referencedColumn);
        bool exists = false;
        foreach (const DatabaseManager::ForeignKeyInfo &existingFK, existingFKs) {
            QString existingKey = QString("%1|%2|%3").arg(existingFK.columnName).arg(existingFK.referencedTable).arg(existingFK.referencedColumn);
            if (key == existingKey) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            // Используем метод addForeignKey для генерации правильного SQL
            // Но сначала нужно проверить, что колонка существует
            bool columnExists = false;
            foreach (const auto &col, newColumns) {
                if (col.first == newFK.columnName) {
                    columnExists = true;
                    break;
                }
            }
            if (columnExists) {
                // Добавляем команду для создания foreign key
                QString constraintName = QString("fk_%1_%2").arg(m_tableName).arg(newFK.columnName).toLower();
                QString fkSql = QString("ALTER TABLE \"%1\" ADD CONSTRAINT \"%2\" FOREIGN KEY (\"%3\") REFERENCES \"%4\"(\"%5\")")
                    .arg(m_tableName)
                    .arg(constraintName)
                    .arg(newFK.columnName)
                    .arg(newFK.referencedTable)
                    .arg(newFK.referencedColumn);
                if (newFK.deleteRule == "CASCADE" || newFK.deleteRule == "RESTRICT" || 
                    newFK.deleteRule == "SET NULL" || newFK.deleteRule == "NO ACTION") {
                    fkSql += QString(" ON DELETE %1").arg(newFK.deleteRule);
                }
                sqlCommands << fkSql;
            }
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

void TableEditWindow::loadForeignKeys()
{
    m_currentForeignKeys = m_dbManager->getForeignKeyInfo(m_tableName);
    
    foreach (const DatabaseManager::ForeignKeyInfo &fkInfo, m_currentForeignKeys) {
        createForeignKeyWidget(fkInfo);
    }
}

QWidget* TableEditWindow::createForeignKeyWidget(const DatabaseManager::ForeignKeyInfo &fkInfo)
{
    QWidget *widget = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(widget);
    layout->setContentsMargins(5, 5, 5, 5);

    QString displayText = QString("%1 → %2.%3 [ON DELETE %4]")
        .arg(fkInfo.columnName)
        .arg(fkInfo.referencedTable)
        .arg(fkInfo.referencedColumn)
        .arg(fkInfo.deleteRule);

    QLabel *label = new QLabel(displayText, widget);
    label->setStyleSheet("QLabel { padding: 4px; }");
    layout->addWidget(label);
    
    layout->addStretch();

    QPushButton *removeBtn = new QPushButton("×", widget);
    removeBtn->setStyleSheet("QPushButton { background-color: white; color: red; border: 1px solid #ccc; border-radius: 4px; min-width: 30px; }");
    connect(removeBtn, &QPushButton::clicked, [this, widget]() { removeForeignKey(widget); });
    layout->addWidget(removeBtn);

    m_foreignKeysLayout->addWidget(widget);
    m_foreignKeyWidgets << widget;

    return widget;
}

QDialog* TableEditWindow::createAddForeignKeyDialog()
{
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Добавить внешний ключ");
    dialog->setModal(true);

    QFormLayout *formLayout = new QFormLayout(dialog);

    // Выбор колонки текущей таблицы
    QComboBox *columnCombo = new QComboBox(dialog);
    QStringList columns = m_dbManager->getColumnList(m_tableName);
    foreach (const QString &col, columns) {
        if (col != "id") { // Пропускаем ID
            columnCombo->addItem(col);
        }
    }
    formLayout->addRow("Колонка:", columnCombo);

    // Выбор целевой таблицы
    QComboBox *tableCombo = new QComboBox(dialog);
    QStringList tables = m_dbManager->getTableList();
    foreach (const QString &table, tables) {
        if (table != m_tableName) { // Исключаем текущую таблицу
            tableCombo->addItem(table);
        }
    }
    formLayout->addRow("Целевая таблица:", tableCombo);

    // Выбор целевой колонки
    QComboBox *refColumnCombo = new QComboBox(dialog);
    formLayout->addRow("Целевая колонка:", refColumnCombo);

    // Функция обновления списка колонок при изменении таблицы
    auto updateColumns = [this, refColumnCombo, tableCombo]() {
        refColumnCombo->clear();
        if (tableCombo->currentIndex() >= 0) {
            QString tableName = tableCombo->currentText();
            QStringList columns = m_dbManager->getColumnList(tableName);
            foreach (const QString &col, columns) {
                refColumnCombo->addItem(col);
            }
        }
    };
    // Используем static_cast для совместимости с Qt 5.5.1 (QOverload появился в Qt 5.7)
    connect(tableCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), updateColumns);
    if (tableCombo->count() > 0) {
        updateColumns();
    }

    // Выбор правила ON DELETE
    QComboBox *deleteRuleCombo = new QComboBox(dialog);
    deleteRuleCombo->addItems(QStringList() << "RESTRICT" << "CASCADE" << "SET NULL" << "NO ACTION");
    deleteRuleCombo->setCurrentText("RESTRICT");
    formLayout->addRow("ON DELETE:", deleteRuleCombo);

    // Кнопки
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, dialog);
    connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    formLayout->addRow(buttonBox);

    // Обработка результата
    if (dialog->exec() == QDialog::Accepted) {
        QString columnName = columnCombo->currentText();
        QString referencedTable = tableCombo->currentText();
        QString referencedColumn = refColumnCombo->currentText();
        QString deleteRule = deleteRuleCombo->currentText();

        if (!columnName.isEmpty() && !referencedTable.isEmpty() && !referencedColumn.isEmpty()) {
            DatabaseManager::ForeignKeyInfo fkInfo;
            fkInfo.columnName = columnName;
            fkInfo.referencedTable = referencedTable;
            fkInfo.referencedColumn = referencedColumn;
            fkInfo.deleteRule = deleteRule;
            fkInfo.constraintName = QString("fk_%1_%2").arg(m_tableName).arg(columnName).toLower();

            m_currentForeignKeys << fkInfo;
            createForeignKeyWidget(fkInfo);
        }
    }

    return dialog;
}

void TableEditWindow::addForeignKey()
{
    createAddForeignKeyDialog();
}

void TableEditWindow::removeForeignKey(QWidget *fkWidget)
{
    // Находим соответствующий ForeignKeyInfo
    int index = m_foreignKeyWidgets.indexOf(fkWidget);
    if (index >= 0 && index < m_currentForeignKeys.size()) {
        m_currentForeignKeys.removeAt(index);
        m_foreignKeyWidgets.removeAll(fkWidget);
        fkWidget->deleteLater();
    }
}

void TableEditWindow::onReferencedTableChanged()
{
    // Этот метод будет использоваться для обновления списка колонок
    // Реализован в createAddForeignKeyDialog
}

void TableEditWindow::goBack()
{
    if (parentWidget()) {
        parentWidget()->raise();
        parentWidget()->activateWindow();
    }
    hide();
}

