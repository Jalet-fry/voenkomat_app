#include "CreateTableDialog.h"
#include <QHeaderView>
#include <QSpinBox>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QRegularExpression>
#else
#include <QRegExp>
#endif

CreateTableDialog::CreateTableDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Создание новой таблицы");
    setMinimumSize(700, 500);
    
    m_dataTypes << "INTEGER" << "BIGINT" << "SMALLINT" 
                << "VARCHAR" << "TEXT" << "CHAR"
                << "NUMERIC" << "DECIMAL" << "REAL" << "DOUBLE PRECISION"
                << "DATE" << "TIMESTAMP" << "TIME"
                << "BOOLEAN" << "BYTEA";
    
    setupUI();
    setupStyles();
}

CreateTableDialog::~CreateTableDialog()
{
}

void CreateTableDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    // Название таблицы
    QLabel *nameLabel = new QLabel("Название таблицы:", this);
    mainLayout->addWidget(nameLabel);
    
    m_tableNameEdit = new QLineEdit(this);
    m_tableNameEdit->setPlaceholderText("Введите название таблицы (например: test_table)");
    mainLayout->addWidget(m_tableNameEdit);

    // Таблица колонок
    QLabel *columnsLabel = new QLabel("Колонки:", this);
    mainLayout->addWidget(columnsLabel);
    
    m_columnsTable = new QTableWidget(this);
    m_columnsTable->setColumnCount(5);
    QStringList headers;
    headers << "Название" << "Тип данных" << "Длина" << "NOT NULL" << "Первичный ключ";
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

    connect(m_addColumnBtn, &QPushButton::clicked, this, &CreateTableDialog::addColumn);
    connect(m_removeColumnBtn, &QPushButton::clicked, this, &CreateTableDialog::removeColumn);
    connect(m_columnsTable, &QTableWidget::itemSelectionChanged, [this]() {
        m_removeColumnBtn->setEnabled(m_columnsTable->currentRow() >= 0);
    });

    // Кнопки OK/Cancel
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    m_okBtn = new QPushButton("Создать", this);
    m_cancelBtn = new QPushButton("Отмена", this);
    buttonLayout->addWidget(m_okBtn);
    buttonLayout->addWidget(m_cancelBtn);
    mainLayout->addLayout(buttonLayout);

    connect(m_okBtn, &QPushButton::clicked, this, &CreateTableDialog::onOkClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &CreateTableDialog::onCancelClicked);

    // Добавляем первую колонку по умолчанию
    addColumn();
}

void CreateTableDialog::setupStyles()
{
    setStyleSheet(
        "QDialog { background-color: #f0f0f0; }"
        "QLabel { font-weight: bold; color: #333; }"
        "QLineEdit { padding: 5px; border: 1px solid #ccc; border-radius: 4px; }"
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

void CreateTableDialog::addColumn()
{
    int row = m_columnsTable->rowCount();
    m_columnsTable->insertRow(row);

    // Название колонки
    QTableWidgetItem *nameItem = new QTableWidgetItem("column_" + QString::number(row + 1));
    m_columnsTable->setItem(row, 0, nameItem);

    // Тип данных
    QComboBox *typeCombo = new QComboBox(this);
    typeCombo->addItems(m_dataTypes);
    typeCombo->setCurrentText("VARCHAR");
    m_columnsTable->setCellWidget(row, 1, typeCombo);

    // Длина (для VARCHAR, CHAR, NUMERIC)
    QSpinBox *lengthSpin = new QSpinBox(this);
    lengthSpin->setMinimum(1);
    lengthSpin->setMaximum(10000);
    lengthSpin->setValue(255);
    lengthSpin->setEnabled(true);
    m_columnsTable->setCellWidget(row, 2, lengthSpin);

    // NOT NULL
    QCheckBox *notNullCheck = new QCheckBox(this);
    m_columnsTable->setCellWidget(row, 3, notNullCheck);

    // Первичный ключ
    QCheckBox *pkCheck = new QCheckBox(this);
    m_columnsTable->setCellWidget(row, 4, pkCheck);

    // Обновляем доступность поля длины в зависимости от типа данных
    connect(typeCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [lengthSpin, typeCombo](int index) {
        QString type = typeCombo->itemText(index);
        bool enabled = (type == "VARCHAR" || type == "CHAR" || type == "NUMERIC" || type == "DECIMAL");
        lengthSpin->setEnabled(enabled);
        if (!enabled) {
            lengthSpin->setValue(0);
        }
    });
}

void CreateTableDialog::removeColumn()
{
    int currentRow = m_columnsTable->currentRow();
    if (currentRow >= 0) {
        m_columnsTable->removeRow(currentRow);
        m_removeColumnBtn->setEnabled(m_columnsTable->rowCount() > 0);
    }
}

QString CreateTableDialog::getDataTypeString(int dataTypeIndex, const QString &length)
{
    if (dataTypeIndex < 0 || dataTypeIndex >= m_dataTypes.size()) {
        return "TEXT";
    }
    
    QString type = m_dataTypes[dataTypeIndex];
    
    // Для типов, требующих длины
    if ((type == "VARCHAR" || type == "CHAR" || type == "NUMERIC" || type == "DECIMAL") && !length.isEmpty()) {
        return QString("%1(%2)").arg(type).arg(length);
    }
    
    return type;
}

void CreateTableDialog::onOkClicked()
{
    QString tableName = m_tableNameEdit->text().trimmed();
    
    if (tableName.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите название таблицы");
        return;
    }
    
    // Проверяем валидность имени таблицы (только буквы, цифры, подчеркивания)
    bool isValid = false;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QRegularExpression nameRegex("^[a-zA-Z_][a-zA-Z0-9_]*$");
    isValid = nameRegex.match(tableName).hasMatch();
#else
    QRegExp nameRegex("^[a-zA-Z_][a-zA-Z0-9_]*$");
    isValid = nameRegex.exactMatch(tableName);
#endif

    if (!isValid) {
        QMessageBox::warning(this, "Ошибка", 
            "Название таблицы может содержать только буквы, цифры и подчеркивания, и должно начинаться с буквы или подчеркивания");
        return;
    }
    
    if (m_columnsTable->rowCount() == 0) {
        QMessageBox::warning(this, "Ошибка", "Добавьте хотя бы одну колонку");
        return;
    }
    
    // Проверяем, что есть хотя бы одна колонка с первичным ключом
    bool hasPrimaryKey = false;
    for (int i = 0; i < m_columnsTable->rowCount(); ++i) {
        QCheckBox *pkCheck = qobject_cast<QCheckBox*>(m_columnsTable->cellWidget(i, 4));
        if (pkCheck && pkCheck->isChecked()) {
            hasPrimaryKey = true;
            break;
        }
    }
    
    if (!hasPrimaryKey) {
        int ret = QMessageBox::question(this, "Предупреждение", 
            "Не указан первичный ключ. Продолжить создание таблицы без первичного ключа?",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret == QMessageBox::No) {
            return;
        }
    }
    
    accept();
}

void CreateTableDialog::onCancelClicked()
{
    reject();
}

QString CreateTableDialog::getTableName() const
{
    return m_tableNameEdit->text().trimmed();
}

QList<QPair<QString, QString>> CreateTableDialog::getColumns() const
{
    QList<QPair<QString, QString>> columns;
    
    for (int i = 0; i < m_columnsTable->rowCount(); ++i) {
        QTableWidgetItem *nameItem = m_columnsTable->item(i, 0);
        if (!nameItem || nameItem->text().trimmed().isEmpty()) {
            continue;
        }
        
        QString columnName = nameItem->text().trimmed();
        
        QComboBox *typeCombo = qobject_cast<QComboBox*>(m_columnsTable->cellWidget(i, 1));
        QSpinBox *lengthSpin = qobject_cast<QSpinBox*>(m_columnsTable->cellWidget(i, 2));
        QCheckBox *notNullCheck = qobject_cast<QCheckBox*>(m_columnsTable->cellWidget(i, 3));
        
        if (!typeCombo) continue;
        
        QString dataType = typeCombo->currentText();
        
        // Добавляем длину для соответствующих типов
        if ((dataType == "VARCHAR" || dataType == "CHAR" || dataType == "NUMERIC" || dataType == "DECIMAL") && lengthSpin) {
            int length = lengthSpin->value();
            if (length > 0) {
                dataType = QString("%1(%2)").arg(dataType).arg(length);
            }
        }
        
        // Добавляем NOT NULL если установлено
        if (notNullCheck && notNullCheck->isChecked()) {
            dataType += " NOT NULL";
        }
        
        columns << qMakePair(columnName, dataType);
    }
    
    return columns;
}

QStringList CreateTableDialog::getPrimaryKeys() const
{
    QStringList primaryKeys;
    
    for (int i = 0; i < m_columnsTable->rowCount(); ++i) {
        QCheckBox *pkCheck = qobject_cast<QCheckBox*>(m_columnsTable->cellWidget(i, 4));
        if (pkCheck && pkCheck->isChecked()) {
            QTableWidgetItem *nameItem = m_columnsTable->item(i, 0);
            if (nameItem) {
                primaryKeys << nameItem->text().trimmed();
            }
        }
    }
    
    return primaryKeys;
}
