#ifndef QUERIESWINDOW_H
#define QUERIESWINDOW_H

#include <QWidget>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMap>
#include "DatabaseManager.h"

class QueryResultWindow;

class QueriesWindow : public QWidget
{
    Q_OBJECT

public:
    explicit QueriesWindow(DatabaseManager *dbManager, QWidget *parent = nullptr);
    ~QueriesWindow();

private slots:
    void loadQueries();
    void runQuery(int queryNumber);
    void addNewQuery();
    void showQueryContextMenu(const QPoint &pos, int queryNumber);
    void deleteQuery(int queryNumber);
    void backupQuery(int queryNumber);
    void goBack();

private:
    void setupUI();
    void setupStyles();
    void refreshQueryButtons();
    QString parseQueryTitle(const QString &queryText) const;
    QString getQueryFilePath(int queryNumber) const;
    QString findResourcesDirectory() const;

    DatabaseManager *m_dbManager;
    QVBoxLayout *m_layout;
    QGridLayout *m_gridLayout;
    QMap<int, QString> m_queries; // номер запроса -> SQL текст
    QMap<int, QString> m_queryTitles; // номер запроса -> название
};

#endif // QUERIESWINDOW_H

