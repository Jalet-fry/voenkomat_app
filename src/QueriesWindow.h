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
#include <QList>
#include <QMenuBar>
#include "DatabaseManager.h"

struct QueryInfo {
    QString number;
    QString description;
    QString type;
    QString sqlText;
};

class QueriesWindow : public QWidget
{
    Q_OBJECT

public:
    explicit QueriesWindow(DatabaseManager *dbManager, QWidget *parent = nullptr);
    ~QueriesWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void loadQueries();
    void runSelectedQuery();
    void goBack();
    void onFilterChanged();

private:
    void setupUI();
    void setupModernUI();
    void setupClassicUI();
    void setupStyles();
    void refreshTable();

    DatabaseManager *m_dbManager;
    bool m_isClassicUI;

    QVBoxLayout *m_layout;
    QTableWidget *m_table;
    QComboBox *m_typeFilter = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QMenuBar *m_menuBar = nullptr;
    QLabel *m_footerHint = nullptr;

    QList<QueryInfo> m_allQueries;
};

#endif // QUERIESWINDOW_H
