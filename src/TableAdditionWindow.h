#ifndef TABLEADDITIONWINDOW_H
#define TABLEADDITIONWINDOW_H

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QVBoxLayout>
#include <QScrollArea>
#include "DatabaseManager.h"

class TableAdditionWindow : public QWidget
{
    Q_OBJECT

public:
    explicit TableAdditionWindow(DatabaseManager *dbManager, QWidget *parent = nullptr);
    ~TableAdditionWindow();

private slots:
    void addColumn();
    void removeColumn(QWidget *columnWidget);
    void createTable();
    void goBack();

private:
    void setupUI();
    void setupStyles();
    QWidget* createColumnWidget();

    DatabaseManager *m_dbManager;
    QLineEdit *m_tableNameInput;
    QVBoxLayout *m_columnsLayout;
    QList<QWidget*> m_columnWidgets;
    QScrollArea *m_scrollArea;
};

#endif // TABLEADDITIONWINDOW_H

