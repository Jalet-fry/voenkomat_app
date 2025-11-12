#include "TableAdditionWindow.h"
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QRegExp>
#include <QComboBox>
#include <QLineEdit>

TableAdditionWindow::TableAdditionWindow(DatabaseManager *dbManager, QWidget *parent)
    : QWidget(parent)
    , m_dbManager(dbManager)
{
    setWindowTitle("Добавление таблицы");
    setGeometry(200, 200, 500, 400);
    
    setupUI();
    setupStyles();
    addColumn(); // Добавляем первый столбец по умолчанию
}

TableAdditionWindow::~TableAdditionWindow()
{
}

void TableAdditionWindow::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(10);
    layout->setContentsMargins(20, 20, 20, 20);

    // Поле имени таблицы
    m_tableNameInput = new QLineEdit(this);
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
    connect(addColumnBtn, &QPushButton::clicked, this, &TableAdditionWindow::addColumn);
    layout->addWidget(addColumnBtn);

    // Кнопки управления
    QHBoxLayout *btnLayout = new QHBoxLayout();
    
    QPushButton *backBtn = new QPushButton("Назад", this);
    backBtn->setStyleSheet(
        "QPushButton { background-color: #E0B0FF; font-size: 16px; padding: 10px; border-radius: 8px; }"
        "QPushButton:hover { background-color: #c770ff; }"
    );
    connect(backBtn, &QPushButton::clicked, this, &TableAdditionWindow::goBack);
    btnLayout->addWidget(backBtn);

    QPushButton *createBtn = new QPushButton("Создать таблицу", this);
    createBtn->setStyleSheet(
        "QPushButton { background-color: #5cffda; font-size: 16px; padding: 10px; border-radius: 8px; }"
        "QPushButton:hover { background-color: #00fac1; }"
    );
    connect(createBtn, &QPushButton::clicked, this, &TableAdditionWindow::createTable);
    btnLayout->addWidget(createBtn);

    layout->addLayout(btnLayout);
}

void TableAdditionWindow::setupStyles()
{
    setStyleSheet("QWidget { background-color: #dbffff; }");
}

QWidget* TableAdditionWindow::createColumnWidget()
{
    QWidget *widget = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);

    QLineEdit *nameEdit = new QLineEdit();
    nameEdit->setPlaceholderText("Имя столбца");
    nameEdit->setStyleSheet("QLineEdit { padding: 4px; border: 1px solid #ccc; border-radius: 4px; }");
    layout->addWidget(nameEdit);

    QComboBox *typeCombo = new QComboBox();
    typeCombo->addItems(QStringList() << "integer" << "varchar(255)" << "text" << "date" << "boolean");
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

void TableAdditionWindow::addColumn()
{
    createColumnWidget();
}

void TableAdditionWindow::removeColumn(QWidget *columnWidget)
{
    if (m_columnWidgets.size() <= 1) {
        QMessageBox::warning(this, "Ошибка", "Таблица должна содержать хотя бы один столбец!");
        return;
    }

    m_columnWidgets.removeAll(columnWidget);
    columnWidget->deleteLater();
}

void TableAdditionWindow::createTable()
{
    QString tableName = m_tableNameInput->text().trimmed();
    if (tableName.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите имя таблицы!");
        return;
    }

    // Валидация имени таблицы
    QRegExp rx("^[a-zA-Z_][a-zA-Z0-9_]*$");
    if (!rx.exactMatch(tableName)) {
        QMessageBox::warning(this, "Ошибка",
            "Некорректное имя таблицы. Используйте только латинские буквы, цифры и подчеркивания");
        return;
    }

    // Собираем информацию о столбцах
    QList<QPair<QString, QString>> columns;
    foreach (QWidget *widget, m_columnWidgets) {
        QHBoxLayout *layout = qobject_cast<QHBoxLayout*>(widget->layout());
        if (layout) {
            QLineEdit *nameEdit = qobject_cast<QLineEdit*>(layout->itemAt(0)->widget());
            QComboBox *typeCombo = qobject_cast<QComboBox*>(layout->itemAt(1)->widget());
            if (nameEdit && typeCombo) {
                QString name = nameEdit->text().trimmed();
                if (!name.isEmpty()) {
                    // Валидация имени столбца
                    if (!rx.exactMatch(name)) {
                        QMessageBox::warning(this, "Ошибка", QString("Некорректное имя столбца: %1").arg(name));
                        return;
                    }
                    columns << qMakePair(name, typeCombo->currentText());
                }
            }
        }
    }

    if (columns.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Добавьте хотя бы один столбец с указанным именем!");
        return;
    }

    // Формируем SQL запрос
    QStringList columnDefs;
    foreach (const auto &col, columns) {
        columnDefs << QString("\"%1\" %2").arg(col.first).arg(col.second);
    }

    QString sql = QString("CREATE TABLE \"%1\" (\n  id SERIAL PRIMARY KEY,\n  %2\n)").arg(tableName).arg(columnDefs.join(",\n  "));

    // Подтверждение
    int ret = QMessageBox::question(this, "Подтверждение",
        QString("Создать таблицу со следующим SQL-запросом?\n\n%1").arg(sql),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (ret == QMessageBox::No) {
        return;
    }

    // Выполняем запрос
    bool ok;
    m_dbManager->executeQuery(sql, &ok);

    if (ok) {
        QMessageBox::information(this, "Успех", "Таблица успешно создана!");
        goBack();
    } else {
        QMessageBox::critical(this, "Ошибка", "Не удалось создать таблицу:\n" + m_dbManager->lastError());
    }
}

void TableAdditionWindow::goBack()
{
    if (parentWidget()) {
        parentWidget()->raise();
        parentWidget()->activateWindow();
    }
    hide();
}

