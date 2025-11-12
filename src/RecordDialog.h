#ifndef RECORDDIALOG_H
#define RECORDDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QMap>
#include "DatabaseManager.h"

class RecordDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RecordDialog(DatabaseManager *dbManager, const QString &tableName, QWidget *parent = nullptr, int recordId = -1);
    ~RecordDialog();

private slots:
    void saveRecord();

private:
    void setupUI();
    void loadRecordData();
    QString getDisplayName(const QString &fieldName) const;
    bool validateRecord(QString &errorMessage);
    QVariant formatValueForSQL(const QString &columnName, const QString &value, const QString &dataType);
    bool isValidDate(const QString &dateStr);
    bool isValidInteger(const QString &value);
    bool isValidTimestamp(const QString &timestampStr);

    DatabaseManager *m_dbManager;
    QString m_tableName;
    int m_recordId;
    QStringList m_columns;
    QMap<QString, QLineEdit*> m_fields;
    QMap<QString, QString> m_fieldDisplayNames;
    QList<DatabaseManager::ColumnDetail> m_columnDetails;
    QList<DatabaseManager::ForeignKeyInfo> m_foreignKeys;
};

#endif // RECORDDIALOG_H

