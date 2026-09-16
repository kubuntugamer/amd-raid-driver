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
    // Satisfies the internal link references expected by the moc engine
    void registerNewSyncTask(const QString &taskName, int targetVolumeId, int initialProgress = 0);

private:
    void setupUiLayout();
    QTableWidget *taskTable;
};

#endif
