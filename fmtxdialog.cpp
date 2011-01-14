#include "fmtxdialog.h"

FMTXDialog::FMTXDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FMTXDialog),
    selector(new FreqPickSelector(this))
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
#ifdef Q_WS_MAEMO_5
    freqButton = new QMaemo5ValueButton("Frequency", this);
    freqButton->setValueLayout(QMaemo5ValueButton::ValueBesideText);
    freqButton->setPickSelector(selector);
#else
    freqButton = new QPushButton("Frequency", this);
#endif
    ui->buttonBox->setOrientation(Qt::Vertical);
    ui->saveLayout->removeWidget(ui->buttonBox);
    if (QApplication::desktop()->screenGeometry().width() < QApplication::desktop()->screenGeometry().height()) {
        ui->saveLayout->addWidget(ui->buttonBox, 2, 0, 1, 2); // portrait
        ui->buttonBox->setSizePolicy(QSizePolicy::MinimumExpanding, ui->buttonBox->sizePolicy().verticalPolicy());
    } else {
        ui->saveLayout->addWidget(ui->buttonBox, 0, 2, 2, 1, Qt::AlignBottom); // landscape
    }
    fmtxState = new GConfItem("/system/fmtx/enabled");
    fmtxFrequency = new GConfItem("/system/fmtx/frequency");
    if(fmtxState->value().toBool())
        ui->fmtxCheckbox->setChecked(true);
    ui->fmtxLayout->addWidget(freqButton);
    connect(fmtxState, SIGNAL(valueChanged()), this, SLOT(onStateChanged()));
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(onSaveClicked()));
}

FMTXDialog::~FMTXDialog()
{
    delete ui;
}

void FMTXDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
}

void FMTXDialog::onSaveClicked()
{
    int frequencyValue = selector->selectedFreq() * 1000;
    fmtxFrequency->set(frequencyValue);
#ifdef DEBUG
    qDebug() << "Selected Frequency:" << QString::number(frequencyValue);
#endif
    if(ui->fmtxCheckbox->isChecked())
        // TODO: use DBus instead(!)
        system(QString("fmtx_client -p 1 -f %2 > /dev/null &").arg(frequencyValue).toUtf8().constData());
    else
        system(QString("fmtx_client -p 0 -f %2 > /dev/null &").arg(frequencyValue).toUtf8().constData());
    this->close();
}

void FMTXDialog::onStateChanged()
{
    if(fmtxState->value().toBool())
        ui->fmtxCheckbox->setChecked(true);
    else
        ui->fmtxCheckbox->setChecked(false);
}
