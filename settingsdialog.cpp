/**************************************************************************
    This file is part of Open MediaPlayer
    Copyright (C) 2010-2011 Mohammad Abu-Garbeyyeh

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

#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose);
    connect(QApplication::desktop(), SIGNAL(resized(int)), this, SLOT(orientationChanged()));
    if (QSettings().value("main/onApplicationExit").toString() == "stop-playback")
        ui->onExitBox->setCurrentIndex(1);
    if (QSettings().contains("lyrics/enable"))
        if (QSettings().value("lyrics/enable").toBool())
            ui->lyricsCheckBox->setChecked(true);
    if (QSettings().contains("FMTX/overrideChecks"))
        if (QSettings().value("FMTX/overrideChecks").toBool())
            ui->fmtxCheckBox->setChecked(true);
    //ui->buttonBox->button(QAbstractButton::)
    this->orientationChanged();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::accept()
{
    if (ui->onExitBox->currentIndex() == 0)
        QSettings().setValue("main/onApplicationExit", "do-nothing");
    else if (ui->onExitBox->currentIndex() == 1)
        QSettings().setValue("main/onApplicationExit", "stop-playback");

    QSettings().setValue("lyrics/enable", ui->lyricsCheckBox->isChecked());
    NowPlayingWindow::destroy();

    QSettings().setValue("FMTX/overrideChecks", ui->fmtxCheckBox->isChecked());

    this->close();
}

void SettingsDialog::orientationChanged()
{
    ui->gridLayout->removeWidget(ui->buttonBox);
    if (QApplication::desktop()->screenGeometry().width() < QApplication::desktop()->screenGeometry().height()) {
        this->setFixedHeight(320);
        ui->gridLayout->addWidget(ui->buttonBox, 3, 0, 1, ui->gridLayout->columnCount()); // portrait
        ui->buttonBox->setSizePolicy(QSizePolicy::MinimumExpanding, ui->buttonBox->sizePolicy().verticalPolicy());
    } else {
        ui->buttonBox->setSizePolicy(QSizePolicy::Maximum, ui->buttonBox->sizePolicy().verticalPolicy());
        ui->gridLayout->addWidget(ui->buttonBox, 2, 1, 1, 1, Qt::AlignBottom); // landscape
        this->setFixedHeight(240);
    }
}
