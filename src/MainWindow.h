#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
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
    // Основные действия
    void openTablesWindow();
    void openQueriesWindow();
    void exportAllData();
    void restoreFromBackup();
    void updateConnectionStatus();
    void exitApp();
    void showHelp();
    void switchMode();

    // CUA Операции (привязаны к выбранной таблице)
    void viewActiveTable();
    void addRecordToActive();
    void deleteRecordFromActive();
    void updateRecordInActive();
    void runSpecialQuery();
    void createSystemBackup();
    void saveLastQueryResult();

    void onTableSelectionChanged();

private:
    void setupUI();
    void setupModernUI();
    void setupClassicUI();
    void setupStyles();
    bool connectToDatabase();
    void clearLayout(QLayout *layout);

    void showHighContrastHelp(const QString &title, const QString &content);

    DatabaseManager *m_dbManager;
    bool m_isClassicUI;
    QString m_activeTable;

    QWidget *m_centralWidget = nullptr;
    QVBoxLayout *m_layout = nullptr;
    QLabel *m_statusLabel = nullptr;

    // Modern UI
    QLabel *m_titleLabel = nullptr;
    QPushButton *m_tablesBtn = nullptr;
    QPushButton *m_queriesBtn = nullptr;
    QPushButton *m_exportBtn = nullptr;
    QPushButton *m_exitBtn = nullptr;
    QPushButton *m_switchModeBtn = nullptr;
    QPushButton *m_helpBtn = nullptr;

    // Classic UI
    QMenuBar *m_menuBar = nullptr;
    QListWidget *m_tableListWidget = nullptr;
    QLabel *m_activeTableLabel = nullptr;
    QLabel *m_classicFooter = nullptr;

    TablesWindow *m_tablesWindow = nullptr;
    QueriesWindow *m_queriesWindow = nullptr;
};

#endif // MAINWINDOW_H
