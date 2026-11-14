#ifndef ARRAYDATACONTROLLER_H
#define ARRAYDATACONTROLLER_H

#include <QObject>
#include <QString>
#include <QList>
#include <QVector>

/* Shared with the kernel bridge.  Keep the ioctl numbers and packed payload
 * layout in src/amd_raid_uapi.h; do not redeclare them here. */
#include "amd_raid_uapi.h"

struct AmdDiskInfo {
    quint32 index = 0;
    quint64 sectors = 0;
    bool assigned = false;
};

struct AmdArrayInfo {
    quint32 id = 0;
    quint32 level = 0;
    quint32 stripeSectors = 0;
    quint64 bytes = 0;
    quint32 memberCount = 0;
    QVector<quint32> members;
    QString name;
    bool readOnly = false;
};

/*
 * Every call here goes to /dev/amd_ctl0.  Status/inventory are readable by
 * any user; create and delete mutate firmware metadata and need
 * CAP_SYS_ADMIN, which the driver checks itself.
 */
class ArrayDataController : public QObject {
    Q_OBJECT
public:
    explicit ArrayDataController(QObject *parent = nullptr);
    ~ArrayDataController() override;

    /* Re-read inventory and status.  False means lastError() explains it. */
    bool refresh();

    bool driverPresent() const { return m_present; }
    static bool privileged();
    quint32 statusMask() const { return m_statusMask; }

    const QList<AmdDiskInfo> &disks() const { return m_disks; }
    const QList<AmdArrayInfo> &arrays() const { return m_arrays; }
    QString lastError() const { return m_error; }

    /* Level is the UAPI spelling (0/1/5/10); sizeMb 0 lets the writer derive
     * the capacity from the members.  Rejects RAID 6, which has no writer. */
    bool createArray(quint32 level, const QVector<quint32> &indices,
                     quint32 sizeMb);
    bool deleteArray(quint32 arrayId);

    /* -1 when the module is not loaded or the parameter is unreadable. */
    static int parityCreateParameter();

    static QString levelName(quint32 level);
    static QString errorText(int err, quint32 level = 0);

private:
    QList<AmdDiskInfo> m_disks;
    QList<AmdArrayInfo> m_arrays;
    QString m_error;
    quint32 m_statusMask = 0;
    bool m_present = false;
};

#endif
