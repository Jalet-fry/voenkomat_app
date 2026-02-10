#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QMap>
#include "DatabaseManager.h"
#include "ConfigManager.h"

class TablesWindow;
class QueriesWindow;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void openQueriesWindow();
    void openTablesWindow();
    void exportAllData();
    void restoreFromBackup();
    void restoreTableFromBackup();

private:
    void setupUI();
    void setupStyles();
    void updateConnectionStatus();
    bool connectToDatabase();

    DatabaseManager *m_dbManager;
    QWidget *m_centralWidget;
    QVBoxLayout *m_layout;
    
    QLabel *m_titleLabel;
    QLabel *m_statusLabel; // Статус подключения к БД
    QPushButton *m_queriesBtn;
    QPushButton *m_tablesBtn;
    QPushButton *m_exportBtn;
    QPushButton *m_restoreBtn;
    QPushButton *m_restoreTableBtn;
    QPushButton *m_exitBtn;

    TablesWindow *m_tablesWindow;
    QueriesWindow *m_queriesWindow;
};

#endif // MAINWINDOW_H

