#include "EditTableStructureDialog.h"
#include <QSpinBox>
#include <QCheckBox>
#include <QHeaderView>

EditTableStructureDialog::EditTableStructureDialog(DatabaseManager *dbManager, const QString &tableName, QWidget *parent)
    : QDialog(parent)
    , m_dbManager(dbManager)
    , m_tableName(tableName)
{
    setWindowTitle(QString("Редактирование структуры таблицы: %1").arg(tableName));
    setMinimumSize(700, 500);
    
    m_dataTypes << "INTEGER" << "BIGINT" << "SMALLINT" 
                << "VARCHAR" << "TEXT" << "CHAR"
                << "NUMERIC" << "DECIMAL" << "REAL" << "DOUBLE PRECISION"
                << "DATE" << "TIMESTAMP" << "TIME"
                << "BOOLEAN" << "BYTEA";
    
    setupUI();
    setupStyles();
    loadExistingColumns();
}

EditTableStructureDialog::~EditTableStructureDialog()
{
}

void EditTableStructureDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    // Информация о таблице
    QLabel *infoLabel = new QLabel(QString("Таблица: <b>%1</b>").arg(m_tableName), this);
    mainLayout->addWidget(infoLabel);

    // Таблица колонок
    QLabel *columnsLabel = new QLabel("Колонки (новые колонки будут добавлены, удаленные - удалены):", this);
    mainLayout->addWidget(columnsLabel);
    
    m_columnsTable = new QTableWidget(this);
    m_columnsTable->setColumnCount(4);
    QStringList headers;
    headers << "Название" << "Тип данных" << "Длина" << "NOT NULL";
    m_columnsTable->setHorizontalHeaderLabels(headers);
    m_columnsTable->horizontalHeader()->setStretchLastSection(true);
    m_columnsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_columnsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    mainLayout->addWidget(m_columnsTable);

    // Кнопки управления колонками
    QHBoxLayout *columnButtonsLayout = new QHBoxLayout();
    m_addColumnBtn = new QPushButton("Добавить колонку", this);
    m_removeColumnBtn = new QPushButton("Удалить колонку", this);
    m_removeColumnBtn->setEnabled(false);
    columnButtonsLayout->addWidget(m_addColumnBtn);
    columnButtonsLayout->addWidget(m_removeColumnBtn);
    columnButtonsLayout->addStretch();
    mainLayout->addLayout(columnButtonsLayout);

    connect(m_addColumnBtn, &QPushButton::clicked, this, &EditTableStructureDialog::addColumn);
    connect(m_removeColumnBtn, &QPushButton::clicked, this, &EditTableStructureDialog::removeColumn);
    connect(m_columnsTable, &QTableWidget::itemSelectionChanged, [this]() {
        m_removeColumnBtn->setEnabled(m_columnsTable->currentRow() >= 0);
    });

    // Кнопки OK/Cancel
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    m_okBtn = new QPushButton("Применить изменения", this);
    m_cancelBtn = new QPushButton("Отмена", this);
    buttonLayout->addWidget(m_okBtn);
    buttonLayout->addWidget(m_cancelBtn);
    mainLayout->addLayout(buttonLayout);

    connect(m_okBtn, &QPushButton::clicked, this, &EditTableStructureDialog::onOkClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &EditTableStructureDialog::onCancelClicked);
}

void EditTableStructureDialog::setupStyles()
{
    setStyleSheet(
        "QDialog { background-color: #f0f0f0; }"
        "QLabel { font-weight: bold; color: #333; }"
        "QPushButton {"
        "    padding: 8px 15px;"
        "    border-radius: 4px;"
        "    background-color: #4CAF50;"
        "    color: white;"
        "    border: none;"
        "}"
        "QPushButton:hover { background-color: #45a049; }"
        "QPushButton:pressed { background-color: #3d8b40; }"
        "QPushButton:disabled { background-color: #cccccc; }"
        "QPushButton#cancelBtn { background-color: #f44336; }"
        "QPushButton#cancelBtn:hover { background-color: #da190b; }"
    );
    m_cancelBtn->setObjectName("cancelBtn");
}

void EditTableStructureDialog::loadExistingColumns()
{
    if (!m_dbManager || !m_dbManager->isConnected()) {
        QMessageBox::critical(this, "Ошибка", "База данных не подключена");
        return;
    }
    
    QList<DatabaseManager::ColumnDetail> columns = m_dbManager->getColumnDetails(m_tableName);
    
    foreach (const DatabaseManager::ColumnDetail &col, columns) {
        m_existingColumns << col.columnName;
        
        int row = m_columnsTable->rowCount();
        m_columnsTable->insertRow(row);
        
        // Название колонки (только для чтения для существующих)
        QTableWidgetItem *nameItem = new QTableWidgetItem(col.columnName);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        nameItem->setBackground(QBrush(QColor(240, 240, 240)));
        m_columnsTable->setItem(row, 0, nameItem);
        
        // Тип данных
        QComboBox *typeCombo = new QComboBox(this);
        typeCombo->addItems(m_dataTypes);
        
        // Определяем базовый тип (без длины)
        QString baseType = col.dataType;
        if (baseType.contains("(")) {
            baseType = baseType.left(baseType.indexOf("("));
        }
        baseType = baseType.toUpper();
        
        int index = typeCombo->findText(baseType);
        if (index >= 0) {
            typeCombo->setCurrentIndex(index);
        } else {
            typeCombo->setCurrentText(col.dataType);
        }
        typeCombo->setEnabled(false); // Нельзя изменять тип существующих колонок
        m_columnsTable->setCellWidget(row, 1, typeCombo);
        
        // Длина
        QSpinBox *lengthSpin = new QSpinBox(this);
        lengthSpin->setMinimum(1);
        lengthSpin->setMaximum(10000);
        if (col.characterMaxLength > 0) {
            lengthSpin->setValue(col.characterMaxLength);
        }
        lengthSpin->setEnabled(false);
        m_columnsTable->setCellWidget(row, 2, lengthSpin);
        
        // NOT NULL
        QCheckBox *notNullCheck = new QCheckBox(this);
        notNullCheck->setChecked(!col.isNullable);
        notNullCheck->setEnabled(false);
        m_columnsTable->setCellWidget(row, 3, notNullCheck);
    }
}

void EditTableStructureDialog::addColumn()
{
    int row = m_columnsTable->rowCount();
    m_columnsTable->insertRow(row);
    
    // Название колонки
    QTableWidgetItem *nameItem = new QTableWidgetItem("new_column_" + QString::number(row + 1));
    m_columnsTable->setItem(row, 0, nameItem);
    
    // Тип данных
    QComboBox *typeCombo = new QComboBox(this);
    typeCombo->addItems(m_dataTypes);
    typeCombo->setCurrentText("VARCHAR");
    m_columnsTable->setCellWidget(row, 1, typeCombo);
    
    // Длина
    QSpinBox *lengthSpin = new QSpinBox(this);
    lengthSpin->setMinimum(1);
    lengthSpin->setMaximum(10000);
    lengthSpin->setValue(255);
    lengthSpin->setEnabled(true);
    m_columnsTable->setCellWidget(row, 2, lengthSpin);
    
    // NOT NULL
    QCheckBox *notNullCheck = new QCheckBox(this);
    m_columnsTable->setCellWidget(row, 3, notNullCheck);
    
    // Обновляем доступность поля длины
    connect(typeCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [lengthSpin, typeCombo](int index) {
        QString type = typeCombo->itemText(index);
        bool enabled = (type == "VARCHAR" || type == "CHAR" || type == "NUMERIC" || type == "DECIMAL");
        lengthSpin->setEnabled(enabled);
        if (!enabled) {
            lengthSpin->setValue(0);
        }
    });
}

void EditTableStructureDialog::removeColumn()
{
    int currentRow = m_columnsTable->currentRow();
    if (currentRow < 0) return;
    
    QTableWidgetItem *nameItem = m_columnsTable->item(currentRow, 0);
    if (!nameItem) return;
    
    QString columnName = nameItem->text();
    
    // Проверяем, является ли это существующей колонкой
    if (m_existingColumns.contains(columnName)) {
        int ret = QMessageBox::question(this, "Подтверждение удаления", 
            QString("Вы уверены, что хотите удалить колонку '%1'?\n\nЭто действие необратимо и может привести к потере данных!").
            arg(columnName),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        
        if (ret == QMessageBox::Yes) {
            m_columnsTable->removeRow(currentRow);
            m_existingColumns.removeAll(columnName);
        }
    } else {
        // Это новая колонка, просто удаляем
        m_columnsTable->removeRow(currentRow);
    }
    
    m_removeColumnBtn->setEnabled(m_columnsTable->rowCount() > 0);
}

QString EditTableStructureDialog::getDataTypeString(int dataTypeIndex, const QString &length)
{
    if (dataTypeIndex < 0 || dataTypeIndex >= m_dataTypes.size()) {
        return "TEXT";
    }
    
    QString type = m_dataTypes[dataTypeIndex];
    
    if ((type == "VARCHAR" || type == "CHAR" || type == "NUMERIC" || type == "DECIMAL") && !length.isEmpty()) {
        return QString("%1(%2)").arg(type).arg(length);
    }
    
    return type;
}

void EditTableStructureDialog::onOkClicked()
{
    if (!m_dbManager || !m_dbManager->isConnected()) {
        QMessageBox::critical(this, "Ошибка", "База данных не подключена");
        return;
    }
    
    // Определяем, какие колонки нужно добавить и удалить
    QStringList currentColumns;
    QList<QPair<QString, QString>> columnsToAdd; // имя, тип
    
    for (int i = 0; i < m_columnsTable->rowCount(); ++i) {
        QTableWidgetItem *nameItem = m_columnsTable->item(i, 0);
        if (!nameItem) continue;
        
        QString columnName = nameItem->text().trimmed();
        if (columnName.isEmpty()) continue;
        
        currentColumns << columnName;
        
        // Если это новая колонка (не в списке существующих)
        if (!m_existingColumns.contains(columnName)) {
            QComboBox *typeCombo = qobject_cast<QComboBox*>(m_columnsTable->cellWidget(i, 1));
            QSpinBox *lengthSpin = qobject_cast<QSpinBox*>(m_columnsTable->cellWidget(i, 2));
            QCheckBox *notNullCheck = qobject_cast<QCheckBox*>(m_columnsTable->cellWidget(i, 3));
            
            if (!typeCombo) continue;
            
            QString dataType = typeCombo->currentText();
            
            if ((dataType == "VARCHAR" || dataType == "CHAR" || dataType == "NUMERIC" || dataType == "DECIMAL") && lengthSpin) {
                int length = lengthSpin->value();
                if (length > 0) {
                    dataType = QString("%1(%2)").arg(dataType).arg(length);
                }
            }
            
            bool isNullable = !(notNullCheck && notNullCheck->isChecked());
            
            columnsToAdd << qMakePair(columnName, dataType);
        }
    }
    
    // Определяем колонки для удаления
    QStringList columnsToRemove;
    foreach (const QString &existingCol, m_existingColumns) {
        if (!currentColumns.contains(existingCol)) {
            columnsToRemove << existingCol;
        }
    }
    
    if (columnsToAdd.isEmpty() && columnsToRemove.isEmpty()) {
        QMessageBox::information(this, "Информация", "Нет изменений для применения");
        return;
    }
    
    // Применяем изменения
    bool success = true;
    QStringList errors;
    
    // Сначала удаляем колонки
    foreach (const QString &columnName, columnsToRemove) {
        if (!m_dbManager->dropColumn(m_tableName, columnName)) {
            success = false;
            errors << QString("Не удалось удалить колонку '%1': %2").arg(columnName).arg(m_dbManager->lastError());
        }
    }
    
    // Затем добавляем новые колонки
    foreach (const auto &col, columnsToAdd) {
        QString dataType = col.second;
        bool isNullable = !dataType.contains("NOT NULL");
        if (dataType.contains("NOT NULL")) {
            dataType = dataType.replace(" NOT NULL", "").trimmed();
        }
        
        if (!m_dbManager->addColumn(m_tableName, col.first, dataType, isNullable)) {
            success = false;
            errors << QString("Не удалось добавить колонку '%1': %2").arg(col.first).arg(m_dbManager->lastError());
        }
    }
    
    if (success) {
        QMessageBox::information(this, "Успех", 
            QString("Структура таблицы успешно обновлена.\n\nДобавлено колонок: %1\nУдалено колонок: %2")
            .arg(columnsToAdd.size()).arg(columnsToRemove.size()));
        accept();
    } else {
        QMessageBox::critical(this, "Ошибка", 
            QString("Произошли ошибки при обновлении структуры:\n\n%1").arg(errors.join("\n")));
    }
}

void EditTableStructureDialog::onCancelClicked()
{
    reject();
}

