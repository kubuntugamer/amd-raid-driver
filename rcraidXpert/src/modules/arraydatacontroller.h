#pragma once
#include <QObject>
#include <QString>
#include <QList>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

// Strict binary alignment mapping parameters for out-of-tree hardware configuration
struct AmdDriveTelemetry {
    uint8_t  port_id;
    uint8_t  slot_id;
    uint32_t disk_index;
    uint64_t total_sectors;
    uint8_t  is_assigned;
};

struct AmdArrayTelemetry {
    uint32_t array_id;
    uint32_t raid_level;
    uint32_t stripe_sectors;
    uint64_t total_capacity_bytes;
    uint32_t member_disk_count;
    uint8_t  member_indices[32];
    char     name[32];
};

struct AmdHardwarePayload {
    uint32_t total_discovered_drives;
    uint32_t total_discovered_arrays;
    struct AmdDriveTelemetry drives[64];
    struct AmdArrayTelemetry arrays[32];
};

#define AMD_IOCTL_MAGIC 'a'
#define AMD_IOCTL_GET_HARDWARE_INVENTORY _IOR(AMD_IOCTL_MAGIC, 0x30, struct AmdHardwarePayload)
#define AMD_IOCTL_COMMIT_NEW_ARRAY       _IOW(AMD_IOCTL_MAGIC, 0x31, struct AmdHardwarePayload)
#define AMD_IOCTL_DESTROY_ARRAY          _IOW(AMD_IOCTL_MAGIC, 0x32, struct AmdHardwarePayload)

class ArrayDataController : public QObject {
    Q_OBJECT
public:
    explicit ArrayDataController(QObject *parent = nullptr);
    ~ArrayDataController();
};
