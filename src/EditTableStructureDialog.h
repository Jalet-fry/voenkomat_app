// TODO: [REVIEW] OK.
#ifndef EDITTABLESTRUCTUREDIALOG_H
#define EDITTABLESTRUCTUREDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTableWidget>
#include <QComboBox>
#include <QLabel>
#include <QMessageBox>
#include <QHeaderView>
#include "DatabaseManager.h"

class EditTableStructureDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditTableStructureDialog(DatabaseManager *dbManager, const QString &tableName, QWidget *parent = nullptr);
    ~EditTableStructureDialog();

private slots:
    void addColumn();
    void removeColumn();
    void onOkClicked();
    void onCancelClicked();

private:
    void setupUI();
    void setupStyles();
    void loadExistingColumns();
    QString getDataTypeString(int dataTypeIndex, const QString &length = "");

    DatabaseManager *m_dbManager;
    QString m_tableName;
    QTableWidget *m_columnsTable;
    QPushButton *m_addColumnBtn;
    QPushButton *m_removeColumnBtn;
    QPushButton *m_okBtn;
    QPushButton *m_cancelBtn;
    
    QStringList m_dataTypes;
    QStringList m_existingColumns; // Для отслеживания существующих колонок
};

#endif // EDITTABLESTRUCTUREDIALOG_H
