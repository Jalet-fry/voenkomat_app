// TODO: [REVIEW] OK.
// TODO: [UI] Contrast is good (black on white).
// TODO: [CUA] check if it should be setFixedSize as per LABS_COMPLIANCE.md.

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
    setMinimumSize(800, 550);
    
    m_dataTypes << "INTEGER" << "BIGINT" << "SMALLINT" 
                << "VARCHAR" << "TEXT" << "CHAR"
                << "NUMERIC" << "DECIMAL" << "REAL" << "DOUBLE PRECISION"
                << "DATE" << "TIMESTAMP" << "TIME"
                << "BOOLEAN" << "BYTEA";
    
    setupUI();
    setupStyles();
}

CreateTableDialog::~CreateTableDialog() {}

void CreateTableDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    QLabel *nameLabel = new QLabel("Название таблицы (на латинице):", this);
    mainLayout->addWidget(nameLabel);
    
    m_tableNameEdit = new QLineEdit(this);
    m_tableNameEdit->setPlaceholderText("Например: test_table");
    m_tableNameEdit->setStyleSheet("QLineEdit { color: black; background: white; padding: 8px; font-size: 14px; }");
    mainLayout->addWidget(m_tableNameEdit);

    m_columnsTable = new QTableWidget(this);
    m_columnsTable->setColumnCount(5);
    QStringList headers;
    headers << "Название" << "Тип данных" << "Длина" << "NOT NULL" << "Первичный ключ";
    m_columnsTable->setHorizontalHeaderLabels(headers);
    m_columnsTable->horizontalHeader()->setStretchLastSection(true);

    // ГАРАНТИРУЕМ ВИДИМОСТЬ ТЕКСТА В ТАБЛИЦЕ
    m_columnsTable->setStyleSheet(
        "QTableWidget { background-color: white; color: black; gridline-color: #dcdde1; }"
        "QHeaderView::section { background-color: #f1f2f6; color: black; padding: 5px; font-weight: bold; }"
    );

    mainLayout->addWidget(m_columnsTable);

    QHBoxLayout *columnButtonsLayout = new QHBoxLayout();
    m_addColumnBtn = new QPushButton(" + Добавить колонку", this);
    m_removeColumnBtn = new QPushButton(" - Удалить колонку", this);
    columnButtonsLayout->addWidget(m_addColumnBtn);
    columnButtonsLayout->addWidget(m_removeColumnBtn);
    columnButtonsLayout->addStretch();
    mainLayout->addLayout(columnButtonsLayout);

    connect(m_addColumnBtn, &QPushButton::clicked, this, &CreateTableDialog::addColumn);
    connect(m_removeColumnBtn, &QPushButton::clicked, this, &CreateTableDialog::removeColumn);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    m_okBtn = new QPushButton("Создать таблицу", this);
    m_cancelBtn = new QPushButton("Отмена", this);
    buttonLayout->addWidget(m_okBtn);
    buttonLayout->addWidget(m_cancelBtn);
    mainLayout->addLayout(buttonLayout);

    connect(m_okBtn, &QPushButton::clicked, this, &CreateTableDialog::onOkClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &CreateTableDialog::reject);

    addColumn();
}

void CreateTableDialog::setupStyles()
{
    setStyleSheet(
        "QDialog { background-color: #f5f6fa; }"
        "QLabel { color: #2f3640; font-weight: bold; }"
        "QPushButton { background-color: #3498db; color: white; border-radius: 4px; padding: 10px; font-weight: bold; min-width: 100px; }"
        "QPushButton:hover { background-color: #2980b9; }"
        "QPushButton#cancelBtn { background-color: #e74c3c; }"
    );
    m_cancelBtn->setObjectName("cancelBtn");
}

void CreateTableDialog::addColumn()
{
    int row = m_columnsTable->rowCount();
    m_columnsTable->insertRow(row);

    QTableWidgetItem *nameItem = new QTableWidgetItem("col_" + QString::number(row + 1));
    nameItem->setForeground(QBrush(Qt::black));
    m_columnsTable->setItem(row, 0, nameItem);

    QComboBox *typeCombo = new QComboBox(this);
    typeCombo->addItems(m_dataTypes);
    typeCombo->setStyleSheet("color: black; background: white;");
    m_columnsTable->setCellWidget(row, 1, typeCombo);

    QSpinBox *lengthSpin = new QSpinBox(this);
    lengthSpin->setRange(0, 10000);
    lengthSpin->setValue(255);
    lengthSpin->setStyleSheet("color: black; background: white;");
    m_columnsTable->setCellWidget(row, 2, lengthSpin);

    m_columnsTable->setCellWidget(row, 3, new QCheckBox(this));
    m_columnsTable->setCellWidget(row, 4, new QCheckBox(this));
}

void CreateTableDialog::removeColumn()
{
    if (m_columnsTable->rowCount() > 1) {
        m_columnsTable->removeRow(m_columnsTable->currentRow() >= 0 ? m_columnsTable->currentRow() : m_columnsTable->rowCount() - 1);
    }
}

void CreateTableDialog::onOkClicked()
{
    QString tableName = m_tableNameEdit->text().trimmed();
    if (tableName.isEmpty()) { QMessageBox::warning(this, "Ошибка", "Введите название"); return; }
    
    bool isValid = false;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QRegularExpression re("^[a-zA-Z_][a-zA-Z0-9_]*$");
    isValid = re.match(tableName).hasMatch();
#else
    QRegExp re("^[a-zA-Z_][a-zA-Z0-9_]*$");
    isValid = re.exactMatch(tableName);
#endif

    if (!isValid) { QMessageBox::warning(this, "Ошибка", "Недопустимые символы в названии"); return; }
    accept();
}

QString CreateTableDialog::getTableName() const { return m_tableNameEdit->text().trimmed(); }

QList<QPair<QString, QString>> CreateTableDialog::getColumns() const
{
    QList<QPair<QString, QString>> columns;
    for (int i = 0; i < m_columnsTable->rowCount(); ++i) {
        QString name = m_columnsTable->item(i, 0)->text();
        QString type = qobject_cast<QComboBox*>(m_columnsTable->cellWidget(i, 1))->currentText();
        int len = qobject_cast<QSpinBox*>(m_columnsTable->cellWidget(i, 2))->value();
        bool notNull = qobject_cast<QCheckBox*>(m_columnsTable->cellWidget(i, 3))->isChecked();
        
        QString fullType = type;
        if ((type == "VARCHAR" || type == "CHAR") && len > 0) fullType += QString("(%1)").arg(len);
        if (notNull) fullType += " NOT NULL";
        
        columns << qMakePair(name, fullType);
    }
    return columns;
}

QStringList CreateTableDialog::getPrimaryKeys() const
{
    QStringList pks;
    for (int i = 0; i < m_columnsTable->rowCount(); ++i) {
        if (qobject_cast<QCheckBox*>(m_columnsTable->cellWidget(i, 4))->isChecked()) {
            pks << m_columnsTable->item(i, 0)->text();
        }
    }
    return pks;
}
