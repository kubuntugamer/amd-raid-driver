#ifndef ARRAYOPTIONSWIDGET_H
#define ARRAYOPTIONSWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>

class ArrayOptionsWidget : public QWidget {
    Q_OBJECT
public:
    explicit ArrayOptionsWidget(QWidget *parent = nullptr);
    ~ArrayOptionsWidget();

    QComboBox *raidTypeCombo;
    QComboBox *chunkSizeCombo;
    QLineEdit *capacityEdit;

private:
    void setupUiLayout();
};

#endif
