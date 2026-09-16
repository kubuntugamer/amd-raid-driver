#include "taskmanagerwidget.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QProgressBar>

TaskManagerWidget::TaskManagerWidget(QWidget *parent) : QWidget(parent), taskCounter(0) {
    setupUiLayout();
}

TaskManagerWidget::~TaskManagerWidget() {}

void TaskManagerWidget::setupUiLayout() {
    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    
    QLabel *titleLabel = new QLabel("Active Background Storage Tasks", this);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 13px;");
    rootLayout->addWidget(titleLabel);

    taskTable = new QTableWidget(this);
    taskTable->setColumnCount(5);
    taskTable->setHorizontalHeaderLabels({"Task ID / Type", "Target Volume", "Progress Matrix", "Processed Sectors", "Calculated ETA"});
    taskTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    taskTable->setAlternatingRowColors(true);
    
    rootLayout->addWidget(taskTable);
}

/**
 * [AI_FUNC_MAP]: TaskManagerWidget::registerNewSyncTask
 * - Objective: Registers an un-simulated hardware task block at a static 0% baseline state.
 */
void TaskManagerWidget::registerNewSyncTask(const QString &volumeName, int diskCount, int totalCapacityMb) {
    taskCounter++;
    int currentRow = taskTable->rowCount();
    taskTable->insertRow(currentRow);

    // Column 0: Task Signature Identification
    taskTable->setItem(currentRow, 0, new QTableWidgetItem(QString("TASK_0%1 (Pending Sync)").arg(taskCounter)));

    // Column 1: Associated Target Identity
    taskTable->setItem(currentRow, 1, new QTableWidgetItem(volumeName));

    // Column 2: Static Progress Bar Node
    QProgressBar *bar = new QProgressBar(this);
    bar->setRange(0, 100);
    bar->setValue(0);
    bar->setTextVisible(true);
    taskTable->setCellWidget(currentRow, 2, bar);

    // Column 3: Processing Capacity Boundaries
    taskTable->setItem(currentRow, 3, new QTableWidgetItem(QString("0 / %1 MB").arg(totalCapacityMb)));

    // Column 4: Execution State Identifier
    taskTable->setItem(currentRow, 4, new QTableWidgetItem("QUEUED_IN_DRIVER"));
}
