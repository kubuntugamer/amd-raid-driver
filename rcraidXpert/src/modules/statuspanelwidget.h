#ifndef STATUSPANELWIDGET_H
#define STATUSPANELWIDGET_H

#include <QGroupBox>
#include <QLabel>

class StatusPanelWidget : public QGroupBox {
    Q_OBJECT
public:
    explicit StatusPanelWidget(QWidget *parent = nullptr);
    bool isKernelVerified() const;
protected:
    void mousePressEvent(QMouseEvent *event) override;
private:
    void updateVisualState();
    bool isAuthenticatedState;
    QLabel *statusIndicator;
    QLabel *statusLabel;
};

#endif
