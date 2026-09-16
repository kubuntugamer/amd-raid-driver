#ifndef BACKGROUNDRATESWIDGET_H
#define BACKGROUNDRATESWIDGET_H

#include <QWidget>
#include <QSlider>
#include <QSpinBox>

class BackgroundRatesWidget : public QWidget {
    Q_OBJECT
public:
    explicit BackgroundRatesWidget(QWidget *parent = nullptr);
    ~BackgroundRatesWidget();
private:
    void setupUiLayout();
    QSlider *rebuildSlider;
    QSlider *verifySlider;
    QSlider *initSlider;
};

#endif
