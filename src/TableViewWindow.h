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
#include <QMenuBar>
#include "DatabaseManager.h"

class TableViewWindow : public QWidget
{
    Q_OBJECT

public:
    explicit TableViewWindow(DatabaseManager *dbManager, const QString &tableName, QWidget *parent = nullptr);
    ~TableViewWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void loadData(const QString &filterClause = "");
    void addRecord();
    void editRecord();
    void deleteRecord();
    void goBack();
    void exportToXlsx();
    void onFilterChanged();
    void applyFilters();
    void showFiltersDialog(); // Для Classic режима

private:
    void setupUI();
    void setupModernUI();
    void setupClassicUI();
    void setupStyles();
    void updateButtonStates();
    QString getDisplayName(const QString &fieldName) const;

    DatabaseManager *m_dbManager;
    QString m_tableName;
    bool m_isClassicUI;

    QVBoxLayout *m_layout;
    QTableWidget *m_table;
    QLabel *m_statusLabel;
    
    // Modern UI
    QScrollArea *m_filterScrollArea = nullptr;
    QPushButton *m_addBtn = nullptr;
    QPushButton *m_editBtn = nullptr;
    QPushButton *m_deleteBtn = nullptr;

    // Classic UI
    QMenuBar *m_menuBar = nullptr;
    QLabel *m_footerHint = nullptr;

    QMap<QString, QWidget*> m_filterWidgets;
    QTimer *m_filterTimer;
    QMap<QString, QString> m_fieldDisplayNames;
};

#endif // TABLEVIEWWINDOW_H
