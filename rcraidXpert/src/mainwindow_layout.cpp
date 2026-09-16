#include "mainwindow.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QLabel>
#include <QHeaderView>
#include <QMenuBar>

#include "modules/disktablewidget.h"
#include "modules/arrayoptionswidget.h"
#include "modules/statuspanelwidget.h"
#include "modules/dashboardwidget.h"
#include "modules/taskmanagerwidget.h"

void MainWindow::setupLayout() {
    QMenuBar *mainMenuBar = menuBar();
    mainMenuBar->clear();
    
    QMenu *controllerMenu    = mainMenuBar->addMenu("&Controller");
    QMenu *arrayMenu         = mainMenuBar->addMenu("&Array");
    QMenu *logicalDriveMenu  = mainMenuBar->addMenu("&Logical Drive");
    QMenu *physicalDriveMenu = mainMenuBar->addMenu("&Physical Drive");
    QMenu *viewMenu          = mainMenuBar->addMenu("&View");
    QMenu *helpMenu          = mainMenuBar->addMenu("&Help");

    QAction *rescanAction = controllerMenu->addAction("Rescan Controller");
    connect(rescanAction, &QAction::triggered, this, &MainWindow::triggerControllerRescan);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout *rootLayout = new QVBoxLayout(centralWidget);
    rootLayout->setContentsMargins(4, 4, 4, 4);

    QSplitter *masterVerticalSplitter = new QSplitter(Qt::Vertical, this);
    rootLayout->addWidget(masterVerticalSplitter);

    QSplitter *upperHorizontalSplitter = new QSplitter(Qt::Horizontal, masterVerticalSplitter);

    QWidget *leftTreeContainer = new QWidget(this);
    QVBoxLayout *treeLayout = new QVBoxLayout(leftTreeContainer);
    treeLayout->setContentsMargins(0, 0, 0, 0);

    arrayTreeView = new QTreeView(this);
    treeModel = new QStandardItemModel(this);
    treeModel->setHorizontalHeaderLabels({"AMD RAIDXpert2 Managed Tree Topology"});
    arrayTreeView->setModel(treeModel);
    arrayTreeView->header()->setVisible(true);
    
    // Pristine slate initialization: mock folders are completely removed
    treeModel->clear();
    treeModel->setHorizontalHeaderLabels({"AMD RAIDXpert2 Managed Tree Topology"});

    treeLayout->addWidget(arrayTreeView);
    upperHorizontalSplitter->addWidget(leftTreeContainer);

    mainTabs = new QTabWidget(this);
    
    QWidget *configurationTab = new QWidget(this);
    QVBoxLayout *configLayout = new QVBoxLayout(configurationTab);
    configLayout->setContentsMargins(2, 2, 2, 2);

    arrayOptions = new ArrayOptionsWidget(this);
    arrayOptions->raidTypeCombo->clear();
    arrayOptions->raidTypeCombo->addItems({
        "RAID 0 (Striping)", 
        "RAID 1 (Mirroring)", 
        "RAID 10 (Striped Mirrors)",
        "RAID 5 (Single Parity)", 
        "RAID 6 (Dual Parity)"
    });
    
    diskTable = new DiskTableWidget(this);
    configLayout->addWidget(arrayOptions, 1);
    configLayout->addWidget(diskTable, 3);
    
    mainTabs->addTab(configurationTab, "Array Creation & Properties Profiles");

    dashboardTab = new DashboardWidget(this);
    mainTabs->addTab(dashboardTab, "System Controller Dashboard");

    tasksTab = new TaskManagerWidget(this);
    mainTabs->addTab(tasksTab, "Logical Task Manager");

    upperHorizontalSplitter->addWidget(mainTabs);
    upperHorizontalSplitter->setStretchFactor(0, 1);
    upperHorizontalSplitter->setStretchFactor(1, 3);

    QWidget *bottomLogContainer = new QWidget(this);
    QVBoxLayout *bottomLayout = new QVBoxLayout(bottomLogContainer);
    bottomLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *logTitleLabel = new QLabel("System Events Monitor Logging Interface Plane:", this);
    logTitleLabel->setStyleSheet("font-weight: bold; background-color: #2b2b2b; color: #00FF00; padding: 4px; font-family: 'Monospace';");
    bottomLayout->addWidget(logTitleLabel);

    statusPanel = new StatusPanelWidget(this);
    bottomLayout->addWidget(statusPanel);

    QHBoxLayout *actionFooterLayout = new QHBoxLayout();
    enableWritesCheck = new QCheckBox("Opt-In Write Access Mode (enable_writes=1)", this);
    commitButton = new QPushButton("Commit Configuration Vector", this);
    commitButton->setStyleSheet("font-weight: bold;");
    cancelButton = new QPushButton("Discard Changes", this);

    actionFooterLayout->addWidget(enableWritesCheck);
    actionFooterLayout->addStretch();
    actionFooterLayout->addWidget(commitButton);
    actionFooterLayout->addWidget(cancelButton);
    bottomLayout->addLayout(actionFooterLayout);

    masterVerticalSplitter->addWidget(upperHorizontalSplitter);
    masterVerticalSplitter->addWidget(bottomLogContainer);
    masterVerticalSplitter->setStretchFactor(0, 3);
    masterVerticalSplitter->setStretchFactor(1, 1);
}
