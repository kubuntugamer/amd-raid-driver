#include "taskmanagerwidget.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QHeaderView>

TaskManagerWidget::TaskManagerWidget(QWidget *parent) : QWidget(parent) {
    setupUiLayout();
}

TaskManagerWidget::~TaskManagerWidget() {}

void TaskManagerWidget::setupUiLayout() {
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);

    QLabel *headerLabel = new QLabel("Logical Task Manager Background Progress Subsystem", this);
    headerLabel->setStyleSheet("font-weight: bold; padding-bottom: 4px;");
    layout->addWidget(headerLabel);

    taskTable = new QTableWidget(this);
    taskTable->setColumnCount(4);
    taskTable->setHorizontalHeaderLabels({"Task ID", "Array Background Operation", "Target Storage volume", "Execution Progress / State"});
    taskTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    taskTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    taskTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    // Starts completely clean. Active arrays will mount background operations here dynamically.
    taskTable->setRowCount(0);

    layout->addWidget(taskTable);
}

void TaskManagerWidget::registerNewSyncTask(const QString &taskName, int targetVolumeId, int initialProgress) {
    int row = taskTable->rowCount();
    taskTable->insertRow(row);
    taskTable->setItem(row, 0, new QTableWidgetItem(QString("TSK_%1").arg(row + 101)));
    taskTable->setItem(row, 1, new QTableWidgetItem(taskName));
    taskTable->setItem(row, 2, new QTableWidgetItem(QString("LD_%1").arg(targetVolumeId)));
    taskTable->setItem(row, 3, new QTableWidgetItem(QString("%1% Running").arg(initialProgress)));
}
