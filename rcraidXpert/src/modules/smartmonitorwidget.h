#ifndef SMARTMONITORWIDGET_H
#define SMARTMONITORWIDGET_H

#include <QWidget>
#include <QTableWidget>

class SmartMonitorWidget : public QWidget {
    Q_OBJECT
public:
    explicit SmartMonitorWidget(QWidget *parent = nullptr);
    ~SmartMonitorWidget();
private:
    void setupUiLayout();
    QTableWidget *smartTable;
};

#endif
