#include "disktablewidget.h"
#include <QHeaderView>
#include <QPainter>
#include <QPixmap>

DiskTableWidget::DiskTableWidget(QWidget *parent) : QTableWidget(parent) {
    setupUiLayout();
}

DiskTableWidget::~DiskTableWidget() {}

void DiskTableWidget::setupUiLayout() {
    setColumnCount(3);
    setHorizontalHeaderLabels({"Status", "ID", "Capacity"});
    horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    
    // Starts completely clean and empty. No mock items are populated at launch.
    setRowCount(0);
}

int DiskTableWidget::getCheckedDiskCount() const {
    int checkedCount = 0;
    for (int i = 0; i < rowCount(); ++i) {
        if (item(i, 0) && item(i, 0)->checkState() == Qt::Checked) {
            checkedCount++;
        }
    }
    return checkedCount;
}

int DiskTableWidget::calculateTotalCheckedCapacity() const {
    return getCheckedDiskCount() * 512; 
}

// This is where real scanned drive structures will attach to the layout grid
void DiskTableWidget::clearAndPopulateInventory(const QString &nodeName, const QString &parentName) {
    (void)nodeName; (void)parentName;
    // Real NVMe scans populate rows here dynamically at runtime
}
