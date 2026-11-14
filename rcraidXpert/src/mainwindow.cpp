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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), controller(new ArrayDataController(this)) {
    setWindowTitle("rcraidXpert");
    resize(1100, 650);

    setupLayout();
    connectActionPipelines();
    loadStyles();
    triggerControllerRescan();
    updateCapacityDisplay();
}

MainWindow::~MainWindow() {}

void MainWindow::loadStyles() {
    QFile styleFile("../assets/styles.qss");
    if (styleFile.exists() && styleFile.open(QFile::ReadOnly)) {
        QString contents = styleFile.readAll();
        if (!contents.isEmpty()) {
            this->setStyleSheet(contents);
        }
    }
}
