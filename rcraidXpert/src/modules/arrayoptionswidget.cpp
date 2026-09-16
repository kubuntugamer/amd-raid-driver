#include "arrayoptionswidget.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QLabel>

ArrayOptionsWidget::ArrayOptionsWidget(QWidget *parent) : QWidget(parent) {
    setupUiLayout();
}

ArrayOptionsWidget::~ArrayOptionsWidget() {}

void ArrayOptionsWidget::setupUiLayout() {
    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(10, 10, 10, 10);

    QLabel *titleLabel = new QLabel("Array Parameters Layout Options", this);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 12px;");
    rootLayout->addWidget(titleLabel);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(8);

    // 1. Architecture Profile Selector
    raidTypeCombo = new QComboBox(this);
    raidTypeCombo->addItem("RAID 0 (Stripe Matrix)");
    raidTypeCombo->addItem("RAID 1 (Mirror Pair)");
    raidTypeCombo->addItem("RAID 10 (Nested Striped Mirror)");
    formLayout->addRow("Selected RAID Profile:", raidTypeCombo);

    // 2. Dynamic Stripe Size Selectors (Stitch your explicit entry text directly into the list item)
    chunkSizeCombo = new QComboBox(this);
    chunkSizeCombo->addItem("64 KB (Standard Random I/O Database Mode)");
    chunkSizeCombo->addItem("128 KB (Default Generic Operating System Mode)");
    chunkSizeCombo->addItem("256 KB (High Throughput File Server Mode)");
    chunkSizeCombo->addItem("512 KB (Enterprise Matrix Cache Sweep Mode)");
    chunkSizeCombo->addItem("1 MB (Gaming / Movie Mode - Sequential Asset Loading)");
    chunkSizeCombo->setCurrentIndex(1); // Default stays at standard 128KB
    formLayout->addRow("Logical Stripe Chunk Size:", chunkSizeCombo);

    // 3. Computed Capacity Feedback Register Line
    capacityEdit = new QLineEdit(this);
    capacityEdit->setReadOnly(true);
    capacityEdit->setText("0 MB");
    capacityEdit->setStyleSheet("font-family: 'Monospace'; font-weight: bold; background-color: rgba(0,0,0,0.05);");
    formLayout->addRow("Aggregated Capacity (MB):", capacityEdit);

    rootLayout->addLayout(formLayout);
}
