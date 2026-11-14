#include "mainwindow.h"
#include <QPainter>
#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>
#include <QStandardItem>

void MainWindow::connectActionPipelines() {
    QMenuBar *mBar = menuBar();
    if (!mBar) return;

    for (QAction *action : mBar->actions()) {
        if (action->text() != "&Array")
            continue;

        QMenu *arrayMenu = action->menu();
        if (!arrayMenu)
            break;

        arrayMenu->clear();

        QAction *refreshAction = arrayMenu->addAction("Refresh Inventory");
        connect(refreshAction, &QAction::triggered,
                this, &MainWindow::triggerControllerRescan);

        QAction *createAction = arrayMenu->addAction("Create Array from Selection...");
        connect(createAction, &QAction::triggered,
                this, &MainWindow::handleCreateArray);

        QAction *deleteAction = arrayMenu->addAction("Delete Selected Array...");
        connect(deleteAction, &QAction::triggered,
                this, &MainWindow::handleDeleteArray);
        break;
    }
}

void MainWindow::reportFailure(const QString &title, const QString &text) {
    if (statusPanel)
        statusPanel->setMessage(title + ": " + text);
    QMessageBox::critical(this, title, text);
}

void MainWindow::handleCreateArray() {
    if (!controller->driverPresent()) {
        reportFailure("Create array", controller->lastError());
        return;
    }
    if (!controller->arrays().isEmpty()) {
        reportFailure("Create array",
                      "A volume is already assembled. Delete it before "
                      "creating another: the driver writes one array box at a "
                      "time and refuses to nest them.");
        return;
    }
    if (!ArrayDataController::privileged()) {
        reportFailure("Root required",
                      "Creating an array rewrites firmware metadata on the "
                      "members and needs CAP_SYS_ADMIN. Start rcraidXpert with "
                      "sudo.");
        return;
    }

    bool levelOk = false;
    const quint32 level =
        arrayOptions->raidTypeCombo->currentData().toUInt(&levelOk);
    if (!levelOk) {
        reportFailure("Create array", "Select a RAID level first.");
        return;
    }

    const QVector<quint32> indices = diskTable->checkedDiskIndices();
    const int count = indices.size();
    QString problem;

    if (count == 0) {
        reportFailure("No disks selected",
                      "Tick the Use box on the disks you want in the array, "
                      "then press Commit again.");
        return;
    }

    switch (level) {
    case AMD_ARRAY_LEVEL_RAID0:
        if (count < 2)
            problem = QString("RAID 0 needs at least 2 disks; %1 selected.")
                          .arg(count);
        break;
    case AMD_ARRAY_LEVEL_RAID1:
        if (count != 2)
            problem = QString("RAID 1 needs exactly 2 disks; %1 selected.")
                          .arg(count);
        break;
    case AMD_ARRAY_LEVEL_RAID10:
        if (count < 4 || (count % 2) != 0)
            problem = QString("RAID 10 needs an even number of at least 4 "
                              "disks; %1 selected.")
                          .arg(count);
        break;
    case AMD_ARRAY_LEVEL_RAID5:
        if (count < 3) {
            problem = QString("RAID 5 needs at least 3 disks; %1 selected.")
                          .arg(count);
        } else if (ArrayDataController::parityCreateParameter() == 0) {
            problem = "RAID 5 create is disabled: the module was loaded "
                      "without parity_create=1. The writer emits metadata only "
                      "and needs members whose user area is known empty.";
        }
        break;
    default:
        problem = "That RAID level has no metadata writer in this driver.";
        break;
    }
    if (!problem.isEmpty()) {
        reportFailure("Invalid selection", problem);
        return;
    }

    quint32 sizeMb = 0;
    const QString capacityText = arrayOptions->capacityEdit->text().trimmed();
    if (!capacityText.isEmpty()) {
        bool sizeOk = false;

        sizeMb = capacityText.toUInt(&sizeOk);
        if (!sizeOk || sizeMb == 0) {
            reportFailure("Invalid capacity",
                          "Enter the capacity in MiB, or leave the field blank "
                          "to use the full member capacity.");
            return;
        }
    }

    QStringList members;
    for (quint32 index : indices)
        members << QString::number(index);

    QString text = QString("Create %1 over disk(s) %2?\n\n"
                           "This writes new firmware metadata over the selected "
                           "members and replaces whatever layout they carry.")
                       .arg(ArrayDataController::levelName(level),
                            members.join(", "));
    if (level == AMD_ARRAY_LEVEL_RAID5)
        text += "\n\nRAID 5 create writes metadata only: data and parity are "
                "not initialised. Zero the new block device before use.";

    if (QMessageBox::warning(this, "Create array", text,
                             QMessageBox::Cancel | QMessageBox::Ok,
                             QMessageBox::Cancel) != QMessageBox::Ok)
        return;

    if (!controller->createArray(level, indices, sizeMb)) {
        reportFailure("Create failed", controller->lastError());
        return;
    }

    diskTable->clearChecks();
    triggerControllerRescan();
    statusPanel->setMessage(
        QString("%1 created over %2 disk(s).%3")
            .arg(ArrayDataController::levelName(level))
            .arg(count)
            .arg(level == AMD_ARRAY_LEVEL_RAID5
                     ? " Zero the new device before use."
                     : " The volume is not formatted."));
}

void MainWindow::handleDeleteArray() {
    if (!controller->driverPresent()) {
        reportFailure("Delete array", controller->lastError());
        return;
    }

    const QModelIndex current = arrayTreeView->currentIndex();
    if (!current.isValid()) {
        reportFailure("Delete array",
                      "Select a logical drive under \"Logical Drives\" first.");
        return;
    }

    QStandardItem *item = treeModel->itemFromIndex(current);
    const QVariant idValue = item ? item->data(Qt::UserRole) : QVariant();
    if (!idValue.isValid()) {
        reportFailure("Delete array",
                      "The current selection is not a logical drive.");
        return;
    }
    const quint32 arrayId = idValue.toUInt();

    QString description = QString("LD %1").arg(arrayId);
    for (const AmdArrayInfo &array : controller->arrays()) {
        if (array.id != arrayId)
            continue;
        QStringList members;
        for (quint32 member : array.members)
            members << QString::number(member);
        description = QString("LD %1 (%2, %3 member(s): disk %4)")
                          .arg(array.id)
                          .arg(ArrayDataController::levelName(array.level))
                          .arg(array.members.size())
                          .arg(members.join(", "));
    }

    if (QMessageBox::warning(
            this, "Delete array",
            QString("Delete %1?\n\n"
                    "The driver rewrites the member metadata without this "
                    "array. Member user areas are left untouched, and the "
                    "disks return to the unassigned pool.")
                .arg(description),
            QMessageBox::Cancel | QMessageBox::Ok,
            QMessageBox::Cancel) != QMessageBox::Ok)
        return;

    if (!controller->deleteArray(arrayId)) {
        reportFailure("Delete failed", controller->lastError());
        return;
    }

    triggerControllerRescan();
    statusPanel->setMessage(
        QString("Array %1 deleted; members released.").arg(arrayId));
}

// Draw look-alike Blue/Green Status Blocks inside Left Navigation Tree Category Branches
QIcon MainWindow::createProceduralHostIcon() {
    QPixmap pix(12, 12);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setBrush(QColor(0, 120, 215)); // Assigned Member Blue
    p.setPen(QColor(0, 120, 215).darker(150));
    p.drawRect(0, 0, 11, 11);
    return QIcon(pix);
}

QIcon MainWindow::createProceduralHbaIcon() {
    QPixmap pix(12, 12);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setBrush(QColor(16, 124, 16)); // Unassigned/Spare Green
    p.setPen(QColor(16, 124, 16).darker(150));
    p.drawRect(0, 0, 11, 11);
    return QIcon(pix);
}

QIcon MainWindow::createProceduralDriveIcon() {
    QPixmap pix(12, 12);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setBrush(QColor(0, 120, 215)); // Default Logical Node Volume Member Blue
    p.setPen(QColor(0, 120, 215).darker(150));
    p.drawRect(0, 0, 11, 11);
    return QIcon(pix);
}
