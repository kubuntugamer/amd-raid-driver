#ifndef DISKTABLEWIDGET_H
#define DISKTABLEWIDGET_H

#include <QTableWidget>
#include <QIcon>

class DiskTableWidget : public QTableWidget {
    Q_OBJECT
public:
    explicit DiskTableWidget(QWidget *parent = nullptr);
    ~DiskTableWidget();

    int getCheckedDiskCount() const;
    int calculateTotalCheckedCapacity() const;
    
    // Explicitly declares both parameters so disktablewidget.cpp matches perfectly
    void clearAndPopulateInventory(const QString &nodeName, const QString &parentName = "");

private:
    void setupUiLayout();
    QIcon createProceduralHealthIcon();
    
    QString m_activeNode;
};

#endif
