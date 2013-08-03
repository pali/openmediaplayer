#ifndef SLEEPERDIALOG_H
#define SLEEPERDIALOG_H

#include <QTimer>
#include <QDialog>
#include <QSettings>
#include <QDateTime>
#include <QAbstractButton>
#include <QStandardItemModel>
#include <QMaemo5ListPickSelector>
#include <QKeyEvent>

#include "includes.h"
#include "rotator.h"

namespace Ui {
    class SleeperDialog;
}

class SleeperDialog : public QDialog
{
    Q_OBJECT

public:
    enum VolumeReduction {
        NoReduction = 0,
        LinearReduction,
        ExponentialReduction
    };

    explicit SleeperDialog(QWidget *parent = 0);
    ~SleeperDialog();

public slots:
    void setTimeoutStamp(qint64 timeoutStamp);

signals:
    void timerRequested(int interval, int reduction);

private:
    Ui::SleeperDialog *ui;
    void keyPressEvent(QKeyEvent *e);
    QTimer *refreshTimer;
    qint64 timeoutStamp;

private slots:
    void refreshTitle();
    void onButtonClicked(QAbstractButton *button);
    void orientationChanged(int h, int w);
};

#endif // SLEEPERDIALOG_H