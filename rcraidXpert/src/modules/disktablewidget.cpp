#include "disktablewidget.h"

#include <QHeaderView>
#include <QTableWidgetItem>
#include <QSignalBlocker>
#include <QColor>

DiskTableWidget::DiskTableWidget(QWidget *parent) : QTableWidget(parent) {
    setupUiLayout();
}

DiskTableWidget::~DiskTableWidget() {}

void DiskTableWidget::setupUiLayout() {
    setColumnCount(5);
    setHorizontalHeaderLabels({"Use", "Disk", "Index", "Capacity", "Member of"});
    horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setRowCount(0);
}

QString DiskTableWidget::formatBytes(quint64 bytes) {
    const double gib = static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0);

    if (gib >= 1024.0)
        return QString("%1 TiB").arg(gib / 1024.0, 0, 'f', 2);
    return QString("%1 GiB").arg(gib, 0, 'f', 1);
}

void DiskTableWidget::setInventory(const QList<AmdDiskInfo> &disks,
                                   const QList<AmdArrayInfo> &arrays) {
    const QSignalBlocker blocker(this);

    clearContents();
    setRowCount(disks.size());

    for (int row = 0; row < disks.size(); row++) {
        const AmdDiskInfo &disk = disks.at(row);
        QString memberOf;

        /* Membership comes from the arrays the driver reports, not from
         * the per-disk is_assigned flag: after a DELETE the driver can
         * still advertise a slot on a disk no array owns, and a member of
         * nothing must be selectable for the next CREATE. */
        for (const AmdArrayInfo &array : arrays) {
            if (array.members.contains(disk.index))
                memberOf = QString("LD %1 (%2)")
                               .arg(array.id)
                               .arg(ArrayDataController::levelName(array.level));
        }
        const bool memberOfArray = !memberOf.isEmpty();

        QTableWidgetItem *use = new QTableWidgetItem;
        if (memberOfArray) {
            /* No CheckStateRole at all: a checkbox that cannot be
             * toggled reads as "the GUI refuses my click". */
            use->setFlags(Qt::ItemIsEnabled);
            use->setText("-\u2009");
            use->setToolTip("This disk belongs to an array; delete the "
                            "array to make it selectable.");
        } else {
            use->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
            use->setCheckState(Qt::Unchecked);
        }
        setItem(row, 0, use);

        setItem(row, 1, new QTableWidgetItem(QString("Disk %1").arg(disk.index)));
        setItem(row, 2, new QTableWidgetItem(QString::number(disk.index)));

        const quint64 bytes = disk.sectors * 512ull;
        QTableWidgetItem *capacity = new QTableWidgetItem(formatBytes(bytes));
        capacity->setData(Qt::UserRole, QVariant::fromValue(bytes));
        setItem(row, 3, capacity);

        QTableWidgetItem *member = new QTableWidgetItem(
            memberOfArray ? memberOf : QStringLiteral("unassigned"));
        if (memberOfArray)
            member->setForeground(QColor(0x1f, 0x7a, 0xd4));
        setItem(row, 4, member);

        for (int col = 1; col < columnCount(); col++)
            item(row, col)->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    }
}

QVector<quint32> DiskTableWidget::checkedDiskIndices() const {
    QVector<quint32> indices;

    for (int row = 0; row < rowCount(); row++) {
        QTableWidgetItem *use = item(row, 0);
        QTableWidgetItem *idx = item(row, 2);

        if (!use || !idx)
            continue;
        if (use->checkState() != Qt::Checked)
            continue;
        indices.append(static_cast<quint32>(idx->text().toUInt()));
    }
    return indices;
}

void DiskTableWidget::clearChecks() {
    const QSignalBlocker blocker(this);

    for (int row = 0; row < rowCount(); row++) {
        if (QTableWidgetItem *use = item(row, 0))
            if (use->flags() & Qt::ItemIsUserCheckable)
                use->setCheckState(Qt::Unchecked);
    }
}
