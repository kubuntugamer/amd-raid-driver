#include "spareassignmentdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>

SpareAssignmentDialog::SpareAssignmentDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Assign Spare Drive Configuration");
    resize(450, 320);
    setupUiLayout();
}

SpareAssignmentDialog::~SpareAssignmentDialog() {}

void SpareAssignmentDialog::setupUiLayout() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    QLabel *lbl = new QLabel("Select Unassigned Drive for Spare Designation Pool:", this);
    mainLayout->addWidget(lbl);

    eligibleDrivesList = new QListWidget(this);
    eligibleDrivesList->addItem("Port 2, Slot 0 - NVMe SSD 512GB (Available Spare Candidate)");
    eligibleDrivesList->addItem("Port 3, Slot 0 - NVMe SSD 512GB (Available Spare Candidate)");
    mainLayout->addWidget(eligibleDrivesList);

    QHBoxLayout *radioLayout = new QHBoxLayout();
    globalSpareRadio = new QRadioButton("Global Hot Spare", this);
    dedicatedSpareRadio = new QRadioButton("Dedicated Pool Spare", this);
    globalSpareRadio->setChecked(true);
    radioLayout->addWidget(globalSpareRadio);
    radioLayout->addWidget(dedicatedSpareRadio);
    mainLayout->addLayout(radioLayout);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *btnOk = new QPushButton("Assign Spare", this);
    QPushButton *btnCancel = new QPushButton("Cancel", this);
    btnLayout->addStretch();
    btnLayout->addWidget(btnOk);
    btnLayout->addWidget(btnCancel);
    mainLayout->addLayout(btnLayout);

    connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}
