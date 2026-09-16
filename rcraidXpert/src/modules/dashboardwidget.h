#ifndef DASHBOARDWIDGET_H
#define DASHBOARDWIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QLabel>
#include <QTableWidget>

class DashboardWidget : public QWidget {
    Q_OBJECT
public:
    explicit DashboardWidget(QWidget *parent = nullptr);
    ~DashboardWidget();

public slots:
    void appendSimulatedLog(const QString &type, const QString &message);

private:
    void setupUiLayout();

    QTextEdit *consoleLogView;
    QTableWidget *registerTable;
    
    // Static driver state tracking registers
    QLabel *lblKthreadPool;
    QLabel *lblXorThroughput;
    QLabel *lblGfThroughput;
    QLabel *lblTrappedFaults;
};

#endif
