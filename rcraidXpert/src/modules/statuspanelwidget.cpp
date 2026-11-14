#include "statuspanelwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

StatusPanelWidget::StatusPanelWidget(QWidget *parent)
    : QGroupBox("rcraid Kernel Status", parent) {
    QVBoxLayout *statusLayout = new QVBoxLayout(this);
    QWidget *rowWidget = new QWidget(this);
    QHBoxLayout *rowLayout = new QHBoxLayout(rowWidget);
    rowLayout->setContentsMargins(5, 2, 5, 2);

    statusIndicator = new QLabel("\u25cf", this);
    statusLabel = new QLabel("probing /dev/amd_ctl0", this);
    statusLabel->setStyleSheet("color: #E0E0E0; font-family: 'Monospace'; font-size: 11px;");

    rowLayout->addWidget(statusIndicator);
    rowLayout->addWidget(statusLabel);
    rowLayout->addStretch();
    statusLayout->addWidget(rowWidget);

    messageLabel = new QLabel(this);
    messageLabel->setWordWrap(true);
    messageLabel->setStyleSheet("color: #aaaaaa; font-family: 'Monospace'; font-size: 11px;");
    statusLayout->addWidget(messageLabel);

    setDriverState(false, "probing /dev/amd_ctl0");

    setStyleSheet(R"(
        QGroupBox { border: 2px dashed #444444; border-radius: 4px; margin-top: 15px; font-size: 11px; color: #FFB300; font-weight: bold; }
        QWidget { background-color: #161616; border-radius: 3px; }
    )");
}

void StatusPanelWidget::setDriverState(bool present, const QString &detail) {
    isAuthenticatedState = present;
    statusIndicator->setStyleSheet(present ? "color: #39ff14; font-size: 14px;"
                                           : "color: #ff5252; font-size: 14px;");
    statusLabel->setText(detail);
}

void StatusPanelWidget::setMessage(const QString &message) {
    messageLabel->setText(message);
}
