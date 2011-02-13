#include "nowplayingwindow.h"

NowPlayingWindow::NowPlayingWindow(QWidget *parent, MafwRendererAdapter* mra, MafwSourceAdapter* msa) :
    QMainWindow(parent),
#ifdef MAFW
    ui(new Ui::NowPlayingWindow),
    mafwrenderer(mra),
    mafwTrackerSource(msa)
#else
    ui(new Ui::NowPlayingWindow)
#endif
{
    ui->setupUi(this);
#ifdef Q_WS_MAEMO_5
    setAttribute(Qt::WA_Maemo5StackedWindow);
#endif
    positionTimer = new QTimer(this);
    positionTimer->setInterval(1000);
    albumArtScene = new QGraphicsScene(ui->view);
    ui->volumeSlider->hide();
    PlayListDelegate *delegate = new PlayListDelegate(ui->songPlaylist);
    ui->songPlaylist->setItemDelegate(delegate);
    this->setButtonIcons();
    ui->buttonsWidget->setLayout(ui->buttonsLayout);
    ui->songPlaylist->hide();
    QMainWindow::setCentralWidget(ui->verticalWidget);
    volumeTimer = new QTimer(this);
    volumeTimer->setInterval(3000);
    this->connectSignals();
    ui->shuffleButton->setFixedSize(ui->shuffleButton->sizeHint());
    ui->repeatButton->setFixedSize(ui->repeatButton->sizeHint());
    ui->volumeButton->setFixedSize(ui->volumeButton->sizeHint());
    ui->view->setFixedHeight(350);
    // We might be starting NowPlayingWindow in portrait mode.
    QRect screenGeometry = QApplication::desktop()->screenGeometry();
    if (screenGeometry.width() < screenGeometry.height()) {
        ui->horizontalLayout_3->setDirection(QBoxLayout::TopToBottom);
        if(!ui->volumeButton->isHidden())
            ui->volumeButton->hide();
        ui->layoutWidget->setGeometry(QRect(ui->layoutWidget->rect().left(),
                                            ui->layoutWidget->rect().top(),
                                            440,
                                            320));
        ui->view->setFixedHeight(360);
        ui->buttonsLayout->setSpacing(30);
    }
    ui->toolBarWidget->setFixedHeight(ui->toolBarWidget->sizeHint().height());
#ifdef MAFW
    mafwrenderer->getCurrentMetadata();
    mafwrenderer->getStatus();
    mafwrenderer->getPosition();
#endif
}

NowPlayingWindow::~NowPlayingWindow()
{
    delete ui;
}

void NowPlayingWindow::setAlbumImage(QString image)
{
    qDeleteAll(albumArtScene->items());
    ui->view->setScene(albumArtScene);
    albumArtScene->setBackgroundBrush(QBrush(Qt::transparent));
    m = new mirror();
    albumArtScene->addItem(m);
    QPixmap albumArt(image);
    albumArt = albumArt.scaled(QSize(295, 295));
    QGraphicsPixmapItem* item = new QGraphicsPixmapItem(albumArt);
    albumArtScene->addItem(item);
    m->setItem(item);
    /*QTransform t;
    t = t.rotate(-10, Qt::YAxis);
    ui->view->setTransform(t);*/
}

void NowPlayingWindow::toggleVolumeSlider()
{
    if(ui->volumeSlider->isHidden()) {
        ui->buttonsWidget->hide();
        ui->volumeSlider->show();
    } else {
        ui->volumeSlider->hide();
        ui->buttonsWidget->show();
        if(volumeTimer->isActive())
            volumeTimer->stop();
    }
}

#ifdef MAFW
void NowPlayingWindow::onVolumeChanged(const QDBusMessage &msg)
{
    /*dbus-send --print-reply --type=method_call --dest=com.nokia.mafw.renderer.Mafw-Gst-Renderer-Plugin.gstrenderer \
                 /com/nokia/mafw/renderer/gstrenderer com.nokia.mafw.extension.get_extension_property string:volume*/
    if (msg.arguments()[0].toString() == "volume") {
        int volumeLevel = qdbus_cast<QVariant>(msg.arguments()[1]).toInt();
#ifdef DEBUG
        qDebug() << QString::number(volumeLevel);
#endif
        ui->volumeSlider->setValue(volumeLevel);
    }
}
#endif

void NowPlayingWindow::setButtonIcons()
{
    this->setAlbumImage(albumImage);
    ui->prevButton->setIcon(QIcon(prevButtonIcon));
    ui->playButton->setIcon(QIcon(playButtonIcon));
    ui->nextButton->setIcon(QIcon(nextButtonIcon));
    ui->shuffleButton->setIcon(QIcon(shuffleButtonIcon));
    ui->repeatButton->setIcon(QIcon(repeatButtonIcon));
    ui->volumeButton->setIcon(QIcon(volumeButtonIcon));
}

void NowPlayingWindow::metadataChanged(QString name, QVariant value)
{
    if(name == "title" /*MAFW_METADATA_KEY_TITLE*/) {
        ui->songTitleLabel->setText(value.toString());
        system(QString("fmtx_client -s '" + value.toString() + "' -t '" + value.toString() + "' > /dev/null &").toUtf8());
    }
    if(name == "artist" /*MAFW_METADATA_KEY_ARTIST*/)
        ui->artistLabel->setText(value.toString());
    if(name == "album" /*MAFW_METADATA_KEY_ALBUM*/)
        ui->albumNameLabel->setText(value.toString());
    if(name == "renderer-art-uri")
        this->setAlbumImage(value.toString());
}

#ifdef MAFW
void NowPlayingWindow::stateChanged(int state)
{
    this->mafwState = state;
  if(state == Paused)
  {
      ui->playButton->setIcon(QIcon(playButtonIcon));
      disconnect(ui->playButton, SIGNAL(clicked()), 0, 0);
      connect(ui->playButton, SIGNAL(clicked()), mafwrenderer, SLOT(resume()));
      if(positionTimer->isActive())
          positionTimer->stop();
  }
  else if(state == Playing)
  {
      ui->playButton->setIcon(QIcon(pauseButtonIcon));
      disconnect(ui->playButton, SIGNAL(clicked()), 0, 0);
      connect(ui->playButton, SIGNAL(clicked()), mafwrenderer, SLOT(pause()));
      if(!positionTimer->isActive())
          positionTimer->start();
  }
  else if(state == Stopped)
  {
      ui->playButton->setIcon(QIcon(playButtonIcon));
      disconnect(ui->playButton, SIGNAL(clicked()), 0, 0);
      connect(ui->playButton, SIGNAL(clicked()), mafwrenderer, SLOT(play()));
      if(positionTimer->isActive())
          positionTimer->stop();
  }
  else if(state == Transitioning) {
      ui->songProgress->setEnabled(false);
      ui->songProgress->setValue(0);
      ui->songProgress->setRange(0, 99);
  }
}
#endif

void NowPlayingWindow::connectSignals()
{
    connect(ui->volumeButton, SIGNAL(clicked()), this, SLOT(toggleVolumeSlider()));
    connect(ui->actionFM_Transmitter, SIGNAL(triggered()), this, SLOT(showFMTXDialog()));
    connect(QApplication::desktop(), SIGNAL(resized(int)), this, SLOT(orientationChanged()));
    connect(ui->volumeButton, SIGNAL(clicked()), this, SLOT(volumeWatcher()));
    connect(volumeTimer, SIGNAL(timeout()), this, SLOT(toggleVolumeSlider()));
    connect(ui->volumeSlider, SIGNAL(sliderPressed()), volumeTimer, SLOT(stop()));
    connect(ui->volumeSlider, SIGNAL(sliderReleased()), volumeTimer, SLOT(start()));
    connect(ui->view, SIGNAL(clicked()), this, SLOT(toggleList()));
    connect(ui->repeatButton, SIGNAL(clicked()), this, SLOT(onRepeatButtonPressed()));
    connect(ui->shuffleButton, SIGNAL(clicked()), this, SLOT(onShuffleButtonPressed()));
    connect(ui->nextButton, SIGNAL(pressed()), this, SLOT(onNextButtonPressed()));
    connect(ui->nextButton, SIGNAL(released()), this, SLOT(onNextButtonPressed()));
    connect(ui->prevButton, SIGNAL(pressed()), this, SLOT(onPrevButtonPressed()));
    connect(ui->prevButton, SIGNAL(released()), this, SLOT(onPrevButtonPressed()));
    connect(ui->songProgress, SIGNAL(sliderPressed()), this, SLOT(onSliderPressed()));
    connect(ui->songProgress, SIGNAL(sliderReleased()), this, SLOT(onSliderReleased()));
#ifdef MAFW
    connect(mafwrenderer, SIGNAL(stateChanged(int)), this, SLOT(stateChanged(int)));
    connect(mafwrenderer, SIGNAL(metadataChanged(QString, QVariant)), this, SLOT(metadataChanged(QString, QVariant)));
    connect(mafwrenderer, SIGNAL(signalGetPosition(int,QString)), this, SLOT(onPositionChanged(int, QString)));
    connect(mafwrenderer, SIGNAL(signalGetPosition(int,QString)), this, SLOT(updateProgressBar(int,QString)));
    connect(mafwrenderer, SIGNAL(signalGetStatus(MafwPlaylist*,uint,MafwPlayState,const char*,QString)),
            this, SLOT(onGetStatus(MafwPlaylist*,uint,MafwPlayState,const char*,QString)));
    connect(mafwrenderer, SIGNAL(signalGetCurrentMetadata(QString,QString,QString,int,QString)),
            this, SLOT(onMetadataRequested(QString,QString,QString,int,QString)));
    connect(mafwrenderer, SIGNAL(mediaIsSeekable(bool)), ui->songProgress, SLOT(setEnabled(bool)));
    connect(ui->playButton, SIGNAL(clicked()), mafwrenderer, SLOT(play()));
    connect(ui->nextButton, SIGNAL(clicked()), mafwrenderer, SLOT(next()));
    connect(ui->prevButton, SIGNAL(clicked()), mafwrenderer, SLOT(previous()));
    connect(positionTimer, SIGNAL(timeout()), mafwrenderer, SLOT(getPosition()));
    QDBusConnection::sessionBus().connect("com.nokia.mafw.renderer.Mafw-Gst-Renderer-Plugin.gstrenderer",
                                          "/com/nokia/mafw/renderer/gstrenderer",
                                          "com.nokia.mafw.extension",
                                          "property_changed",
                                          this, SLOT(onVolumeChanged(const QDBusMessage &)));
#endif
}

void NowPlayingWindow::showFMTXDialog()
{
#ifdef Q_WS_MAEMO_5
    FMTXDialog *fmtxDialog = new FMTXDialog(this);
    fmtxDialog->show();
#endif
}

void NowPlayingWindow::onSongSelected(int songNumber, int totalNumberOfSongs, QString song, QString album, QString artist, int duration)
{
    this->setAlbumImage(albumImage);
    ui->songNumberLabel->setText(QString::number(songNumber) + "/" + QString::number(totalNumberOfSongs) + tr(" songs"));
    ui->songTitleLabel->setText(song);
    ui->albumNameLabel->setText(album);
    ui->artistLabel->setText(artist);
    this->songDuration = duration;
    QTime t(0, 0);
    t = t.addSecs(duration);
    ui->trackLengthLabel->setText(t.toString("mm:ss"));
    ui->songPlaylist->setCurrentRow(songNumber-1);
    if(!ui->songPlaylist->isHidden()) {
        ui->songPlaylist->hide();
        ui->scrollArea->show();
    }
}

#ifdef MAFW
void NowPlayingWindow::onMetadataRequested(QString name, QString album, QString artist, int, QString error)
{
    ui->songTitleLabel->setText(name);
    ui->albumNameLabel->setText(album);
    ui->artistLabel->setText(artist);
    if(!error.isEmpty())
        qDebug() << error;
}
#endif

void NowPlayingWindow::orientationChanged()
{
    QRect screenGeometry = QApplication::desktop()->screenGeometry();
    if (screenGeometry.width() > screenGeometry.height()) {
        // Landscape mode
#ifdef DEBUG
        qDebug() << "NowPlayingWindow: Orientation changed: Landscape.";
#endif
        ui->horizontalLayout_3->setDirection(QBoxLayout::LeftToRight);
        //ui->buttonsLayout->addItem(ui->horizontalSpacer_10);
        if(ui->volumeButton->isHidden())
            ui->volumeButton->show();
        ui->layoutWidget->setGeometry(QRect(0, 0, 372, 351));
        ui->view->setFixedHeight(360);
        ui->buttonsLayout->setSpacing(60);
    } else {
        // Portrait mode
#ifdef DEBUG
        qDebug() << "NowPlayingWindow: Orientation changed: Portrait.";
#endif
        ui->horizontalLayout_3->setDirection(QBoxLayout::TopToBottom);
        //ui->buttonsLayout->removeItem(ui->horizontalSpacer_10);
        if(!ui->volumeButton->isHidden())
            ui->volumeButton->hide();
        ui->layoutWidget->setGeometry(QRect(ui->layoutWidget->rect().left(),
                                            ui->layoutWidget->rect().top(),
                                            440,
                                            320));
        ui->view->setFixedHeight(360);
        ui->buttonsLayout->setSpacing(30);
    }
}

void NowPlayingWindow::listSongs(QString fileName)
{
    QListWidgetItem *songItem = new QListWidgetItem(fileName);
    songItem->setData(UserRoleSongName, fileName);
    ui->songPlaylist->addItem(songItem);
}

void NowPlayingWindow::toggleList()
{
    if(ui->songPlaylist->isHidden()) {
        /* Hide scrollArea first, then show playlist otherwise
           the other labels will move a bit in portrait mode   */
        ui->scrollArea->hide();
        ui->songPlaylist->show();
    } else {
        ui->songPlaylist->hide();
        ui->scrollArea->show();
    }
}

void NowPlayingWindow::volumeWatcher()
{
    if(!ui->volumeSlider->isHidden())
        volumeTimer->start();
}

void NowPlayingWindow::onShuffleButtonPressed()
{
    if(ui->shuffleButton->isChecked()) {
        ui->shuffleButton->setIcon(QIcon(shuffleButtonPressed));
    } else {
        ui->shuffleButton->setIcon(QIcon(shuffleButtonIcon));
    }
}

void NowPlayingWindow::onRepeatButtonPressed()
{
    if(ui->repeatButton->isChecked()) {
        ui->repeatButton->setIcon(QIcon(repeatButtonPressedIcon));
    } else {
        ui->repeatButton->setIcon(QIcon(repeatButtonIcon));
    }
}

void NowPlayingWindow::onNextButtonPressed()
{
    if(ui->nextButton->isDown())
        ui->nextButton->setIcon(QIcon(nextButtonPressedIcon));
    else
        ui->nextButton->setIcon(QIcon(nextButtonIcon));
}

void NowPlayingWindow::onPrevButtonPressed()
{
    if(ui->prevButton->isDown())
        ui->prevButton->setIcon(QIcon(prevButtonPressedIcon));
    else
        ui->prevButton->setIcon(QIcon(prevButtonIcon));
}

void NowPlayingWindow::onSliderPressed()
{
#ifdef MAFW
    disconnect(mafwrenderer, SIGNAL(signalGetPosition(int,QString)), this, SLOT(updateProgressBar(int,QString)));
    //connect(ui->songProgress, SIGNAL(valueChanged(int)), this, SLOT(setPosition(int)));
#endif
}

void NowPlayingWindow::onSliderReleased()
{
#ifdef MAFW
    this->setPosition(ui->songProgress->value());
    connect(mafwrenderer, SIGNAL(signalGetPosition(int,QString)), this, SLOT(updateProgressBar(int,QString)));
#endif
}

#ifdef MAFW
void NowPlayingWindow::onPositionChanged(int position, QString)
{
    QTime t(0, 0);
    t = t.addSecs(position);
    ui->currentPositionLabel->setText(t.toString("mm:ss"));
}

void NowPlayingWindow::updateProgressBar(int position, QString)
{
    if(ui->trackLengthLabel->text() != "00:00" || ui->trackLengthLabel->text() != "--:--") {
        if(this->songDuration != 0) {
#ifdef DEBUG
            qDebug() << "Current position: " << position;
            qDebug() << "Song Length: " << this->songDuration;
#endif
            ui->songProgress->setRange(0, this->songDuration);
            ui->songProgress->setValue(position);
        }
    }
}

void NowPlayingWindow::onGetStatus(MafwPlaylist*, uint, MafwPlayState state, const char *, QString)
{
    this->stateChanged(state);
}

void NowPlayingWindow::setPosition(int newPosition)
{
    mafwrenderer->setPosition(SeekAbsolute, newPosition);
    mafwrenderer->getPosition();
}

#ifdef MAFW
void NowPlayingWindow::showEvent(QShowEvent *)
{
    mafwrenderer->getCurrentMetadata();
    mafwrenderer->getStatus();
    mafwrenderer->getPosition();
    if(positionTimer->isActive())
        ui->songProgress->setEnabled(true);
}
#endif

#endif

void NowPlayingWindow::keyPressEvent(QKeyEvent *e)
{
    if(e->key() == Qt::Key_Backspace)
        this->close();
    else if(e->key() == Qt::Key_Space) {
        if(this->mafwState == Playing)
            mafwrenderer->pause();
        else if(this->mafwState == Paused)
            mafwrenderer->resume();
        else if(this->mafwState == Stopped)
            mafwrenderer->play();
    }
    else if(e->key() == Qt::Key_Right)
        mafwrenderer->next();
    else if(e->key() == Qt::Key_Left)
        mafwrenderer->previous();
    else if(e->key() == Qt::Key_Control) {
        if(ui->menuNow_playing_menu->isHidden())
            ui->menuNow_playing_menu->show();
        else
            ui->menuNow_playing_menu->hide();
    }
}
