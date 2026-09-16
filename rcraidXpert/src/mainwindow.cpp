#include "mainwindow.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFile>
#include <QTimer>
#include <QMessageBox>
#include <QShortcut>
#include <QCheckBox>
#include <QPixmap>
#include <QPainter>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), arrayCounter(0) {
    setWindowTitle("rcraidXpert (Transform)");
    resize(1100, 650);
    
    setupLayout();
    connectActionPipelines();
    triggerControllerRescan();
    loadStyles();
    initializeSimulationTimers();
    connectActionPipelines();
    triggerControllerRescan();
    updateCapacityDisplay();
}

MainWindow::~MainWindow() {}

void MainWindow::loadStyles() {
    // Left as a clean, low-overhead placeholder for native system fallback tracking rules
    QFile styleFile("../assets/styles.qss");
    if (styleFile.exists() && styleFile.open(QFile::ReadOnly)) {
        QString contents = styleFile.readAll();
        if (!contents.isEmpty()) {
            this->setStyleSheet(contents);
        }
    }
}
