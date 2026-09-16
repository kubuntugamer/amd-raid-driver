#include "disktablewidget.h"
#include <QPainter>
#include <QLabel>
#include <QHBoxLayout>

QWidget* DiskTableWidget::createGraphicalTelemetryHeader(const QString &nodeName, const QString &parentName, int unprovisionedCount) {
    QWidget *frame = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(frame);
    layout->setContentsMargins(16, 12, 16, 12); layout->setSpacing(24);

    int maxSpares = (nodeName.contains("Embedded") || parentName.contains("Local")) ? 4 : 20;
    int arrayCount = (maxSpares - unprovisionedCount) / 4;

    // 1. THERMAL RADIAL DONUT GAUGE
    int coreTemp = nodeName.contains("Embedded") ? 42 : 58;
    QPixmap thermalPix(100, 100); thermalPix.fill(Qt::transparent);
    QPainter pTemp(&thermalPix); pTemp.setRenderHint(QPainter::Antialiasing);
    pTemp.setPen(QPen(QColor(45, 48, 52), 8)); pTemp.drawEllipse(10, 10, 80, 80);
    QColor arcColor = coreTemp > 50 ? QColor(255, 140, 0) : QColor(0, 191, 255);
    pTemp.setPen(QPen(arcColor, 8, Qt::SolidLine, Qt::RoundCap));
    pTemp.drawArc(10, 10, 80, 80, 90 * 16, -(coreTemp * 16 * 360) / 100);
    pTemp.setPen(palette().color(QPalette::Text)); pTemp.setFont(QFont("Monospace", 10, QFont::Bold));
    pTemp.drawText(QRect(10, 10, 80, 80), Qt::AlignCenter, QString("%1°C").arg(coreTemp));
    pTemp.end();
    QLabel *lblThermal = new QLabel(this); lblThermal->setPixmap(thermalPix);

    // 2. ARRAY RESOURCE BLOCK STATUS BADGE
    QPixmap arrayPix(120, 100); arrayPix.fill(Qt::transparent);
    QPainter pArr(&arrayPix); pArr.setRenderHint(QPainter::Antialiasing);
    pArr.setBrush(arrayCount > 0 ? QColor(20, 70, 40) : QColor(50, 52, 56));
    pArr.setPen(QPen(arrayCount > 0 ? QColor(57, 255, 20) : QColor(140, 145, 150), 1));
    pArr.drawRoundedRect(4, 15, 112, 54, 4, 4);
    pArr.setPen(palette().color(QPalette::Text)); pArr.setFont(QFont("Sans", 8, QFont::Bold));
    pArr.drawText(QRect(4, 20, 112, 20), Qt::AlignCenter, "LOGICAL ARRAYS");
    pArr.setFont(QFont("Monospace", 12, QFont::Bold));
    pArr.drawText(QRect(4, 40, 112, 24), Qt::AlignCenter, QString("0%1 Active").arg(arrayCount));
    pArr.end();
    QLabel *lblArray = new QLabel(this); lblArray->setPixmap(arrayPix);

    // 3. PCIe HARDWARE BUS RIBBON
    QPixmap pciePix(180, 100); pciePix.fill(Qt::transparent);
    QPainter pPcie(&pciePix); pPcie.setRenderHint(QPainter::Antialiasing);
    pPcie.setBrush(QColor(35, 38, 41)); pPcie.setPen(QColor(77, 80, 83)); pPcie.drawRoundedRect(4, 20, 172, 44, 2, 2);
    pPcie.setBrush(QColor(215, 165, 30)); pPcie.drawRect(12, 48, 156, 4);
    pPcie.setPen(QColor(57, 255, 20)); pPcie.setFont(QFont("Monospace", 9, QFont::Bold));
    pPcie.drawText(QRect(12, 24, 156, 20), Qt::AlignCenter, nodeName.contains("Embedded") ? "PCIe Gen 5 x8" : "PCIe Gen 5 x16");
    pPcie.end();
    QLabel *lblPcie = new QLabel(this); lblPcie->setPixmap(pciePix);

    layout->addWidget(new QLabel("Core Temp:", this)); layout->addWidget(lblThermal);
    layout->addSpacing(16); layout->addWidget(lblArray); layout->addSpacing(16);
    layout->addWidget(new QLabel("Bus Link State:", this)); layout->addWidget(lblPcie);
    layout->addStretch();
    return frame;
}

int DiskTableWidget::getCheckedDiskCount() const {
    // Dynamically counts total entries inside our live highlighted canvas set
    return m_selectedCanvasSlots.count();
}

int DiskTableWidget::calculateTotalCheckedCapacity() const {
    int count = getCheckedDiskCount();
    int driveSize = m_activeNode.contains("Embedded") ? 4000144 : 7630888;
    return count * driveSize;
}
