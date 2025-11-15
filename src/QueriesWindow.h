#ifndef QUERIESWINDOW_H
#define QUERIESWINDOW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTableWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QMap>
#include <QList>
#include <QModelIndex>
#include "DatabaseManager.h"

class QueryResultWindow;

struct QueryInfo {
    QString number;      // "5.1", "6.1", etc.
    QString description; // Текстовое описание
    QString type;        // "Lab5" или "Lab6"
    QString filePath;    // Полный путь к файлу
    QString sqlText;     // SQL текст запроса
};

class QueriesWindow : public QWidget
{
    Q_OBJECT

public:
    explicit QueriesWindow(DatabaseManager *dbManager, QWidget *parent = nullptr);
    ~QueriesWindow();

private slots:
    void loadQueries();
    void runQuery(const QueryInfo &queryInfo);
    void addNewQuery();
    void showQueryContextMenu(const QPoint &pos);
    void deleteQuery(const QueryInfo &queryInfo);
    void backupQuery(const QueryInfo &queryInfo);
    void goBack();
    void onTableDoubleClicked(const QModelIndex &index);
    void onFilterChanged();
    void onSearchTextChanged(const QString &text);

private:
    void setupUI();
    void setupStyles();
    void refreshTable();
    void populateTable();
    QString parseQueryDescription(const QString &queryText) const;
    QString findResourcesDirectory() const;
    QueryInfo getQueryInfoFromRow(int row) const;
    void loadQueriesFromFolder(const QString &folderPath, const QString &type);

    DatabaseManager *m_dbManager;
    QVBoxLayout *m_layout;
    QHBoxLayout *m_filterLayout;
    QTableWidget *m_table;
    QComboBox *m_typeFilter;
    QLineEdit *m_searchEdit;
    QList<QueryInfo> m_allQueries; // Все загруженные запросы
    QList<QueryInfo> m_filteredQueries; // Отфильтрованные запросы для отображения
};

#endif // QUERIESWINDOW_H

