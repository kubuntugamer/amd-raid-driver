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
#include "modules/smartmonitorwidget.h"

void MainWindow::setupLayout() {
    setWindowFlags(Qt::Window | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);
    setFixedSize(1280, 720);

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
    rootLayout->setContentsMargins(6, 6, 6, 6);

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
    
    treeModel->clear();
    treeModel->setHorizontalHeaderLabels({"AMD RAIDXpert2 Managed Tree Topology"});

    treeLayout->addWidget(arrayTreeView);
    upperHorizontalSplitter->addWidget(leftTreeContainer);

    mainTabs = new QTabWidget(this);
    
    QWidget *configurationTab = new QWidget(this);
    QVBoxLayout *configLayout = new QVBoxLayout(configurationTab);
    configLayout->setContentsMargins(4, 4, 4, 4);

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
    
    mainTabs->addTab(configurationTab, "Array Creation & Properties");

    dashboardTab = new DashboardWidget(this);
    mainTabs->addTab(dashboardTab, "System Controller Dashboard");

    tasksTab = new TaskManagerWidget(this);
    mainTabs->addTab(tasksTab, "Logical Task Manager");

    // Seamlessly embed the formatted S.M.A.R.T telemetry matrix panel tab
    SmartMonitorWidget *smartTab = new SmartMonitorWidget(this);
    mainTabs->addTab(smartTab, "S.M.A.R.T. Telemetry Logs");

    upperHorizontalSplitter->addWidget(mainTabs);
    upperHorizontalSplitter->setStretchFactor(0, 1);
    upperHorizontalSplitter->setStretchFactor(1, 3);

    QWidget *bottomLogContainer = new QWidget(this);
    QVBoxLayout *bottomLayout = new QVBoxLayout(bottomLogContainer);
    bottomLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *logTitleLabel = new QLabel("System Events Monitor Console Logging Panel", this);
    logTitleLabel->setStyleSheet("font-weight: bold; padding: 4px 6px; border-bottom: 1px solid rgba(128,128,128,0.3);");
    bottomLayout->addWidget(logTitleLabel);

    statusPanel = new StatusPanelWidget(this);
    bottomLayout->addWidget(statusPanel);

    QHBoxLayout *actionFooterLayout = new QHBoxLayout();
    enableWritesCheck = new QCheckBox("Opt-In Write Access Mode (enable_writes=1)", this);
    commitButton = new QPushButton("Commit Configuration Vector", this);
    commitButton->setStyleSheet("font-weight: bold; padding: 4px 12px;");
    cancelButton = new QPushButton("Discard Changes", this);

    // Map "Discard Changes" to flush selections and poll real hardware state clean
    connect(cancelButton, &QPushButton::clicked, this, &MainWindow::triggerControllerRescan);

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
