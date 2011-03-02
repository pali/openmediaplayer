/**************************************************************************
    This file is part of Open MediaPlayer
    Copyright (C) 2010-2011 Timur Kristof

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

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
    ui->gridLayout->addWidget(freqButton, 1, 0, 1, 1);
    this->orientationChanged();
    fmtxState = new GConfItem("/system/fmtx/enabled");
    fmtxFrequency = new GConfItem("/system/fmtx/frequency");
    if(fmtxState->value().toBool())
        ui->fmtxCheckbox->setChecked(true);
    connect(fmtxState, SIGNAL(valueChanged()), this, SLOT(onStateChanged()));
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(onSaveClicked()));
    connect(QApplication::desktop(), SIGNAL(resized(int)), this, SLOT(orientationChanged()));
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

void FMTXDialog::orientationChanged()
{
    ui->gridLayout->removeWidget(ui->buttonBox);
    if (QApplication::desktop()->screenGeometry().width() < QApplication::desktop()->screenGeometry().height()) {
        this->setFixedHeight(230);
        ui->gridLayout->addWidget(ui->buttonBox, 3, 0, 1, ui->gridLayout->columnCount()); // portrait
        ui->buttonBox->setSizePolicy(QSizePolicy::MinimumExpanding, ui->buttonBox->sizePolicy().verticalPolicy());
    } else {
        ui->buttonBox->setSizePolicy(QSizePolicy::Maximum, ui->buttonBox->sizePolicy().verticalPolicy());
        ui->gridLayout->addWidget(ui->buttonBox, 1, 1, 1, 1, Qt::AlignBottom); // landscape
        this->setFixedHeight(150);
    }
}

void FMTXDialog::keyPressEvent(QKeyEvent *e)
{
    if(e->key() == Qt::Key_Backspace)
        this->close();
}
