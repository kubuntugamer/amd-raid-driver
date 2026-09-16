#include "backgroundrateswidget.h"
#include <QFormLayout>
#include <QLabel>

BackgroundRatesWidget::BackgroundRatesWidget(QWidget *parent) : QWidget(parent) {
    setupUiLayout();
}

BackgroundRatesWidget::~BackgroundRatesWidget() {}

void BackgroundRatesWidget::setupUiLayout() {
    QFormLayout *layout = new QFormLayout(this);
    
    QLabel *title = new QLabel("Controller Background Activity Execution Thresholds", this);
    title->setStyleSheet("font-weight: bold; margin-bottom: 10px;");
    layout->addRow(title);

    rebuildSlider = new QSlider(Qt::Horizontal, this);
    rebuildSlider->setRange(1, 100);
    rebuildSlider->setValue(50);
    layout->addRow("Array Rebuild Rate (Priority):", rebuildSlider);

    verifySlider = new QSlider(Qt::Horizontal, this);
    verifySlider->setRange(1, 100);
    verifySlider->setValue(30);
    layout->addRow("Consistency Verification Speed:", verifySlider);

    initSlider = new QSlider(Qt::Horizontal, this);
    initSlider->setRange(1, 100);
    initSlider->setValue(40);
    layout->addRow("Background Initialization Rate:", initSlider);
}
