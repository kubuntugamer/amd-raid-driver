#include "dashboardwidget.h"
#include "arraydatacontroller.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QFrame>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

DashboardWidget::DashboardWidget(QWidget *parent) : QWidget(parent) {
    setupUiLayout();
}

DashboardWidget::~DashboardWidget() {}

void DashboardWidget::setupUiLayout() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    QLabel *headerLabel = new QLabel("AMD RAIDXpert2 System Status Dashboard Overview", this);
    headerLabel->setStyleSheet("font-weight: bold; font-size: 14px; padding-bottom: 6px;");
    mainLayout->addWidget(headerLabel);

    struct AmdHardwarePayload hardware_state{};
    bool hardware_active = false;
    
    int fd = ::open("/dev/amd_ctl0", O_RDWR | O_NONBLOCK);
    if (fd >= 0) {
        if (::ioctl(fd, AMD_IOCTL_GET_HARDWARE_INVENTORY, &hardware_state) >= 0) {
            hardware_active = true;
        }
        ::close(fd);
    }

    QGridLayout *gridLayout = new QGridLayout();

    auto createStatusBlock = [this](const QString &title, const QString &value, const QString &style) -> QFrame* {
        QFrame *frame = new QFrame(this);
        frame->setFrameShape(QFrame::StyledPanel);
        frame->setFrameShadow(QFrame::Raised);
        
        QVBoxLayout *fLayout = new QVBoxLayout(frame);
        QLabel *lblTitle = new QLabel(title, frame);
        lblTitle->setStyleSheet("font-size: 11px; color: gray;");
        
        QLabel *lblVal = new QLabel(value, frame);
        lblVal->setStyleSheet(style + " font-size: 18px; font-weight: bold;");
        
        fLayout->addWidget(lblTitle);
        fLayout->addWidget(lblVal);
        return frame;
    };

    QString healthText = hardware_active ? "OPTIMAL" : "OFFLINE";
    QString healthStyle = hardware_active ? "color: #107C10;" : "color: #A80000;";
    int controllerCount = hardware_active ? 1 : 0; 
    int logicalCount = hardware_active ? hardware_state.total_discovered_arrays : 0;
    int physicalCount = hardware_active ? hardware_state.total_discovered_drives : 0;

    QFrame *healthFrame = createStatusBlock("Global Subsystem Health", healthText, healthStyle);
    gridLayout->addWidget(healthFrame, 0, 0);

    QFrame *ctrlFrame = createStatusBlock("Active NVMe Controllers", QString("%1 Discovered").arg(controllerCount), "");
    gridLayout->addWidget(ctrlFrame, 0, 1);

    QFrame *logicalFrame = createStatusBlock("Functional Logical Drives", QString("%1 Volumes").arg(logicalCount), "");
    gridLayout->addWidget(logicalFrame, 1, 0);

    QFrame *physicalFrame = createStatusBlock("Available Physical Disks", QString("%1 Devices").arg(physicalCount), "");
    gridLayout->addWidget(physicalFrame, 1, 1);

    mainLayout->addLayout(gridLayout);

    QLabel *logLabel = new QLabel("System Monitor Log Stream", this);
    logLabel->setStyleSheet("font-weight: bold; margin-top: 10px;");
    mainLayout->addWidget(logLabel);

    QFrame *logStreamFrame = new QFrame(this);
    logStreamFrame->setFrameShape(QFrame::StyledPanel);
    logStreamFrame->setFrameShadow(QFrame::Sunken);
    logStreamFrame->setMinimumHeight(200);
    logStreamFrame->setStyleSheet("background-color: rgba(0,0,0,0.05);");
    mainLayout->addWidget(logStreamFrame);

    mainLayout->addStretch();
}

void DashboardWidget::appendSimulatedLog(const QString &message, const QString &type) {
    (void)message; (void)type;
}
