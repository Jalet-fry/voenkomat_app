#ifndef TABLEVIEWWINDOW_H
#define TABLEVIEWWINDOW_H

#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QMenu>
#include <QScrollArea>
#include <QLabel>
#include <QMap>
#include <QList>
#include <QVariant>
#include <QTimer>
#include "DatabaseManager.h"

class TableViewWindow : public QWidget
{
    Q_OBJECT

public:
    explicit TableViewWindow(DatabaseManager *dbManager, const QString &tableName, QWidget *parent = nullptr);
    ~TableViewWindow();

private slots:
    void loadData(const QString &filterClause = "", const QList<QVariant> &filterParams = QList<QVariant>());
    void addRecord();
    void editRecord();
    void deleteRecord();
    void showContextMenu(const QPoint &pos);
    void goBack();
    void exportToCSV();
    void exportToXlsx();
    void onFilterChanged();
    void applyFilters();

private:
    void setupUI();
    void setupStyles();
    void setupFilters();
    void updateButtonStates();
    QString getDisplayName(const QString &fieldName) const;
    void buildFilterQuery(QString &whereClause, QList<QVariant> &params);
    void clearFilters();
    QString escapeLikePattern(const QString &text) const;

    DatabaseManager *m_dbManager;
    QString m_tableName;
    QTableWidget *m_table;
    QVBoxLayout *m_layout;
    QMap<QString, QString> m_fieldDisplayNames;
    QPushButton *m_editBtn;
    QPushButton *m_deleteBtn;
    
    // Фильтрация
    QScrollArea *m_filterScrollArea;
    QWidget *m_filterWidget;
    QGridLayout *m_filterLayout;
    QMap<QString, QWidget*> m_filterWidgets;
    QList<DatabaseManager::ColumnDetail> m_columnDetails;
    QPushButton *m_clearFiltersBtn;
    QTimer *m_filterTimer;
    QLabel *m_filterStatusLabel;
};

#endif // TABLEVIEWWINDOW_H

