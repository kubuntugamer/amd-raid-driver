#ifndef CREATEARRAYWIZARD_H
#define CREATEARRAYWIZARD_H

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>

class CreateArrayWizard : public QDialog {
    Q_OBJECT
public:
    explicit CreateArrayWizard(QWidget *parent = nullptr);
    ~CreateArrayWizard();

    QString getArrayName() const;
    int getRaidLevelIndex() const;
    int getStripeSizeIndex() const;

private:
    void setupWizardLayout();

    QLineEdit *arrayNameEdit;
    QComboBox *raidLevelCombo;
    QComboBox *stripeSizeCombo;
    QComboBox *initTypeCombo;
    QListWidget *availableDisksList;
    
    QPushButton *btnNext;
    QPushButton *btnCancel;
};

#endif
