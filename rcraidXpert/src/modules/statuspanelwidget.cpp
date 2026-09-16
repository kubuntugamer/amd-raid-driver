#include "statuspanelwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <iostream>

StatusPanelWidget::StatusPanelWidget(QWidget *parent) 
    : QGroupBox("System Kernel Core Status (Click to Toggle Mode)", parent), isAuthenticatedState(true) {
    
    QVBoxLayout *statusLayout = new QVBoxLayout(this);
    QWidget *rowWidget = new QWidget(this);
    QHBoxLayout *rowLayout = new QHBoxLayout(rowWidget);
    rowLayout->setContentsMargins(5, 2, 5, 2);
    
    statusIndicator = new QLabel("●", this);
    statusLabel = new QLabel("", this);
    statusLabel->setStyleSheet("color: #E0E0E0; font-family: 'Monospace'; font-size: 11px;");
    
    rowLayout->addWidget(statusIndicator);
    rowLayout->addWidget(statusLabel);
    rowLayout->addStretch();
    statusLayout->addWidget(rowWidget);
    
    updateVisualState();
    
    setStyleSheet(R"(
        QGroupBox { border: 2px dashed #444444; border-radius: 4px; margin-top: 15px; font-size: 11px; color: #FFB300; font-weight: bold; }
        QWidget { background-color: #161616; border-radius: 3px; }
    )");
}

bool StatusPanelWidget::isKernelVerified() const {
    return isAuthenticatedState;
}

void StatusPanelWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        isAuthenticatedState = !isAuthenticatedState;
        updateVisualState();
        std::cout << "rcraidXpert Simulation State Shift: Kernel target configuration toggled to " 
                  << (isAuthenticatedState ? "AUTHENTICATED" : "UNSIGNED / ALERT") << std::endl;
    }
    QGroupBox::mousePressEvent(event);
}

void StatusPanelWidget::updateVisualState() {
    if (isAuthenticatedState) {
        statusIndicator->setStyleSheet("color: #00FF66; font-size: 16px; margin-right: 5px;");
        statusLabel->setText("DKMS Liquorix Core - Authenticated (MOK Key Active / Driver Verified)");
    } else {
        statusIndicator->setStyleSheet("color: #FF3333; font-size: 16px; margin-right: 5px;");
        statusLabel->setText("Generic Staging Kernel - Unsigned (Secure Boot Operational Block Imminent)");
    }
}
