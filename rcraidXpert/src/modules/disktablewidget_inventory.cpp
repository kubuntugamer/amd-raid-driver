#include "disktablewidget.h"
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QLabel>
#include <QGridLayout>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

void DiskTableWidget::populateHostSummary(const QString &nodeName) {
    setColumnCount(1); setRowCount(1);
    horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    verticalHeader()->setVisible(false); horizontalHeader()->setVisible(false);

    QWidget *canvas = new QWidget(this);
    canvas->setStyleSheet("background-color: #1a1c1e; border: 2px solid #3c4043; border-radius: 6px;");
    QVBoxLayout *layout = new QVBoxLayout(canvas);
    layout->setContentsMargins(24, 24, 24, 24);

    QLabel *lblTitle = new QLabel(QString("SYSTEM MANAGEMENT PLANE: %1").arg(nodeName.toUpper()), this);
    lblTitle->setStyleSheet("color: #eff0f1; font-weight: bold; font-size: 14px; border: none;");
    layout->addWidget(lblTitle);

    QHBoxLayout *portLayout = new QHBoxLayout();
    portLayout->setSpacing(12);
    for (int i = 0; i < 4; ++i) {
        QLabel *port = new QLabel(this);
        port->setFixedSize(64, 40);
        port->setAlignment(Qt::AlignCenter);
        port->setStyleSheet(QString("background-color: #2d3033; border: 2px solid #5f6368; border-radius: 3px; "
                                    "color: %1; font-family: monospace; font-size: 9px; font-weight: bold;")
                            .arg(i == 0 ? "#39ff14" : "#888888"));
        port->setText(QString("ETH 0%1\n[%2]").arg(i + 1).arg(i == 0 ? "UP" : "LN"));
        portLayout->addWidget(port);
    }
    portLayout->addStretch();
    layout->addSpacing(16);
    layout->addLayout(portLayout);

    QLabel *lblDesc = new QLabel("All network management rings operating inside nominal tolerances. PCIe core link width distribution initialized.", this);
    lblDesc->setStyleSheet("color: #aaaaaa; font-size: 11px; border: none; margin-top: 12px;");
    layout->addWidget(lblDesc);
    layout->addStretch();

    setCellWidget(0, 0, canvas);
    setRowHeight(0, 380);
}

void DiskTableWidget::populateControllerDashboard(const QString &nodeName, const QString &parentName) {
    setColumnCount(1); setRowCount(2);
    horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    verticalHeader()->setVisible(false); horizontalHeader()->setVisible(false);

    int maxSlots = (nodeName.contains("Embedded") || parentName.contains("Local")) ? 4 : 28;
    m_selectedCanvasSlots.clear();

    int unprovisionedCount = 0;
    for (int i = 0; i < maxSlots; ++i) {
        int num = i + 1;
        QString diskName = QString("nvme_spare_%1").arg(num < 10 ? "0" + QString::number(num) : QString::number(num));
        QString uniqueDiskKey = QString("%1::%2").arg(parentName.isEmpty() ? "Local Host (Application Management Plane)" : parentName, diskName);
        if (m_provisionedDrives.contains(uniqueDiskKey)) continue;
        unprovisionedCount++;
    }

    setCellWidget(0, 0, createGraphicalTelemetryHeader(nodeName, parentName, unprovisionedCount));
    setRowHeight(0, 120);

    setCellWidget(1, 0, createChassisHardwareCanvas(nodeName, parentName, maxSlots));
    setRowHeight(1, 260);
}

QWidget* DiskTableWidget::createChassisHardwareCanvas(const QString &nodeName, const QString &parentName, int totalSlots) {
    QWidget *canvasFrame = new QWidget(this);
    canvasFrame->setStyleSheet("background-color: #111315; border: 1px solid rgba(120,120,120,0.15); border-radius: 4px;");
    
    QVBoxLayout *rootLayout = new QVBoxLayout(canvasFrame);
    rootLayout->setContentsMargins(12, 12, 12, 12);

    QLabel *lblLabel = new QLabel("PHYSICAL DRIVE BAY COMPARTMENT MAP (CLICK SLOTS DIRECTLY TO STAGE FOR PROVISIONING)", this);
    lblLabel->setStyleSheet("color: #888888; font-family: monospace; font-size: 10px; font-weight: bold; border: none;");
    rootLayout->addWidget(lblLabel);

    QWidget *gridContainer = new QWidget(this);
    QGridLayout *gridLayout = new QGridLayout(gridContainer);
    gridLayout->setSpacing(10);
    gridLayout->setContentsMargins(0, 8, 0, 0);

    int maxColumns = totalSlots <= 4 ? 4 : 7;
    int visibleSlotCount = 0;

    for (int i = 0; i < totalSlots; ++i) {
        int slotID = i + 1;
        QString diskName = QString("nvme_spare_%1").arg(slotID < 10 ? "0" + QString::number(slotID) : QString::number(slotID));
        QString uniqueKey = QString("%1::%2").arg(parentName.isEmpty() ? "Local Host (Application Management Plane)" : parentName, diskName);

        if (m_provisionedDrives.contains(uniqueKey)) continue;

        int row = visibleSlotCount / maxColumns;
        int col = visibleSlotCount % maxColumns;

        QToolButton *slotButton = new QToolButton(this);
        slotButton->setCheckable(true);
        slotButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        slotButton->setMinimumHeight(44);

        bool isHighlighted = m_selectedCanvasSlots.contains(slotID);
        slotButton->setChecked(isHighlighted);
        slotButton->setText(QString("BAY_%1\n%2").arg(slotID < 10 ? "0" + QString::number(slotID) : QString::number(slotID)).arg(isHighlighted ? "[STAGED]" : "[SPARE]"));
        
        QString slotStyle = isHighlighted 
            ? "background-color: #10324d; border: 2px solid #00bfff; border-radius: 3px; color: #00bfff; font-family: monospace; font-size: 10px; font-weight: bold;"
            : "background-color: #232629; border: 1px dashed #4d5053; border-radius: 3px; color: #eff0f1; font-family: monospace; font-size: 10px;";
        
        slotButton->setStyleSheet(slotStyle);

        connect(slotButton, &QToolButton::clicked, this, [this, slotButton, slotID, parentName]() {
            if (m_selectedCanvasSlots.contains(slotID)) {
                m_selectedCanvasSlots.remove(slotID);
                slotButton->setText(QString("BAY_%1\n[SPARE]").arg(slotID < 10 ? "0" + QString::number(slotID) : QString::number(slotID)));
                slotButton->setStyleSheet("background-color: #232629; border: 1px dashed #4d5053; border-radius: 3px; color: #eff0f1; font-family: monospace; font-size: 10px;");
            } else {
                m_selectedCanvasSlots.insert(slotID);
                slotButton->setText(QString("BAY_%1\n[STAGED]").arg(slotID < 10 ? "0" + QString::number(slotID) : QString::number(slotID)));
                slotButton->setStyleSheet("background-color: #10324d; border: 2px solid #00bfff; border-radius: 3px; color: #00bfff; font-family: monospace; font-size: 10px; font-weight: bold;");
            }
            emit itemChanged(nullptr);
        });

        gridLayout->addWidget(slotButton, row, col);
        visibleSlotCount++;
    }

    if (visibleSlotCount == 0) {
        QLabel *lblEmpty = new QLabel("ALL DISK ENVELOPE MODULES ALLOCATED IN RUNNING ARRAYS", this);
        lblEmpty->setStyleSheet("color: #39ff14; font-family: monospace; font-size: 11px; font-weight: bold; border: none; padding: 20px;");
        lblEmpty->setAlignment(Qt::AlignCenter);
        gridLayout->addWidget(lblEmpty, 0, 0);
    }

    rootLayout->addWidget(gridContainer, 1);
    return canvasFrame;
}

void DiskTableWidget::populateArrayMembers() {
    setColumnCount(1); setRowCount(1);
    horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    verticalHeader()->setVisible(false); horizontalHeader()->setVisible(false);

    QWidget *canvas = new QWidget(this);
    canvas->setStyleSheet("background-color: #111315; border: 1px solid rgba(120,120,120,0.15); border-radius: 4px;");
    QVBoxLayout *layout = new QVBoxLayout(canvas);
    layout->setContentsMargins(16, 16, 16, 16);

    QLabel *lblTitle = new QLabel(QString("LOGICAL MATRIX LAYER ARCHITECTURE: %1").arg(m_activeNode.toUpper()), this);
    lblTitle->setStyleSheet("color: #39ff14; font-family: monospace; font-weight: bold; font-size: 11px; border: none;");
    layout->addWidget(lblTitle);

    QHBoxLayout *stripLayout = new QHBoxLayout();
    stripLayout->setSpacing(10);
    for (int i = 0; i < 4; ++i) {
        QLabel *diskBlock = new QLabel(this);
        diskBlock->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        diskBlock->setFixedHeight(80);
        diskBlock->setAlignment(Qt::AlignCenter);
        diskBlock->setStyleSheet("background-color: #14321a; border: 2px solid #39ff14; border-radius: 4px; "
                                 "color: #eff0f1; font-family: monospace; font-size: 11px; font-weight: bold;");
        diskBlock->setText(QString("STRIPE_MEMBER_0%1\n\n7.6 TB | ONLINE\nTemp: %2°C").arg(i + 1).arg(34 + i));
        stripLayout->addWidget(diskBlock);
    }
    layout->addSpacing(12);
    layout->addLayout(stripLayout);
    layout->addStretch();
    setCellWidget(0, 0, canvas);
    setRowHeight(0, 380);
}

void DiskTableWidget::populateIndividualDiskMetrics(const QString &nodeName) {
    setColumnCount(1); setRowCount(1);
    horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    verticalHeader()->setVisible(false); horizontalHeader()->setVisible(false);

    QWidget *m2CardCanvas = new QWidget(this);
    m2CardCanvas->setStyleSheet("background-color: #111315; border: 1px solid rgba(120,120,120,0.15); border-radius: 4px;");
    QVBoxLayout *rootLayout = new QVBoxLayout(m2CardCanvas);
    rootLayout->setContentsMargins(20, 20, 20, 20);

    QLabel *lblTitle = new QLabel(QString("PHYSICAL SCHEMATIC VIEW: DEVICE [%1]").arg(nodeName.toUpper()), this);
    lblTitle->setStyleSheet("color: #eff0f1; font-family: monospace; font-weight: bold; font-size: 12px; border: none;");
    rootLayout->addWidget(lblTitle);

    QWidget *m2PcbFrame = new QWidget(this);
    m2PcbFrame->setFixedHeight(120);
    m2PcbFrame->setStyleSheet("background-color: #153c25; border: 2px solid #2e7d32; border-radius: 4px;");
    QHBoxLayout *pcbLayout = new QHBoxLayout(m2PcbFrame);
    pcbLayout->setContentsMargins(0, 0, 16, 0); pcbLayout->setSpacing(16);

    QLabel *lblPins = new QLabel(this);
    lblPins->setFixedSize(14, 116);
    lblPins->setStyleSheet("background-color: #d4af37; border-right: 2px solid #996515; border-top-left-radius: 2px; border-bottom-left-radius: 2px;");
    pcbLayout->addWidget(lblPins);

    QLabel *lblController = new QLabel("ARM\nCONTROLLER\n[ASIC]", this);
    lblController->setFixedSize(85, 80); lblController->setAlignment(Qt::AlignCenter);
    lblController->setStyleSheet("background-color: #2b2b2b; color: #a9b7c6; border: 1px solid #4d4d4d; font-family: monospace; font-size: 9px; font-weight: bold;");
    pcbLayout->addWidget(lblController);

    for (int i = 0; i < 2; ++i) {
        QLabel *lblNand = new QLabel(QString("3D NAND\nFLASH DIE\n[BLOCK_0%1]").arg(i + 1), this);
        lblNand->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed); lblNand->setFixedHeight(85); lblNand->setAlignment(Qt::AlignCenter);
        lblNand->setStyleSheet("background-color: #1f1f1f; color: #eff0f1; border: 1px solid #3c3c3c; font-family: monospace; font-size: 9px;");
        pcbLayout->addWidget(lblNand);
    }
    rootLayout->addSpacing(16); rootLayout->addWidget(m2PcbFrame);

    QLabel *lblTelemetrySpecs = new QLabel(this);
    lblTelemetrySpecs->setStyleSheet("color: #a9b7c6; font-family: 'Liberation Mono', monospace; font-size: 11px; border: none; margin-top: 16px;");
    lblTelemetrySpecs->setText(
        "REGISTER PROPERTIES [/sys/block/nvme0]:\n"
        "├─ Hardware Serial Key:  AMD-NVME-7630888-SRV90X\n"
        "├─ Total User LBA Bound: 14,904,078,144 Sectors (512e Mapping Mode)\n"
        "├─ Link Width State:     PCIe Gen 5.0 x4 Speed Lane Context\n"
        "└─ SMART Diagnostics:   0 Fault Alerts Trapped | HEALTHY (Optimal Operational Profile)"
    );
    rootLayout->addWidget(lblTelemetrySpecs); rootLayout->addStretch();
    setCellWidget(0, 0, m2CardCanvas); setRowHeight(0, 380);
}
