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

#include "internetradiowindow.h"

InternetRadioWindow::InternetRadioWindow(QWidget *parent, MafwRendererAdapter* mra, MafwSourceAdapter* msa) :
        QMainWindow(parent),
        ui(new Ui::InternetRadioWindow)
#ifdef MAFW
        ,mafwrenderer(mra),
        mafwTrackerSource(msa)
#endif
{
    ui->setupUi(this);
#ifdef Q_WS_MAEMO_5
    setAttribute(Qt::WA_Maemo5StackedWindow);
#endif
    ui->centralwidget->setLayout(ui->verticalLayout);
    InternetRadioDelegate *delegate = new InternetRadioDelegate(ui->listWidget);
    ui->listWidget->setItemDelegate(delegate);
    window = new RadioNowPlayingWindow(this);
    this->connectSignals();
    this->orientationChanged();
#ifdef MAFW
    if(mafwTrackerSource->isReady())
        this->listStations();
    else
        connect(mafwTrackerSource, SIGNAL(sourceReady()), this, SLOT(listStations()));
#endif
}

InternetRadioWindow::~InternetRadioWindow()
{
    delete ui;
}

void InternetRadioWindow::connectSignals()
{
    connect(ui->actionFM_transmitter, SIGNAL(triggered()), this, SLOT(showFMTXDialog()));
    connect(ui->actionAdd_radio_bookmark, SIGNAL(triggered()), this, SLOT(showAddBookmarkDialog()));
    connect(ui->listWidget, SIGNAL(clicked(QModelIndex)), this, SLOT(onStationSelected()));
    connect(QApplication::desktop(), SIGNAL(resized(int)), this, SLOT(orientationChanged()));
}

void InternetRadioWindow::showFMTXDialog()
{
#ifdef Q_WS_MAEMO_5
    FMTXDialog *fmtxDialog = new FMTXDialog(this);
    fmtxDialog->show();
#endif
}

void InternetRadioWindow::onStationSelected()
{
    mafwrenderer->playObject(ui->listWidget->currentItem()->data(UserRoleObjectID).toString().toUtf8());
    window->show();
}

void InternetRadioWindow::showAddBookmarkDialog()
{
    bookmarkDialog = new QDialog(this);
    bookmarkDialog->setWindowTitle(tr("Add radio bookmark"));
    QRect screenGeometry = QApplication::desktop()->screenGeometry();

    // Labels
    nameLabel = new QLabel(bookmarkDialog);
    nameLabel->setText(tr("Name"));
    addressLabel = new QLabel(bookmarkDialog);
    addressLabel->setText(tr("Web address"));

    // Input boxes
    nameBox = new QLineEdit(bookmarkDialog);
    addressBox = new QLineEdit(bookmarkDialog);
    addressBox->setText("http://");

    // Spacer above save button
    QSpacerItem *verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    // Save button
    saveButton = new QPushButton(bookmarkDialog);
    saveButton->setText(tr("Save"));
    saveButton->setDefault(true);
    connect(saveButton, SIGNAL(clicked()), this, SLOT(onSaveClicked()));
    buttonBox = new QDialogButtonBox(Qt::Vertical);
    buttonBox->addButton(saveButton, QDialogButtonBox::ActionRole);

    // Layouts
    QVBoxLayout *labelLayout = new QVBoxLayout();
    labelLayout->addWidget(nameLabel);
    labelLayout->addWidget(addressLabel);

    QVBoxLayout *boxLayout = new QVBoxLayout();
    boxLayout->addWidget(nameBox);
    boxLayout->addWidget(addressBox);

    QHBoxLayout *dialogLayout = new QHBoxLayout();
    if (screenGeometry.width() < screenGeometry.height()) {
        dialogLayout->setDirection(QBoxLayout::TopToBottom);
    }

    QVBoxLayout *saveButtonLayout = new QVBoxLayout();
    if (screenGeometry.width() > screenGeometry.height()) {
        saveButtonLayout->addItem(verticalSpacer);
    }
    saveButtonLayout->addWidget(buttonBox);

    // Pack all layouts together
    QHBoxLayout *horizontalLayout = new QHBoxLayout();
    horizontalLayout->addItem(labelLayout);
    horizontalLayout->addItem(boxLayout);

    dialogLayout->addItem(horizontalLayout);
    dialogLayout->addItem(saveButtonLayout);

    bookmarkDialog->setLayout(dialogLayout);
    bookmarkDialog->setAttribute(Qt::WA_DeleteOnClose);
    bookmarkDialog->show();
}

void InternetRadioWindow::onSaveClicked()
{
    if(nameBox->text().isEmpty()) {
#ifdef Q_WS_MAEMO_5
        QMaemo5InformationBox::information(this, tr("Unable to add empty bookmark"));
#else
        QMessageBox::critical(this, tr("Error"), tr("Unable to add empty bookmark"));
#endif
    } else {
        if(!saveButton->text().contains("http://")) {
#ifdef Q_WS_MAEMO_5
            QMaemo5InformationBox::information(this, tr("Invalid URL"));
#else
            QMessageBox::critical(this, tr("Error"), tr("Invalid URL"));
#endif
        } else {
            if(!saveButton->text().contains("*.*")) {
#ifdef Q_WS_MAEMO_5
                QMaemo5InformationBox::information(this, tr("Invalid URL"));
#else
                QMessageBox::critical(this, tr("Error"), tr("Invalid URL"));
#endif
            }
        }
    }
}

#ifdef MAFW
void InternetRadioWindow::listStations()
{
#ifdef DEBUG
    qDebug("Source ready");
#endif
    connect(mafwTrackerSource, SIGNAL(signalSourceBrowseResult(uint, int, uint, QString, GHashTable*, QString)),
            this, SLOT(browseAllStations(uint, int, uint, QString, GHashTable*, QString)));

    this->browseAllStationsId = mafwTrackerSource->sourceBrowse("iradiosource::", false, NULL, NULL,
                                                               MAFW_SOURCE_LIST(MAFW_METADATA_KEY_TITLE,
                                                                                MAFW_METADATA_KEY_URI,
                                                                                ),
                                                               0, MAFW_SOURCE_BROWSE_ALL);
}

void InternetRadioWindow::browseAllStations(uint browseId, int, uint, QString objectId, GHashTable* metadata, QString)
{
    if(browseId != browseAllStationsId)
      return;


    QString title;
    QString URI;
    if(metadata != NULL) {
        GValue *v;
        v = mafw_metadata_first(metadata,
                                MAFW_METADATA_KEY_TITLE);
        title = v ? QString::fromUtf8(g_value_get_string (v)) : tr("(unknown station)");

        v = mafw_metadata_first(metadata,
                                MAFW_METADATA_KEY_URI);
        URI = v ? QString::fromUtf8(g_value_get_string (v)) : tr("(unknown)");

        QListWidgetItem *item = new QListWidgetItem(ui->listWidget);

        item->setText(title);
        item->setData(UserRoleSongTitle, title);
        item->setData(UserRoleSongURI, URI);
        item->setData(UserRoleObjectID, objectId);
        ui->listWidget->addItem(item);
  }
}
#endif

void InternetRadioWindow::orientationChanged()
{
    QRect screenGeometry = QApplication::desktop()->screenGeometry();
    ui->indicator->setGeometry(screenGeometry.width()-122, screenGeometry.height()-(70+55),
                               ui->indicator->width(),ui->indicator->height());
    ui->indicator->raise();
}

void InternetRadioWindow::focusInEvent(QFocusEvent *)
{
    ui->indicator->triggerAnimation();
}

void InternetRadioWindow::focusOutEvent(QFocusEvent *)
{
    ui->indicator->stopAnimation();
}
