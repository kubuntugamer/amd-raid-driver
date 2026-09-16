#ifndef TASKMANAGERWIDGET_H
#define TASKMANAGERWIDGET_H

#include <QWidget>
#include <QTableWidget>

class TaskManagerWidget : public QWidget {
    Q_OBJECT
public:
    explicit TaskManagerWidget(QWidget *parent = nullptr);
    ~TaskManagerWidget();

public slots:
    // Restores the exact function signature required by mainwindow_actions.cpp
    void registerNewSyncTask(const QString &volumeName, int diskCount, int totalCapacityMb);

private:
    void setupUiLayout();

    QTableWidget *taskTable;
    int taskCounter;
};

#endif
