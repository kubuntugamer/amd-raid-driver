#ifndef DASHBOARDWIDGET_H
#define DASHBOARDWIDGET_H

#include <QWidget>

class DashboardWidget : public QWidget {
    Q_OBJECT
public:
    explicit DashboardWidget(QWidget *parent = nullptr);
    ~DashboardWidget();

public slots:
    // Satisfies the internal link references expected by the moc engine
    void appendSimulatedLog(const QString &message, const QString &type = "INFO");

private:
    void setupUiLayout();
};

#endif
