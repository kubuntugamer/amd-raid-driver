#include "createarraywizard.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>

CreateArrayWizard::CreateArrayWizard(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Create Array Wizard");
    resize(500, 400);
    setupWizardLayout();
}

CreateArrayWizard::~CreateArrayWizard() {}

void CreateArrayWizard::setupWizardLayout() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    QLabel *headerLabel = new QLabel("Configure Array Creation Parameters", this);
    headerLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #111; padding-bottom: 8px;");
    mainLayout->addWidget(headerLabel);

    QFormLayout *formLayout = new QFormLayout();
    
    arrayNameEdit = new QLineEdit("AMD_Array_0", this);
    formLayout->addRow("Array Name:", arrayNameEdit);

    raidLevelCombo = new QComboBox(this);
    raidLevelCombo->addItems({"RAID 0 (Striping)", "RAID 1 (Mirroring)", "RAID 5 (Single Parity)", "RAID 6 (Dual Parity)"});
    formLayout->addRow("RAID Level:", raidLevelCombo);

    stripeSizeCombo = new QComboBox(this);
    stripeSizeCombo->addItems({"64 KB", "128 KB", "256 KB", "512 KB", "1024 KB"});
    formLayout->addRow("Stripe Size:", stripeSizeCombo);

    initTypeCombo = new QComboBox(this);
    initTypeCombo->addItems({"Fast Initialization", "Full Initialization", "No Initialization"});
    formLayout->addRow("Initialization Type:", initTypeCombo);

    mainLayout->addLayout(formLayout);

    QLabel *diskLabel = new QLabel("Select Physical Disk Members:", this);
    diskLabel->setStyleSheet("font-weight: bold; margin-top: 6px;");
    mainLayout->addWidget(diskLabel);

    availableDisksList = new QListWidget(this);
    availableDisksList->setSelectionMode(QAbstractItemView::MultiSelection);
    availableDisksList->addItem("Port 0, Slot 0 - NVMe SSD 512GB (Unassigned)");
    availableDisksList->addItem("Port 1, Slot 0 - NVMe SSD 512GB (Unassigned)");
    availableDisksList->addItem("Port 2, Slot 0 - NVMe SSD 512GB (Unassigned)");
    availableDisksList->addItem("Port 3, Slot 0 - NVMe SSD 512GB (Unassigned)");
    mainLayout->addWidget(availableDisksList);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    btnNext = new QPushButton("Create Array", this);
    btnNext->setStyleSheet("font-weight: bold;");
    btnCancel = new QPushButton("Cancel", this);
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(btnNext);
    buttonLayout->addWidget(btnCancel);
    mainLayout->addLayout(buttonLayout);

    connect(btnNext, &QPushButton::clicked, this, &QDialog::accept);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

QString CreateArrayWizard::getArrayName() const { return arrayNameEdit->text(); }
int CreateArrayWizard::getRaidLevelIndex() const { return raidLevelCombo->currentIndex(); }
int CreateArrayWizard::getStripeSizeIndex() const { return stripeSizeCombo->currentIndex(); }
