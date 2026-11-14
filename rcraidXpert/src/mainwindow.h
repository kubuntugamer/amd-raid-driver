#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeView>
#include <QStandardItemModel>
#include <QPushButton>
#include <QTabWidget>
#include <QIcon>
#include <QTableView>
#include <QStringList>
#include "modules/disktablewidget.h"
#include "modules/arrayoptionswidget.h"
#include "modules/statuspanelwidget.h"
#include "modules/dashboardwidget.h"
#include "modules/taskmanagerwidget.h"
#include "modules/arraydatacontroller.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    /* Text of every tree row, for the --dump-tree smoke check. */
    QStringList treeDump() const;

private slots:
    void triggerControllerRescan();
    void handleCreateArray();
    void handleDeleteArray();

private:
    void setupLayout();
    void loadStyles();
    void buildTreeFromInventory();
    void updateCapacityDisplay();
    void connectActionPipelines();
    void reportFailure(const QString &title, const QString &text);

    QIcon createProceduralHostIcon();
    QIcon createProceduralHbaIcon();
    QIcon createProceduralDriveIcon();

    ArrayDataController *controller;
    QTreeView *arrayTreeView;
    QStandardItemModel *treeModel;
    QTabWidget *mainTabs;

    DiskTableWidget *diskTable;
    ArrayOptionsWidget *arrayOptions;
    StatusPanelWidget *statusPanel;
    DashboardWidget *dashboardTab;
    TaskManagerWidget *tasksTab;

    QPushButton *commitButton;
    QPushButton *cancelButton;
};

#endif
