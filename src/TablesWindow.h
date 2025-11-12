#ifndef TABLESWINDOW_H
#define TABLESWINDOW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QMenu>
#include "DatabaseManager.h"

class TableViewWindow;
class TableEditWindow;

class TablesWindow : public QWidget
{
    Q_OBJECT

public:
    explicit TablesWindow(DatabaseManager *dbManager, QWidget *parent = nullptr);
    ~TablesWindow();

private slots:
    void refreshTables();
    void openTable(const QString &tableName);
    void showTableContextMenu(const QPoint &pos, const QString &tableName);
    void editTable(const QString &tableName);
    void deleteTable(const QString &tableName);
    void backupTable(const QString &tableName);
    void goBack();

private:
    void setupUI();
    void setupStyles();
    void refreshTableButtons();

    DatabaseManager *m_dbManager;
    QVBoxLayout *m_layout;
    QScrollArea *m_scrollArea;
    QWidget *m_scrollContent;
    QVBoxLayout *m_tableButtonsLayout;
    
    QMap<QString, QString> m_tableDisplayNames;
    QMap<QPushButton*, QString> m_buttonToTable;
};

#endif // TABLESWINDOW_H

