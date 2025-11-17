#ifndef TABLEEDITWINDOW_H
#define TABLEEDITWINDOW_H

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QDialog>
#include <QGroupBox>
#include <QMap>
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
    void addForeignKey();
    void removeForeignKey(QWidget *fkWidget);
    void saveChanges();
    void goBack();
    void onReferencedTableChanged();

private:
    void setupUI();
    void setupStyles();
    void loadTableColumns();
    void loadForeignKeys();
    QWidget* createColumnWidget(const QString &columnName = "", const QString &columnType = "varchar(255)");
    QWidget* createForeignKeyWidget(const DatabaseManager::ForeignKeyInfo &fkInfo);
    QDialog* createAddForeignKeyDialog();

    DatabaseManager *m_dbManager;
    QString m_originalTableName;
    QString m_tableName;
    QLineEdit *m_tableNameInput;
    QVBoxLayout *m_columnsLayout;
    QList<QWidget*> m_columnWidgets;
    QScrollArea *m_scrollArea;
    
    // Foreign keys
    QGroupBox *m_foreignKeysGroup;
    QScrollArea *m_fkScrollArea;
    QVBoxLayout *m_foreignKeysLayout;
    QList<QWidget*> m_foreignKeyWidgets;
    QList<DatabaseManager::ForeignKeyInfo> m_currentForeignKeys;
};

#endif // TABLEEDITWINDOW_H

