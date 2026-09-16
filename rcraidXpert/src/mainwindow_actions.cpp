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
                
                connect(createAction, &QAction::triggered, this, [this]() {
                    CreateArrayWizard wizard(this);
                    if (wizard.exec() == QDialog::Accepted) {
                        addNewArrayToTree();
                    }
                });

                connect(deleteAction, &QAction::triggered, this, [this]() {
                    if (!treeModel || !arrayTreeView) return;

                    QModelIndex currentIdx = arrayTreeView->currentIndex();
                    if (currentIdx.isValid()) {
                        QStandardItem *selectedItem = treeModel->itemFromIndex(currentIdx);
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

// Draw look-alike Blue/Green Status Blocks inside Left Navigation Tree Category Branches
QIcon MainWindow::createProceduralHostIcon() {
    QPixmap pix(12, 12);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setBrush(QColor(0, 120, 215)); // Assigned Member Blue
    p.setPen(QColor(0, 120, 215).darker(150));
    p.drawRect(0, 0, 11, 11);
    return QIcon(pix);
}

QIcon MainWindow::createProceduralHbaIcon() {
    QPixmap pix(12, 12);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setBrush(QColor(16, 124, 16)); // Unassigned/Spare Green
    p.setPen(QColor(16, 124, 16).darker(150));
    p.drawRect(0, 0, 11, 11);
    return QIcon(pix);
}

QIcon MainWindow::createProceduralDriveIcon() {
    QPixmap pix(12, 12);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setBrush(QColor(0, 120, 215)); // Default Logical Node Volume Member Blue
    p.setPen(QColor(0, 120, 215).darker(150));
    p.drawRect(0, 0, 11, 11);
    return QIcon(pix);
}
