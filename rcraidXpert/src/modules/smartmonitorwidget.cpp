#include "smartmonitorwidget.h"
#include <QVBoxLayout>
#include <QHeaderView>

SmartMonitorWidget::SmartMonitorWidget(QWidget *parent) : QWidget(parent) {
    setupUiLayout();
}

SmartMonitorWidget::~SmartMonitorWidget() {}

void SmartMonitorWidget::setupUiLayout() {
    QVBoxLayout *layout = new QVBoxLayout(this);
    smartTable = new QTableWidget(this);
    smartTable->setColumnCount(4);
    smartTable->setHorizontalHeaderLabels({"ID", "Attribute Name", "Current Value", "Operational Status"});
    smartTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    
    // Seed standard NVMe physical storage reporting fields
    int row = 0;
    smartTable->insertRow(row);
    smartTable->setItem(row, 0, new QTableWidgetItem("01"));
    smartTable->setItem(row, 1, new QTableWidgetItem("Critical Warning Telemetry"));
    smartTable->setItem(row, 2, new QTableWidgetItem("0x00"));
    smartTable->setItem(row, 3, new QTableWidgetItem("Healthy"));
    
    row++;
    smartTable->insertRow(row);
    smartTable->setItem(row, 0, new QTableWidgetItem("02"));
    smartTable->setItem(row, 1, new QTableWidgetItem("Composite Temperature Grid"));
    smartTable->setItem(row, 2, new QTableWidgetItem("304 Kelvin (31 C)"));
    smartTable->setItem(row, 3, new QTableWidgetItem("Healthy"));

    row++;
    smartTable->insertRow(row);
    smartTable->setItem(row, 0, new QTableWidgetItem("03"));
    smartTable->setItem(row, 1, new QTableWidgetItem("Available Spare Capacity Block"));
    smartTable->setItem(row, 2, new QTableWidgetItem("100%"));
    smartTable->setItem(row, 3, new QTableWidgetItem("Healthy"));

    layout->addWidget(smartTable);
}
