#include "ringtonedialog.h"

RingtoneDialog::RingtoneDialog(QWidget *parent,
                               MafwSourceAdapter *mafwSource, QString objectId,
                               QString title, QString artist) :
    ConfirmDialog(Ringtone, parent, title, artist),
    accepted(false),
    objectId(objectId)
{
    this->setAttribute(Qt::WA_DeleteOnClose);

    connect(mafwSource, SIGNAL(signalGotUri(QString,QString)), this, SLOT(onUriReceived(QString,QString)));

    mafwSource->getUri(objectId.toUtf8());
}

void RingtoneDialog::onUriReceived(QString objectId, QString uri)
{
    if (objectId != this->objectId) return;

    this->uri = uri;

    if (accepted)
        setRingtone();
}

void RingtoneDialog::done(int r)
{
    if (r == QMessageBox::Yes) {
        if (uri.isNull()) {
            this->setEnabled(false);
            accepted = true;
        } else {
            setRingtone();
        }
    } else {
        ConfirmDialog::done(r);
    }
}

void RingtoneDialog::setRingtone()
{
    QDBusInterface("com.nokia.profiled",
                   "/com/nokia/profiled",
                   "com.nokia.profiled",
                   QDBusConnection::sessionBus())
    .call("set_value", "general", "ringing.alert.tone", uri);

    QMaemo5InformationBox::information(this->parentWidget(), tr("Selected song set as ringing tone"));

    ConfirmDialog::done(QMessageBox::Yes);
}
