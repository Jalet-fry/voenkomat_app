#ifndef RECORDDIALOG_H
#define RECORDDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QMap>
#include <QDialogButtonBox>
#include "DatabaseManager.h"

class RecordDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RecordDialog(DatabaseManager *dbManager, const QString &tableName, QWidget *parent = nullptr, int recordId = -1);
    ~RecordDialog();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void saveRecord();

private:
    void setupUI();
    void setupModernUI();
    void setupClassicUI();
    void setupStyles();
    void loadRecordData();
    QString getDisplayName(const QString &fieldName) const;

    DatabaseManager *m_dbManager;
    QString m_tableName;
    int m_recordId;
    bool m_isClassicUI;

    QStringList m_columns;
    QMap<QString, QLineEdit*> m_fields;
    QMap<QString, QString> m_fieldDisplayNames;
    QList<DatabaseManager::ForeignKeyInfo> m_foreignKeys;

    QVBoxLayout *m_layout;
    QDialogButtonBox *m_buttonBox;
};

#endif // RECORDDIALOG_H
