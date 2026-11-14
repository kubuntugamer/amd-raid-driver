#include "arraydatacontroller.h"

#include <QFile>

#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <cstdio>

#define AMD_CTL_DEV "/dev/amd_ctl0"
#define PARITY_CREATE_PARAM "/sys/module/rcraid/parameters/parity_create"

static bool read_u32_file(const char *path, quint32 *out)
{
    FILE *f = fopen(path, "r");
    unsigned int value;

    if (!f)
        return false;
    if (fscanf(f, "%u", &value) != 1) {
        fclose(f);
        return false;
    }
    fclose(f);
    *out = value;
    return true;
}

ArrayDataController::ArrayDataController(QObject *parent) : QObject(parent) {}
ArrayDataController::~ArrayDataController() = default;

bool ArrayDataController::privileged()
{
    return geteuid() == 0;
}

int ArrayDataController::parityCreateParameter()
{
    quint32 value;

    if (!read_u32_file(PARITY_CREATE_PARAM, &value))
        return -1;
    return value ? 1 : 0;
}

QString ArrayDataController::levelName(quint32 level)
{
    switch (level) {
    case AMD_ARRAY_LEVEL_RAID0:
        return QStringLiteral("RAID 0");
    case AMD_ARRAY_LEVEL_RAID1:
        return QStringLiteral("RAID 1");
    case AMD_ARRAY_LEVEL_RAID5:
        return QStringLiteral("RAID 5");
    case AMD_ARRAY_LEVEL_RAID6:
        return QStringLiteral("RAID 6");
    case AMD_ARRAY_LEVEL_RAID10:
        return QStringLiteral("RAID 10");
    default:
        return QStringLiteral("level %1").arg(level);
    }
}

QString ArrayDataController::errorText(int err, quint32 level)
{
    switch (err) {
    case 0:
        return QString();
    case EPERM:
    case EACCES:
        return QObject::tr("the driver refused: this needs root "
                           "(CAP_SYS_ADMIN)");
    case EOPNOTSUPP:
        if (level == AMD_ARRAY_LEVEL_RAID5)
            return QObject::tr("RAID 5 create is disabled: load the module "
                               "with parity_create=1 and use members whose "
                               "user area is empty (create writes metadata "
                               "only, never data or parity)");
        if (level == AMD_ARRAY_LEVEL_RAID6)
            return QObject::tr("RAID 6 create is not supported: the driver "
                               "has no Q write or initialisation path");
        return QObject::tr("the driver does not implement that operation");
    case EBUSY:
        return QObject::tr("a volume is already assembled; delete it first");
    case EINVAL:
        return QObject::tr("the driver rejected the selection as invalid "
                           "(level, member count or duplicate disk)");
    case ENODEV:
        return QObject::tr("a selected disk is not a bound, initialised "
                           "member");
    case ENOSPC:
        return QObject::tr("the requested size does not fit the selected "
                           "members");
    case ENOENT:
        return QObject::tr("no such array");
    case ENOMEM:
        return QObject::tr("out of memory");
    case EFAULT:
        return QObject::tr("the driver could not copy the request");
    default:
        return QString::fromLocal8Bit(strerror(err));
    }
}

bool ArrayDataController::refresh()
{
    struct AmdHardwarePayload payload;
    quint32 mask = 0;
    int fd, ret;

    m_disks.clear();
    m_arrays.clear();
    m_error.clear();
    m_statusMask = 0;
    m_present = false;

    fd = ::open(AMD_CTL_DEV, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        int err = errno;

        m_error = (err == ENOENT || err == ENODEV)
                       ? tr("rcraid is not loaded (%1 is missing)")
                             .arg(QLatin1String(AMD_CTL_DEV))
                       : tr("cannot open %1: %2")
                             .arg(QLatin1String(AMD_CTL_DEV),
                                  QString::fromLocal8Bit(strerror(err)));
        return false;
    }
    m_present = true;

    memset(&payload, 0, sizeof(payload));
    if (::ioctl(fd, AMD_IOCTL_GET_HARDWARE_INVENTORY, &payload) < 0) {
        int err = errno;

        ::close(fd);
        m_error = tr("inventory request failed: %1")
                      .arg(QString::fromLocal8Bit(strerror(err)));
        return false;
    }

    if (::ioctl(fd, AMD_IOCTL_GET_STATUS, &mask) < 0)
        mask = 0;
    ::close(fd);

    /* The payload is fixed-size; clamp to the array bounds so a driver bug
     * cannot walk us off the end. */
    quint32 ndrives = qMin<quint32>(payload.total_discovered_drives, 64);
    for (quint32 i = 0; i < ndrives; i++) {
        const struct AmdDriveTelemetry &d = payload.drives[i];
        AmdDiskInfo disk;

        disk.index = d.disk_index;
        disk.sectors = d.total_sectors;
        disk.assigned = d.is_assigned != 0;
        m_disks.append(disk);
    }

    quint32 narrays = qMin<quint32>(payload.total_discovered_arrays, 32);
    for (quint32 i = 0; i < narrays; i++) {
        const struct AmdArrayTelemetry &a = payload.arrays[i];
        AmdArrayInfo array;

        array.id = a.array_id;
        array.level = a.raid_level;
        array.stripeSectors = a.stripe_sectors;
        array.bytes = a.total_capacity_bytes;
        array.memberCount = a.member_disk_count;
        for (quint32 m = 0; m < a.member_disk_count && m < 32; m++) {
            if (a.member_indices[m] != 0xff)
                array.members.append(a.member_indices[m]);
        }
        array.name = QString::fromLatin1(a.name, strnlen(a.name, sizeof(a.name)));
        m_arrays.append(array);
    }

    m_statusMask = mask;
    return true;
}

bool ArrayDataController::createArray(quint32 level,
                                      const QVector<quint32> &indices,
                                      quint32 sizeMb)
{
    struct amd_array_create req;
    int fd, ret;

    if (!privileged()) {
        m_error = tr("create requires root; start rcraidXpert with sudo");
        return false;
    }
    if (level == AMD_ARRAY_LEVEL_RAID6) {
        m_error = errorText(EOPNOTSUPP, level);
        return false;
    }
    if (indices.size() < 2 ||
        indices.size() > static_cast<int>(AMD_ARRAY_MAX_MEMBERS)) {
        m_error = tr("select 2 to %1 disks").arg(AMD_ARRAY_MAX_MEMBERS);
        return false;
    }

    memset(&req, 0, sizeof(req));
    req.level = level;
    req.member_count = static_cast<quint32>(indices.size());
    req.size_mb = sizeMb;
    for (int i = 0; i < indices.size(); i++)
        req.member_indices[i] = indices[i];

    fd = ::open(AMD_CTL_DEV, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        m_error = tr("cannot open %1: %2")
                      .arg(QLatin1String(AMD_CTL_DEV),
                           QString::fromLocal8Bit(strerror(errno)));
        return false;
    }
    ret = ::ioctl(fd, AMD_IOCTL_CREATE_ARRAY, &req);
    if (ret < 0) {
        m_error = errorText(errno, level);
        ::close(fd);
        return false;
    }
    ::close(fd);
    m_error.clear();
    return true;
}

bool ArrayDataController::deleteArray(quint32 arrayId)
{
    struct amd_array_delete req;
    int fd, ret;

    if (!privileged()) {
        m_error = tr("delete requires root; start rcraidXpert with sudo");
        return false;
    }

    memset(&req, 0, sizeof(req));
    req.array_id = arrayId;

    fd = ::open(AMD_CTL_DEV, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        m_error = tr("cannot open %1: %2")
                      .arg(QLatin1String(AMD_CTL_DEV),
                           QString::fromLocal8Bit(strerror(errno)));
        return false;
    }
    ret = ::ioctl(fd, AMD_IOCTL_DELETE_ARRAY, &req);
    if (ret < 0) {
        m_error = errorText(errno);
        ::close(fd);
        return false;
    }
    ::close(fd);
    m_error.clear();
    return true;
}
