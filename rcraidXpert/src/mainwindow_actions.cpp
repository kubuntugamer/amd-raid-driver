#include "mainwindow.h"
#include "modules/createarraywizard.h"
#include <QPainter>
#include <QMenuBar>
#include <QMenu>

void MainWindow::triggerControllerRescan() {
    addNewArrayToTree();
}

void MainWindow::connectActionPipelines() {
    QMenuBar *mBar = menuBar();
    if (!mBar) return;

    for (QAction *action : mBar->actions()) {
        if (action->text() == "&Array") {
            QMenu *arrayMenu = action->menu();
            if (arrayMenu) {
                arrayMenu->clear();
                
                QAction *createAction = arrayMenu->addAction("Create Array...");
                QAction *deleteAction = arrayMenu->addAction("Delete Selected Array");
                
                // 1. Array Creation Dialog Pipeline
                connect(createAction, &QAction::triggered, this, [this]() {
                    CreateArrayWizard wizard(this);
                    if (wizard.exec() == QDialog::Accepted) {
                        addNewArrayToTree();
                    }
                });

                // 2. Destructive Array Deletion Pipeline
                connect(deleteAction, &QAction::triggered, this, [this]() {
                    if (!treeModel || !arrayTreeView) return;

                    QModelIndex currentIdx = arrayTreeView->currentIndex();
                    if (currentIdx.isValid()) {
                        QStandardItem *selectedItem = treeModel->itemFromIndex(currentIdx);
                        
                        // Guard baseline category directory folders from accidental drop loops
                        if (selectedItem && selectedItem->text().contains("RAID 10")) {
                            QStandardItem *parentItem = selectedItem->parent();
                            if (parentItem) {
                                parentItem->removeRow(selectedItem->row());
                            }
                        }
                    }
                });
            }
            break;
        }
    }
}

void MainWindow::executeConfigurationPurge() {}
void MainWindow::validateAndCommitArray() {}

QIcon MainWindow::createProceduralHostIcon() {
    QPixmap pix(16, 16); pix.fill(Qt::transparent);
    QPainter p(&pix); p.setBrush(Qt::blue); p.drawRect(2, 2, 12, 12);
    return QIcon(pix);
}

QIcon MainWindow::createProceduralHbaIcon() {
    QPixmap pix(16, 16); pix.fill(Qt::transparent);
    QPainter p(&pix); p.setBrush(Qt::darkGreen); p.drawRect(1, 4, 14, 8);
    return QIcon(pix);
}

QIcon MainWindow::createProceduralDriveIcon() {
    QPixmap pix(16, 16); pix.fill(Qt::transparent);
    QPainter p(&pix); p.setBrush(Qt::gray); p.drawRoundedRect(3, 1, 10, 14, 2, 2);
    return QIcon(pix);
}
