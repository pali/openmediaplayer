#include "carview.h"

#ifdef Q_WS_MAEMO_5
#include <QtGui/QX11Info>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#endif

CarView::CarView(QWidget *parent, MafwAdapterFactory *factory ) :
    QMainWindow(parent),
    ui(new Ui::CarView)
#ifdef MAFW
    ,mafwFactory(factory),
    mafwrenderer(factory->getRenderer()),
    mafwTrackerSource(factory->getTrackerSource()),
    playlist(factory->getPlaylistAdapter())
#endif
{
    ui->setupUi(this);
    ui->declarativeView->setResizeMode(QDeclarativeView::SizeRootObjectToView);
#ifdef Q_WS_MAEMO_5
    setAttribute(Qt::WA_Maemo5StackedWindow);
    setAttribute(Qt::WA_Maemo5NonComposited);
#endif
    QGLWidget *glWidget = new QGLWidget(this);
    ui->declarativeView->setViewport(glWidget);

    positionTimer = new QTimer(this);
    positionTimer->setInterval(1000);

    rootObject = dynamic_cast<QObject*>(ui->declarativeView->rootObject());
    rootObject->setParent(this);

    connect(rootObject, SIGNAL(quitButtonClicked()), this, SLOT(close()));
    connect(rootObject, SIGNAL(prevButtonClicked()), mafwrenderer, SLOT(previous()));
    connect(rootObject, SIGNAL(playButtonClicked()), this, SLOT(onPlayClicked()));
    connect(rootObject, SIGNAL(nextButtonClicked()), mafwrenderer, SLOT(next()));
    connect(rootObject, SIGNAL(sliderValueChanged(int)), this, SLOT(onSliderValueChanged(int)));
    connect(rootObject, SIGNAL(playlistItemSelected(int)), this, SLOT(onPlaylistItemChanged(int)));
    connect(this, SIGNAL(titleChanged(QVariant)), rootObject, SLOT(setSongTitle(QVariant)));
    connect(this, SIGNAL(albumChanged(QVariant)), rootObject, SLOT(setSongAlbum(QVariant)));
    connect(this, SIGNAL(artistChanged(QVariant)), rootObject, SLOT(setSongArtist(QVariant)));
    connect(this, SIGNAL(albumArtChanged(QVariant)), rootObject, SLOT(setAlbumArt(QVariant)));
    connect(this, SIGNAL(durationTextChanged(QVariant)), rootObject, SLOT(setPosition(QVariant)));
    connect(this, SIGNAL(positionChanged(QVariant)), rootObject, SLOT(setSliderValue(QVariant)));
    connect(this, SIGNAL(durationChanged(QVariant)), rootObject, SLOT(setSliderMaximum(QVariant)));
    connect(this, SIGNAL(stateIconChanged(QVariant)), rootObject, SLOT(setPlayButtonIcon(QVariant)));
    connect(this, SIGNAL(addToPlaylist(QVariant,QVariant,QVariant,QVariant)),
            rootObject, SLOT(addPlaylistItem(QVariant,QVariant,QVariant,QVariant)));
    connect(this, SIGNAL(rowChanged(QVariant)), rootObject, SLOT(onRowChanged(QVariant)));
    connect(mafwrenderer, SIGNAL(stateChanged(int)), this, SLOT(stateChanged(int)));
    connect(mafwrenderer, SIGNAL(signalGetPosition(int,QString)), this, SLOT(onPositionChanged(int,QString)));
    connect(mafwrenderer, SIGNAL(signalGetStatus(MafwPlaylist*,uint,MafwPlayState,const char*,QString)),
            this, SLOT(onGetStatus(MafwPlaylist*,uint,MafwPlayState,const char*,QString)));
    connect(positionTimer, SIGNAL(timeout()), mafwrenderer, SLOT(getPosition()));

    positionTimer->start();


#ifdef Q_WS_MAEMO_5
    quint32 disable = {0};
    Atom winPortraitModeSupportAtom = XInternAtom(QX11Info::display(), "_HILDON_PORTRAIT_MODE_SUPPORT", false);
    XChangeProperty(QX11Info::display(), winId(), winPortraitModeSupportAtom, XA_CARDINAL, 32, PropModeReplace, (uchar*) &disable, 1);

    this->setDNDAtom(true);
#endif

#ifdef MAFW
    mafwrenderer->getStatus();
    mafwrenderer->getPosition();
#endif
}

CarView::~CarView()
{
    delete ui;
}

void CarView::setMetadata(QString songName, QString albumName, QString artistName, QString albumArtUri, int duration)
{
    this->title = songName;
    this->album = albumName;
    this->artist = artistName;
    this->albumArt = albumArtUri;
    this->duration = duration;
    this->songDuration = duration;

    emit titleChanged(this->title);
    emit albumChanged(this->album);
    emit artistChanged(this->artist);
    emit albumArtChanged(this->albumArt);
    emit durationChanged(this->songDuration);
}

#ifdef Q_WS_MAEMO_5
void CarView::setDNDAtom(bool dnd)
{
    quint32 enable;
    if (dnd)
        enable = 1;
    else
        enable = 0;
    Atom winDNDAtom = XInternAtom(QX11Info::display(), "_HILDON_DO_NOT_DISTURB", false);
    XChangeProperty(QX11Info::display(), winId(), winDNDAtom, XA_INTEGER, 32, PropModeReplace, (uchar*) &enable, 1);
}
#endif

#ifdef MAFW
void CarView::onPositionChanged(int position, QString)
{
    duration = time_mmss(position) + "/" + time_mmss(songDuration);
    emit durationTextChanged(duration);
    emit positionChanged(position);
}

void CarView::stateChanged(int state)
{
    this->mafwState = state;
    QString playButtonIconString;

    if(state == Paused) {
        playButtonIconString = playButtonIcon;
        /*ui->playButton->setIcon(QIcon(playButtonIcon));
        disconnect(ui->playButton, SIGNAL(clicked()), 0, 0);
        connect(ui->playButton, SIGNAL(clicked()), mafwrenderer, SLOT(resume()));*/
        mafwrenderer->getPosition();
        if(positionTimer->isActive())
            positionTimer->stop();
    }
    else if(state == Playing) {
        playButtonIconString = QString(pauseButtonIcon).prepend("file://");
        /*ui->playButton->setIcon(QIcon(pauseButtonIcon));
        disconnect(ui->playButton, SIGNAL(clicked()), 0, 0);
        connect(ui->playButton, SIGNAL(clicked()), mafwrenderer, SLOT(pause()));*/
        mafwrenderer->getPosition();
        if(!positionTimer->isActive())
            positionTimer->start();
    }
    else if(state == Stopped) {
        playButtonIconString = QString(playButtonIcon).prepend("file://");
        /*ui->playButton->setIcon(QIcon(playButtonIcon));
        disconnect(ui->playButton, SIGNAL(clicked()), 0, 0);
        connect(ui->playButton, SIGNAL(clicked()), mafwrenderer, SLOT(play()));*/
        if(positionTimer->isActive())
            positionTimer->stop();
    }
    else if(state == Transitioning) {
        /*ui->songProgress->setEnabled(false);
        ui->songProgress->setValue(0);
        ui->songProgress->setRange(0, 99);
        ui->currentPositionLabel->setText("00:00");*/
    }
    emit stateIconChanged(playButtonIconString);
}

void CarView::onGetStatus(MafwPlaylist*,uint,MafwPlayState state,const char*,QString)
{
    this->stateChanged(state);
}

void CarView::onPlayClicked()
{
    if (this->mafwState == Playing)
        mafwrenderer->pause();
    else if (this->mafwState == Stopped)
        mafwrenderer->play();
    else if (this->mafwState == Paused)
        mafwrenderer->resume();
}

void CarView::onSliderValueChanged(int position)
{
    mafwrenderer->setPosition(SeekAbsolute, position);
}

void CarView::addItemToPlaylist(QListWidgetItem *item)
{
    int duration = item->data(UserRoleSongDuration).toInt();
    emit addToPlaylist(item->data(UserRoleSongTitle).toString(),
                       QVariant(item->data(UserRoleSongArtist).toString() + " / " + item->data(UserRoleSongAlbum).toString()),
                       time_mmss(duration),
                       item->text().toInt());
}

void CarView::onPlaylistItemChanged(int index)
{
    mafwrenderer->gotoIndex(index);
}

void CarView::setCurrentRow(int row)
{
    emit rowChanged(row);
}

#endif
