#ifndef DISKTABLEWIDGET_H
#define DISKTABLEWIDGET_H

#include <QTableWidget>
#include <QList>
#include <QVector>
#include <QIcon>

#include "arraydatacontroller.h"

class DiskTableWidget : public QTableWidget {
    Q_OBJECT
public:
    explicit DiskTableWidget(QWidget *parent = nullptr);
    ~DiskTableWidget();

    /* Rows come straight from AMD_IOCTL_GET_HARDWARE_INVENTORY.  Disks that
     * already belong to an array are shown but cannot be checked. */
    void setInventory(const QList<AmdDiskInfo> &disks,
                      const QList<AmdArrayInfo> &arrays);

    QVector<quint32> checkedDiskIndices() const;
    void clearChecks();

private:
    void setupUiLayout();
    static QString formatBytes(quint64 bytes);
};

#endif
