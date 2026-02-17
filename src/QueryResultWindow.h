#ifndef QUERYRESULTWINDOW_H
#define QUERYRESULTWINDOW_H

#include <QDialog>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QStringList>
#include <QMap>

class DatabaseManager;

class QueryResultWindow : public QDialog
{
    Q_OBJECT

public:
    explicit QueryResultWindow(const QString &title,
                              const QStringList &columnNames,
                              const QList<QList<QVariant>> &rows,
                              DatabaseManager *dbManager = nullptr,
                              QWidget *parent = nullptr);
    ~QueryResultWindow();

private slots:
    void goBack();
    void exportToCSV();
    void exportToXlsx();

private:
    void setupUI();
    void setupStyles();
    QString getDisplayName(const QString &fieldName) const;

    QString m_title;
    QStringList m_columnNames;
    QList<QList<QVariant>> m_rows;
    QTableWidget *m_table;
    QMap<QString, QString> m_fieldDisplayNames;
    DatabaseManager *m_dbManager;
};

#endif // QUERYRESULTWINDOW_H
