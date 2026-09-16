#include "mainwindow.h"

void MainWindow::updateCapacityDisplay() {}
void MainWindow::initializeSimulationTimers() {}

void MainWindow::addNewArrayToTree() {
    if (!treeModel) return;

    // Traverse the left navigation model to find the "Logical Drives" container root
    QList<QStandardItem *> matches = treeModel->findItems("AMD RAID Controller (ID: 0)", Qt::MatchRecursive);
    if (!matches.isEmpty()) {
        QStandardItem *controllerRoot = matches.first();
        for (int i = 0; i < controllerRoot->rowCount(); ++i) {
            QStandardItem *child = controllerRoot->child(i);
            if (child->text() == "Logical Drives") {
                // Dynamically append a fresh nested look-alike target node item to the tree layout
                arrayCounter++;
                QString arrayLabel = QString("LD %1 - RAID 10 (Striped Mirrors Functional Pool)").arg(arrayCounter);
                QStandardItem *newLogicalDisk = new QStandardItem(createProceduralDriveIcon(), arrayLabel);
                child->appendRow(newLogicalDisk);
                
                // Force an update to expand the category tree immediately
                if (arrayTreeView) {
                    arrayTreeView->expand(child->index());
                }
                break;
            }
        }
    }
}
