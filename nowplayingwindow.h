#ifndef NOWPLAYINGWINDOW_H
#define NOWPLAYINGWINDOW_H

#include <QMainWindow>
#include <QtGui>
#include <QtDBus>
#include <QTimer>

#include "mirror.h"
#include "cqgraphicsview.h"
#include "ui_nowplayingwindow.h"
#include "includes.h"
#include "delegates/playlistdelegate.h"
#include "entertainmentview.h"

#ifdef Q_WS_MAEMO_5
    #include "fmtxdialog.h"
    #include "share.h"
    #include "maemo5deviceevents.h"
#endif

#ifdef MAFW
    #include "mafwrendereradapter.h"
    #include "mafwplaylistadapter.h"
    #include "mafwsourceadapter.h"
    #include <gq/GConfItem>
    #include "mafwplaylistmanageradapter.h"
#else
    class MafwRendererAdapter;
    class MafwSourceAdapter;
    class MafwPlaylistAdapter;
#endif
namespace Ui {
    class NowPlayingWindow;
}

class NowPlayingWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit NowPlayingWindow(QWidget *parent = 0, MafwRendererAdapter* mra = 0, MafwSourceAdapter* msa = 0, MafwPlaylistAdapter* pls = 0);
    ~NowPlayingWindow();

public slots:
    void onSongSelected(int, int, QString, QString, QString, int);
    void setAlbumImage(QString);
    void onShuffleButtonToggled(bool);
#ifdef MAFW
    void updatePlaylistState();
#endif

private:
    Ui::NowPlayingWindow *ui;
    EntertainmentView *entertainmentView;
#ifdef MAFW
    MafwRendererAdapter* mafwrenderer;
    MafwSourceAdapter* mafwTrackerSource;
    MafwPlaylistAdapter *playlist;
    MafwPlaylist *mafwPlaylist;
    MafwPlaylistManagerAdapter *mafw_playlist_manager;
    int mafwState;
    GConfItem *lastPlayingSong;
    void showEvent(QShowEvent *);
    void updatePlaylist();
#endif
    void setButtonIcons();
    void setSongNumber(int currentSong, int numberOfSongs);
    void connectSignals();
    QTimer *volumeTimer;
    QTimer *positionTimer;
    QLineEdit *playlistNameLineEdit;
    bool playlistRequested;
    bool isDefaultArt;
    bool buttonWasDown;
    int songDuration;
    int currentSongPosition;
    QGraphicsScene *albumArtScene;
    QString albumArtUri;
    mirror *m;
    void keyPressEvent(QKeyEvent *);
    QMenu *contextMenu;
    QDialog *savePlaylistDialog;
#ifdef Q_WS_MAEMO_5
    Maemo5DeviceEvents *deviceEvents;
#endif

private slots:
    void toggleVolumeSlider();
    void showFMTXDialog();
    void toggleList();
    void setRingingTone();
#ifdef MAFW
    void onPropertyChanged(const QDBusMessage &msg);
    void stateChanged(int state);
    void onPositionChanged(int, QString);
    void onGetStatus(MafwPlaylist*,uint,MafwPlayState,const char*,QString);
    void onRendererMetadataRequested(GHashTable*, QString object_id, QString);
    void onSourceMetadataRequested(QString, GHashTable*, QString);
    void onGetPlaylistItems(QString object_id, GHashTable *metadata, guint index);
    void setPosition(int);
    void onPlaylistItemActivated(QListWidgetItem*);
    void clearPlaylist();
    void onPlaylistChanged();
    void onGconfValueChanged();
    void onMediaChanged(int index, char*);
    void onNextButtonClicked();
    void onPreviousButtonClicked();
#endif
    void metadataChanged(QString name, QVariant value);
    void volumeWatcher();
    void onRepeatButtonToggled(bool);
    void orientationChanged();
    void onNextButtonPressed();
    void onPrevButtonPressed();
    void onPositionSliderMoved(int position);
    void onContextMenuRequested(const QPoint &point);
    void onShareClicked();
    void showEntertainmentview();
    void updateEntertainmentViewMetadata();
    void nullEntertainmentView();
    void savePlaylist();
    void onSavePlaylistAccepted();
    void onDeleteFromNowPlaying();
    void selectItemByText(int numberInPlaylist);
#ifdef Q_WS_MAEMO_5
    void onScreenLocked(bool locked);
#endif
};

#endif // NOWPLAYINGWINDOW_H
