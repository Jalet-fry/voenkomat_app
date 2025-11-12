#include "TablesWindow.h"
#include "TableViewWindow.h"
#include "TableEditWindow.h"
#include "BackupManager.h"
#include <QLabel>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>

TablesWindow::TablesWindow(DatabaseManager *dbManager, QWidget *parent)
    : QWidget(parent)
    , m_dbManager(dbManager)
{
    setWindowTitle("Таблицы");
    setGeometry(200, 200, 400, 500);
    
    setupUI();
    setupStyles();
    refreshTables();
}

TablesWindow::~TablesWindow()
{
}

void TablesWindow::setupUI()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setSpacing(15);
    m_layout->setContentsMargins(20, 20, 20, 20);

    // Заголовок
    QLabel *title = new QLabel("Таблицы базы данных", this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 20px; color: #333; font-weight: bold;");
    m_layout->addWidget(title);

    // Прокручиваемая область для кнопок таблиц
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollContent = new QWidget();
    m_tableButtonsLayout = new QVBoxLayout(m_scrollContent);
    m_tableButtonsLayout->setSpacing(10);
    m_scrollArea->setWidget(m_scrollContent);
    m_layout->addWidget(m_scrollArea);

    // Кнопка "Назад"
    QPushButton *backBtn = new QPushButton("Назад", this);
    backBtn->setMinimumHeight(40);
    connect(backBtn, &QPushButton::clicked, this, &TablesWindow::goBack);
    m_layout->addWidget(backBtn);

    // Инициализация русских названий таблиц
    m_tableDisplayNames["prizivnik"] = "Призывники";
    m_tableDisplayNames["comissar"] = "Комиссары";
    m_tableDisplayNames["kategoria_godnosti"] = "Категории годности";
    m_tableDisplayNames["med_osvidetelstvovanie"] = "Медосвидетельствования";
    m_tableDisplayNames["voennyi_bilet"] = "Военные билеты";
    m_tableDisplayNames["voenno_uchetnaya_karta"] = "Военно-учётные карты";
    m_tableDisplayNames["prizivnoe_meropriyatie"] = "Призывные мероприятия";
    m_tableDisplayNames["prizivnik_comissar"] = "Связь призывник-комиссар";
    m_tableDisplayNames["prizivnik_meropriyatie"] = "Связь призывник-мероприятие";
}

void TablesWindow::setupStyles()
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
        "}"
        "QPushButton:hover { background-color: #FF69B4; }"
        "QPushButton:pressed { background-color: #FF1493; }"
    );
}

void TablesWindow::refreshTables()
{
    if (!m_dbManager || !m_dbManager->isConnected()) {
        QMessageBox::warning(this, "Ошибка", "База данных не подключена");
        return;
    }

    QStringList tables = m_dbManager->getTableList();
    refreshTableButtons();
}

void TablesWindow::refreshTableButtons()
{
    // Очистка старых кнопок
    QLayoutItem *item;
    while ((item = m_tableButtonsLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    m_buttonToTable.clear();

    if (!m_dbManager || !m_dbManager->isConnected()) {
        return;
    }

    QStringList tables = m_dbManager->getTableList();

    foreach (const QString &tableName, tables) {
        QString displayName = m_tableDisplayNames.value(tableName, tableName);
        
        QPushButton *btn = new QPushButton(displayName, this);
        btn->setContextMenuPolicy(Qt::CustomContextMenu);
        
        connect(btn, &QPushButton::clicked, [this, tableName]() {
            openTable(tableName);
        });
        
        connect(btn, &QPushButton::customContextMenuRequested, [this, btn, tableName](const QPoint &pos) {
            showTableContextMenu(btn->mapToGlobal(pos), tableName);
        });
        
        m_tableButtonsLayout->addWidget(btn);
        m_buttonToTable[btn] = tableName;
    }
}

void TablesWindow::openTable(const QString &tableName)
{
    TableViewWindow *viewWindow = new TableViewWindow(m_dbManager, tableName, this);
    viewWindow->setWindowFlags(Qt::Window);
    viewWindow->raise();
    viewWindow->activateWindow();
    viewWindow->show();
}

void TablesWindow::showTableContextMenu(const QPoint &pos, const QString &tableName)
{
    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background-color: white; border: 1px solid #ccc; }"
        "QMenu::item { padding: 5px 25px 5px 20px; color: black; }"
        "QMenu::item:selected { background-color: #E0B0FF; color: white; }"
    );

    QAction *editAction = menu.addAction("Редактировать таблицу");
    QAction *deleteAction = menu.addAction("Удалить");
    QAction *backupAction = menu.addAction("Создать резервную копию");

    QAction *selectedAction = menu.exec(pos);

    if (selectedAction == editAction) {
        editTable(tableName);
    } else if (selectedAction == deleteAction) {
        deleteTable(tableName);
    } else if (selectedAction == backupAction) {
        backupTable(tableName);
    }
}

void TablesWindow::editTable(const QString &tableName)
{
    TableEditWindow *editWindow = new TableEditWindow(m_dbManager, tableName, this);
    editWindow->setWindowFlags(Qt::Window);
    editWindow->raise();
    editWindow->activateWindow();
    editWindow->show();
}

void TablesWindow::deleteTable(const QString &tableName)
{
    int ret = QMessageBox::question(this, "Удаление",
        QString("Вы уверены, что хотите удалить таблицу %1?").arg(tableName),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        bool ok;
        m_dbManager->executeQuery(QString("DROP TABLE %1 CASCADE").arg(tableName), &ok);
        
        if (ok) {
            QMessageBox::information(this, "Успех", "Таблица удалена!");
            refreshTables();
        } else {
            QMessageBox::critical(this, "Ошибка", "Не удалось удалить таблицу:\n" + m_dbManager->lastError());
        }
    }
}

void TablesWindow::backupTable(const QString &tableName)
{
    BackupManager backupManager(m_dbManager);
    QString exportsDir = backupManager.getExportsDirectory();
    QString defaultPath = QDir(exportsDir).absoluteFilePath(QString("%1_backup.sql").arg(tableName));
    
    QString fileName = QFileDialog::getSaveFileName(this,
        "Сохранить резервную копию", defaultPath,
        "SQL Files (*.sql)");

    if (!fileName.isEmpty()) {
        if (backupManager.exportTable(tableName, fileName)) {
            QMessageBox::information(this, "Успех", "Резервная копия создана:\n" + fileName);
        } else {
            QMessageBox::critical(this, "Ошибка", "Не удалось создать резервную копию:\n" + backupManager.lastError());
        }
    }
}

void TablesWindow::goBack()
{
    if (parentWidget()) {
        parentWidget()->raise();
        parentWidget()->activateWindow();
    }
    hide();
}

