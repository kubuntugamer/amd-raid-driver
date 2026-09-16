#ifndef SPAREASSIGNMENTDIALOG_H
#define SPAREASSIGNMENTDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QRadioButton>

class SpareAssignmentDialog : public QDialog {
    Q_OBJECT
public:
    explicit SpareAssignmentDialog(QWidget *parent = nullptr);
    ~SpareAssignmentDialog();
private:
    void setupUiLayout();
    QListWidget *eligibleDrivesList;
    QRadioButton *globalSpareRadio;
    QRadioButton *dedicatedSpareRadio;
};

#endif
