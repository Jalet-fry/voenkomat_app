#ifndef CREATETABLEDIALOG_H
#define CREATETABLEDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QMessageBox>
#include <QHeaderView>

class CreateTableDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreateTableDialog(QWidget *parent = nullptr);
    ~CreateTableDialog();

    QString getTableName() const;
    QList<QPair<QString, QString>> getColumns() const;
    QStringList getPrimaryKeys() const;

private slots:
    void addColumn();
    void removeColumn();
    void onOkClicked();
    void onCancelClicked();

private:
    void setupUI();
    void setupStyles();
    QString getDataTypeString(int dataTypeIndex, const QString &length = "");

    QLineEdit *m_tableNameEdit;
    QTableWidget *m_columnsTable;
    QPushButton *m_addColumnBtn;
    QPushButton *m_removeColumnBtn;
    QPushButton *m_okBtn;
    QPushButton *m_cancelBtn;
    
    QStringList m_dataTypes;
};

#endif // CREATETABLEDIALOG_H

