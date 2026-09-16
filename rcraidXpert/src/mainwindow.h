#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeView>
#include <QStandardItemModel>
#include <QPushButton>
#include <QTabWidget>
#include <QCheckBox>
#include <QIcon>
#include <QTableView>
#include "modules/disktablewidget.h"
#include "modules/arrayoptionswidget.h"
#include "modules/statuspanelwidget.h"
#include "modules/dashboardwidget.h"
#include "modules/taskmanagerwidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void triggerControllerRescan();

private:
    void setupLayout();
    void loadStyles();
    void updateCapacityDisplay();
    void addNewArrayToTree();
    void initializeSimulationTimers();
    void connectActionPipelines();
    void executeConfigurationPurge();
    void validateAndCommitArray();

    QIcon createProceduralHostIcon();
    QIcon createProceduralHbaIcon();
    QIcon createProceduralDriveIcon();

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
    QCheckBox *enableWritesCheck;
    int arrayCounter;
};

#endif
