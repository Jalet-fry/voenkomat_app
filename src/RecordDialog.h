#ifndef RECORDDIALOG_H
#define RECORDDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QMap>
#include <QDialogButtonBox>
#include <QCompleter>
#include "DatabaseManager.h"

class RecordDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RecordDialog(DatabaseManager *dbManager, const QString &tableName, QWidget *parent = nullptr, const QString &recordId = "");
    ~RecordDialog();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void saveRecord();

private:
    void setupUI();
    void setupStyles();
    void loadRecordData();
    void setupAutocomplete();
    void setupForeignKeyFields();
    QString getDisplayName(const QString &fieldName) const;

    DatabaseManager *m_dbManager;
    QString m_tableName;
    QString m_recordId;
    bool m_isClassicUI;

    QStringList m_columns;
    QMap<QString, QWidget*> m_fieldWidgets; // QLineEdit или QComboBox
    QMap<QString, QString> m_fieldDisplayNames;

    QVBoxLayout *m_layout;
    QDialogButtonBox *m_buttonBox;
};

#endif // RECORDDIALOG_H
