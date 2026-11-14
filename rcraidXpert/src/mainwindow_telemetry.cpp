#include "mainwindow.h"

#include <QStandardItem>
#include <QStringList>

static QString formatCapacity(quint64 bytes) {
    const double gib = static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0);

    if (bytes == 0)
        return QStringLiteral("0 B");

    if (gib >= 1024.0)
        return QString("%1 TiB").arg(gib / 1024.0, 0, 'f', 2);
    if (gib >= 1.0)
        return QString("%1 GiB").arg(gib, 0, 'f', 1);
    return QString("%1 MiB").arg(static_cast<double>(bytes) / (1024.0 * 1024.0),
                                 0, 'f', 1);
}

static QString volumeStateText(quint32 mask) {
    QStringList states;

    if (mask & RC_AMD_STATUS_FAILED)
        states << "unavailable";
    if (mask & RC_AMD_STATUS_DEGRADED)
        states << "degraded";
    if (mask & RC_AMD_STATUS_RESYNCING)
        states << "resyncing";
    if (states.isEmpty())
        states << (mask & RC_AMD_STATUS_LIVE ? "optimal" : "unknown");
    return states.join(", ");
}

static void dumpItem(QStandardItem *item, int depth, QStringList *out) {
    if (!item)
        return;
    out->append(QString(depth * 2, ' ') + item->text());
    for (int row = 0; row < item->rowCount(); row++)
        dumpItem(item->child(row), depth + 1, out);
}

void MainWindow::triggerControllerRescan() {
    const bool ok = controller->refresh();

    buildTreeFromInventory();
    diskTable->setInventory(controller->disks(), controller->arrays());
    updateCapacityDisplay();

    if (!ok) {
        statusPanel->setDriverState(false, "/dev/amd_ctl0 unreachable");
        statusPanel->setMessage(controller->lastError());
        return;
    }

    QString detail = QString("/dev/amd_ctl0 live: %1 disk(s), %2 volume(s)%3")
                         .arg(controller->disks().size())
                         .arg(controller->arrays().size())
                         .arg(ArrayDataController::privileged()
                                  ? ", root (create/delete enabled)"
                                  : ", read-only (not root)");
    statusPanel->setDriverState(true, detail);

    if (!controller->arrays().isEmpty()) {
        const AmdArrayInfo &array = controller->arrays().first();
        statusPanel->setMessage(
            QString("LD %1: %2, %3, %4 member(s), %5%6.")
                .arg(array.id)
                .arg(ArrayDataController::levelName(array.level))
                .arg(formatCapacity(array.bytes))
                .arg(array.members.size())
                .arg(volumeStateText(controller->statusMask()))
                .arg(array.readOnly
                         ? ", READ-ONLY (reload with enable_writes=1, and "
                           "parity_writes=1 for RAID 5)"
                         : QString()));
    } else {
        statusPanel->setMessage(
            "No volume assembled. Select disks in the table, choose a level, "
            "then Commit Configuration Vector.");
    }

    if (commitButton) {
        const bool canCreate = controller->driverPresent() &&
                               controller->arrays().isEmpty() &&
                               ArrayDataController::privileged();

        commitButton->setEnabled(canCreate);
        if (!controller->arrays().isEmpty())
            commitButton->setToolTip(
                "LD " + QString::number(controller->arrays().first().id) +
                " is assembled. This driver holds one array: delete it "
                "before creating another.");
        else if (!ArrayDataController::privileged())
            commitButton->setToolTip(
                "Creating an array needs root (CAP_SYS_ADMIN); start "
                "rcraidXpert with sudo.");
        else if (!controller->driverPresent())
            commitButton->setToolTip("rcraid is not loaded.");
        else
            commitButton->setToolTip(
                "Writes new array metadata over the ticked disks.");
    }
}

void MainWindow::buildTreeFromInventory() {
    if (!treeModel)
        return;

    treeModel->clear();
    treeModel->setHorizontalHeaderLabels({"AMD RAIDXpert2 Managed Tree Topology"});

    QStandardItem *root = new QStandardItem(createProceduralHostIcon(),
                                            "AMD RAID Controller (ID: 0)");
    treeModel->appendRow(root);

    if (!controller->driverPresent()) {
        root->appendRow(new QStandardItem(createProceduralDriveIcon(),
                                          "driver not loaded"));
        if (arrayTreeView)
            arrayTreeView->expandAll();
        return;
    }

    QStandardItem *logical = new QStandardItem(
        createProceduralHbaIcon(),
        QString("Logical Drives (%1)").arg(controller->arrays().size()));
    for (const AmdArrayInfo &array : controller->arrays()) {
        QStandardItem *ld = new QStandardItem(
            createProceduralDriveIcon(),
            QString("LD %1 - %2 - %3%4")
                .arg(array.id)
                .arg(ArrayDataController::levelName(array.level))
                .arg(formatCapacity(array.bytes))
                .arg(array.readOnly ? QStringLiteral(" (read-only)")
                                    : QString()));
        ld->setData(QVariant::fromValue(array.id), Qt::UserRole);

        QStringList members;
        for (quint32 member : array.members)
            members << QString::number(member);
        ld->setToolTip(QString("Members: disk %1\nStripe: %2 sectors\n"
                               "Capacity: %3 bytes\nState: %4")
                           .arg(members.join(", "))
                           .arg(array.stripeSectors)
                           .arg(array.bytes)
                           .arg(volumeStateText(controller->statusMask())));
        logical->appendRow(ld);
    }
    root->appendRow(logical);

    QStandardItem *physical = new QStandardItem(
        createProceduralHbaIcon(),
        QString("Physical Drives (%1)").arg(controller->disks().size()));
    for (const AmdDiskInfo &disk : controller->disks()) {
        QString owner;

        for (const AmdArrayInfo &array : controller->arrays()) {
            if (array.members.contains(disk.index))
                owner = QString("LD %1").arg(array.id);
        }

        QString label = QString("Disk %1 - %2")
                            .arg(disk.index)
                            .arg(formatCapacity(disk.sectors * 512ull));
        if (!owner.isEmpty())
            label += QString(" (%1)").arg(owner);

        QStandardItem *item = new QStandardItem(createProceduralDriveIcon(), label);
        item->setToolTip(QString("%1 sectors, %2")
                             .arg(disk.sectors)
                             .arg(owner.isEmpty()
                                      ? "unassigned"
                                      : owner + " member"));
        physical->appendRow(item);
    }
    root->appendRow(physical);

    if (arrayTreeView)
        arrayTreeView->expandAll();
}

void MainWindow::updateCapacityDisplay() {
    quint64 total = 0;
    quint64 free = 0;

    for (const AmdDiskInfo &disk : controller->disks()) {
        const quint64 bytes = disk.sectors * 512ull;

        total += bytes;
        if (!disk.assigned)
            free += bytes;
    }

    setWindowTitle(
        QString("rcraidXpert - %1 disk(s), %2 total, %3 free")
            .arg(controller->disks().size())
            .arg(formatCapacity(total))
            .arg(free ? formatCapacity(free) : QStringLiteral("none")));
}

QStringList MainWindow::treeDump() const {
    QStringList out;

    if (!treeModel)
        return out;
    for (int row = 0; row < treeModel->rowCount(); row++)
        dumpItem(treeModel->item(row), 0, &out);
    return out;
}
