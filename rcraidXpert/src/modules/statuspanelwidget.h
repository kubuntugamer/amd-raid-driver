#ifndef STATUSPANELWIDGET_H
#define STATUSPANELWIDGET_H

#include <QGroupBox>
#include <QLabel>

class StatusPanelWidget : public QGroupBox {
    Q_OBJECT
public:
    explicit StatusPanelWidget(QWidget *parent = nullptr);

    bool isKernelVerified() const { return isAuthenticatedState; }

    /* Reflects the live /dev/amd_ctl0 state, not a togglable simulation. */
    void setDriverState(bool present, const QString &detail);
    void setMessage(const QString &message);

private:
    bool isAuthenticatedState = false;
    QLabel *statusIndicator;
    QLabel *statusLabel;
    QLabel *messageLabel;
};

#endif
