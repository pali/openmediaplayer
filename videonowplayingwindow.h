#ifndef VIDEONOWPLAYINGWINDOW_H
#define VIDEONOWPLAYINGWINDOW_H

#include <QMainWindow>
#include <QIcon>
#include <QTimer>
#include <QtDBus>
#include <QDesktopWidget>
#include <QGLWidget>
#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>

#include "ui_videonowplayingwindow.h"
#include "includes.h"

#ifdef Q_WS_MAEMO_5
    #include "qmaemo5rotator.h"
#endif

#ifdef MAFW
    #include "mafwrendereradapter.h"
    #include "mafwsourceadapter.h"
    #include "mafwplaylistadapter.h"
#endif

namespace Ui {
    class VideoNowPlayingWindow;
}

class VideoNowPlayingWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit VideoNowPlayingWindow(QWidget *parent = 0, MafwRendererAdapter* mra = 0, MafwSourceAdapter* msa = 0, MafwPlaylistAdapter* pls = 0);
    ~VideoNowPlayingWindow();
    void playObject(QString objectId);

protected:
    void paintEvent(QPaintEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void keyPressEvent(QKeyEvent *);

private:
    Ui::VideoNowPlayingWindow *ui;
    void setIcons();
    void connectSignals();
    void showOverlay(bool show);
    QTimer *volumeTimer;
    QTimer *positionTimer;
    QString objectIdToPlay;
    bool portrait;
    bool isOverlayVisible;
#ifdef Q_WS_MAEMO_5
    QMaemo5Rotator *rotator;
    void setDNDAtom(bool dnd);
#endif
#ifdef MAFW
    MafwRendererAdapter *mafwrenderer;
    MafwSourceAdapter *mafwTrackerSource;
    MafwPlaylistAdapter *playlist;
    int colorkey;
    int mafwState;
    int length;
#endif

private slots:
    void toggleVolumeSlider();
    void volumeWatcher();
    void orientationChanged();
#ifdef MAFW
    void onPropertyChanged(const QDBusMessage &msg);
    void stateChanged(int state);
    void onGetStatus(MafwPlaylist*, uint, MafwPlayState state, const char *, QString);
    void onPositionChanged(int position, QString);
    void onSourceMetadataRequested(QString, GHashTable *metadata, QString error);
#endif
#ifdef Q_WS_MAEMO_5
    void onPortraitMode();
    void onLandscapeMode();
#endif
    void playVideo();
    void onSliderMoved(int);
};

#endif // VIDEONOWPLAYINGWINDOW_H
