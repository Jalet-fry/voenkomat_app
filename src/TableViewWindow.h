#ifndef TABLEVIEWWINDOW_H
#define TABLEVIEWWINDOW_H

#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMenu>
#include "DatabaseManager.h"

class TableViewWindow : public QWidget
{
    Q_OBJECT

public:
    explicit TableViewWindow(DatabaseManager *dbManager, const QString &tableName, QWidget *parent = nullptr);
    ~TableViewWindow();

private slots:
    void loadData();
    void addRecord();
    void editRecord();
    void deleteRecord();
    void showContextMenu(const QPoint &pos);
    void goBack();
    void exportToCSV();
    void exportToXlsx();

private:
    void setupUI();
    void setupStyles();
    QString getDisplayName(const QString &fieldName) const;

    DatabaseManager *m_dbManager;
    QString m_tableName;
    QTableWidget *m_table;
    QVBoxLayout *m_layout;
    QMap<QString, QString> m_fieldDisplayNames;
};

#endif // TABLEVIEWWINDOW_H

