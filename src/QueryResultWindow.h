#ifndef QUERYRESULTWINDOW_H
#define QUERYRESULTWINDOW_H

#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QStringList>

class QueryResultWindow : public QWidget
{
    Q_OBJECT

public:
    explicit QueryResultWindow(const QString &title,
                              const QStringList &columnNames,
                              const QList<QList<QVariant>> &rows,
                              QWidget *parent = nullptr);
    ~QueryResultWindow();

private slots:
    void goBack();
    void exportToCSV();

private:
    void setupUI();
    void setupStyles();
    QString getDisplayName(const QString &fieldName) const;

    QString m_title;
    QStringList m_columnNames;
    QList<QList<QVariant>> m_rows;
    QTableWidget *m_table;
    QMap<QString, QString> m_fieldDisplayNames;
};

#endif // QUERYRESULTWINDOW_H

