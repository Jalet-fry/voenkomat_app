#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include "DatabaseManager.h"
#include "TablesWindow.h"
#include "QueriesWindow.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    // Операции классического режима (Лаб 2)
    void viewData();
    void addRecord();
    void updateRecord();
    void deleteRecord();
    void openQueries();
    void saveQueryResult();
    void createBackup();
    void exitApp();

    void applyFilter();
    void onTableSelected(const QString &tableName);
    void switchMode();

    // Современный графический режим
    void openTablesWindow();
    void openQueriesWindow();
    void updateConnectionStatus();
    bool connectToDatabase();
    void exportAllData();
    void restoreFromBackup();
    void restoreTableFromBackup();

private:
    void setupUI();
    void setupStyles();
    void setupClassicUI();
    void setupMenus();
    void setupShortcuts();
    void loadTablesMenu();

    DatabaseManager *m_dbManager;
    QString m_activeTable;

    // Виджеты современного интерфейса
    QWidget *m_centralWidget;
    QVBoxLayout *m_layout;
    QLabel *m_titleLabel;
    QLabel *m_statusLabel;
    QPushButton *m_queriesBtn;
    QPushButton *m_tablesBtn;
    QPushButton *m_exportBtn;
    QPushButton *m_restoreBtn;
    QPushButton *m_restoreTableBtn;
    QPushButton *m_exitBtn;

    // Виджеты классического режима (Лаб 2)
    QTableWidget *m_mainTable;
    QComboBox *m_filterColumnCombo;
    QLineEdit *m_filterValueEdit;
    QPushButton *m_filterBtn;
    bool m_classicMode;

    TablesWindow *m_tablesWindow;
    QueriesWindow *m_queriesWindow;
};

#endif // MAINWINDOW_H
