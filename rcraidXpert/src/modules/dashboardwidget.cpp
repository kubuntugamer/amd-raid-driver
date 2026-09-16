#include "dashboardwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QDateTime>

DashboardWidget::DashboardWidget(QWidget *parent) : QWidget(parent) {
    setupUiLayout();
}

DashboardWidget::~DashboardWidget() {}

void DashboardWidget::setupUiLayout() {
    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setSpacing(12);

    // =========================================================================
    // 🅐 BACK-END STATE MATRIX (STUBBED HARDWARE REGISTERS)
    // =========================================================================
    QLabel *registerTitle = new QLabel("AMD Kernel Driver Subsystem Register Status [/dev/amd_ctl0]", this);
    registerTitle->setStyleSheet("font-weight: bold; font-size: 12px;");
    rootLayout->addWidget(registerTitle);

    registerTable = new QTableWidget(this);
    registerTable->setColumnCount(3);
    registerTable->setRowCount(3);
    registerTable->setHorizontalHeaderLabels({"Driver Subsystem Target", "Active Context Parameter", "Register Mapping Address"});
    
    // Spacious Infrastructure Grid Layout Settings
    registerTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    registerTable->verticalHeader()->setVisible(false); 
    registerTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    registerTable->setAlternatingRowColors(true);
    
    // Inject clean padding margins around layout text items
    registerTable->setStyleSheet("QTableWidget::item { padding: 6px 12px; }");

    // Enforce large row heights for clean visual isolation
    for (int row = 0; row < 3; ++row) {
        registerTable->setRowHeight(row, 54);
    }
    
    // Set total container tracking height (Header 34px + 3 Rows x 54px + 4px Padding margins)
    registerTable->setFixedHeight(200);

    // Row 0: Asynchronous kthread Workqueues Tracker (src/patch_async_worker.c)
    registerTable->setItem(0, 0, new QTableWidgetItem("kamd_async_worker Pool"));
    registerTable->setItem(0, 1, new QTableWidgetItem("8 Active kthreads [IDLE_WAIT]"));
    registerTable->setItem(0, 2, new QTableWidgetItem("SYSFS: /sys/kernel/rcraid/workers"));

    // Row 1: Parity Math Metric Trackers (src/patch_parity_math.c)
    registerTable->setItem(1, 0, new QTableWidgetItem("GF(2^8) Galois Field Engine"));
    registerTable->setItem(1, 1, new QTableWidgetItem("256-Byte LUT Tracking Matrix [OPTIMAL]"));
    registerTable->setItem(1, 2, new QTableWidgetItem("MMIO: 0x1022:b000/0x40"));

    // Row 2: Transient Fault Isolation Log Channel (src/rc_nvme.c)
    registerTable->setItem(2, 0, new QTableWidgetItem("Transient Fault Intercept Loop"));
    registerTable->setItem(2, 1, new QTableWidgetItem("Trap Active [Forcing BLK_STS_RESOURCE]"));
    registerTable->setItem(2, 2, new QTableWidgetItem("CONTROL: /dev/amd_ctl0"));

    // Centering alignments check
    for (int r = 0; r < registerTable->rowCount(); ++r) {
        for (int c = 0; c < registerTable->columnCount(); ++c) {
            if (registerTable->item(r, c)) {
                registerTable->item(r, c)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            }
        }
    }

    rootLayout->addWidget(registerTable);

    // =========================================================================
    // 🅒 CONTINUOUS READ-ONLY TERMINAL LOG CONSOLE
    // =========================================================================
    QLabel *consoleTitle = new QLabel("System Monitor Log Stream", this);
    consoleTitle->setStyleSheet("font-weight: bold; font-size: 12px;");
    rootLayout->addWidget(consoleTitle);

    consoleLogView = new QTextEdit(this);
    consoleLogView->setReadOnly(true);
    
    consoleLogView->setStyleSheet(
        "background-color: #0c0f12; "
        "color: #39ff14; "
        "font-family: 'Liberation Mono', 'DejaVu Sans Mono', monospace; "
        "font-size: 10.5pt; "
        "border: 1px solid rgba(120, 120, 120, 0.2);"
    );
    rootLayout->addWidget(consoleLogView, 1);
}

void DashboardWidget::appendSimulatedLog(const QString &type, const QString &message) {
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    QString labelColor = "#39ff14";
    
    if (type == "WARNING" || type == "RETRY_INTERCEPT") {
        labelColor = "#ffb86c";
    } else if (type == "CRITICAL") {
        labelColor = "#ff5555";
    }

    QString formattedLine = QString("<span style='color: #888888;'>[%1]</span> "
                                    "<span style='color: %2; font-weight: bold;'>[%3]</span> "
                                    "<span style='color: #eff0f1;'>%4</span>")
                                    .arg(timeStr, labelColor, type, message);

    consoleLogView->append(formattedLine);
}
