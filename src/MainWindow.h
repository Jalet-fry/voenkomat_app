#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QListWidget>
#include "DatabaseManager.h"

class TablesWindow;
class QueriesWindow;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    // Общие действия
    void openTablesWindow();
    void openQueriesWindow();
    void exportAllData();
    void restoreFromBackup();
    void updateConnectionStatus();
    void exitApp();
    void showHelp();
    void switchMode();

    // Операции CUA (Operations)
    void addRecord();
    void viewActiveTable();
    void deleteRecord();
    void updateRecord();
    void openQueries();
    void saveQueryResult();
    void createBackup();

    // Служебные
    void applyFilter();
    void onTableSelected(const QString &tableName);

private:
    void setupUI();
    void setupModernUI();
    void setupClassicUI();
    void setupStyles();
    bool connectToDatabase();
    void clearLayout(QLayout *layout);
    void refreshTablesMenu();

    void showHighContrastHelp(const QString &title, const QString &content);

    DatabaseManager *m_dbManager;
    bool m_isClassicUI;
    QString m_activeTable;

    QWidget *m_centralWidget = nullptr;
    QVBoxLayout *m_layout = nullptr;
    QLabel *m_statusLabel = nullptr;

    // Classic UI Elements
    QMenuBar *m_menuBar = nullptr;
    QMenu *m_tablesMenu = nullptr;
    QComboBox *m_filterColumnCombo = nullptr;
    QLineEdit *m_filterValueEdit = nullptr;
    QPushButton *m_applyFilterBtn = nullptr;
    QTableWidget *m_mainTable = nullptr;
    QLabel *m_activeTableLabel = nullptr;
    QLabel *m_classicFooter = nullptr;

    // Modern UI Elements
    QLabel *m_titleLabel = nullptr;
    QPushButton *m_tablesBtn = nullptr;
    QPushButton *m_queriesBtn = nullptr;
    QPushButton *m_exportBtn = nullptr;
    QPushButton *m_helpBtn = nullptr;
    QPushButton *m_switchModeBtn = nullptr;
    QPushButton *m_exitBtn = nullptr;

    TablesWindow *m_tablesWindow = nullptr;
    QueriesWindow *m_queriesWindow = nullptr;
};

#endif // MAINWINDOW_H
