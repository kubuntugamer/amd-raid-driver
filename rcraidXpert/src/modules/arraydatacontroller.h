#pragma once
#include <QObject>
#include <QString>
#include <QList>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

/* Shared with the kernel bridge.  Keep the ioctl numbers and packed payload
 * layout in src/amd_raid_uapi.h; do not redeclare them here. */
#include "amd_raid_uapi.h"

class ArrayDataController : public QObject {
    Q_OBJECT
public:
    explicit ArrayDataController(QObject *parent = nullptr);
    ~ArrayDataController();
};
