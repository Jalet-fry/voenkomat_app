#ifndef TABLEEDITWINDOW_H
#define TABLEEDITWINDOW_H

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QVBoxLayout>
#include <QScrollArea>
#include "DatabaseManager.h"

class TableEditWindow : public QWidget
{
    Q_OBJECT

public:
    explicit TableEditWindow(DatabaseManager *dbManager, const QString &tableName, QWidget *parent = nullptr);
    ~TableEditWindow();

private slots:
    void addColumn();
    void removeColumn(QWidget *columnWidget);
    void saveChanges();
    void goBack();

private:
    void setupUI();
    void setupStyles();
    void loadTableColumns();
    QWidget* createColumnWidget(const QString &columnName = "", const QString &columnType = "varchar(255)");

    DatabaseManager *m_dbManager;
    QString m_originalTableName;
    QString m_tableName;
    QLineEdit *m_tableNameInput;
    QVBoxLayout *m_columnsLayout;
    QList<QWidget*> m_columnWidgets;
    QScrollArea *m_scrollArea;
};

#endif // TABLEEDITWINDOW_H

