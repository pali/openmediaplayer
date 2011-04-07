#ifndef SINGLEPLAYLISTVIEW_H
#define SINGLEPLAYLISTVIEW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include "ui_singleplaylistview.h"
#include "delegates/songlistitemdelegate.h"

#ifdef Q_WS_MAEMO_5
    #include <QMaemo5InformationBox>
    #include <QMaemo5ValueButton>
#endif

#ifdef MAFW
    #include "mafwrendereradapter.h"
    #include "mafwsourceadapter.h"
    #include "mafwplaylistadapter.h"
#endif

namespace Ui {
    class SinglePlaylistView;
}

class SinglePlaylistView : public QMainWindow
{
    Q_OBJECT

public:
    explicit SinglePlaylistView(QWidget *parent = 0, MafwRendererAdapter* mra = 0, MafwSourceAdapter* msa = 0, MafwPlaylistAdapter* pls = 0);
    ~SinglePlaylistView();
#ifdef MAFW
    void browsePlaylist(MafwPlaylist *mafwplaylist);
    void browseObjectId(QString objectId);
    void browseAutomaticPlaylist(QString filter, QString sorting, int maxCount);
#endif

protected:
    void keyReleaseEvent(QKeyEvent *e);

private:
    Ui::SinglePlaylistView *ui;
#ifdef Q_WS_MAEMO_5
    QMaemo5ValueButton *shuffleAllButton;
    void notifyOnAddedToNowPlaying(int songCount);
#else
    QPushButton *shuffleAllButton;
#endif

#ifdef MAFW
    MafwRendererAdapter *mafwrenderer;
    MafwSourceAdapter *mafwTrackerSource;
    MafwPlaylistAdapter *playlist;
    uint browsePlaylistId;
#endif
    void setSongCount(int count);

private slots:
    void orientationChanged();
#ifdef MAFW
    void onGetItems(QString objectId, GHashTable* metadata, guint index);
    void onBrowseResult(uint browseId, int, uint, QString objectId, GHashTable *metadata, QString);
    void onDeleteUriReceived(QString objectId, QString uri);
    void onShareUriReceived(QString objectId, QString Uri);
    void onRingingToneUriReceived(QString objectId, QString uri);
#endif
    void onItemSelected(QListWidgetItem *);
    void addAllToNowPlaying();
    void onSearchTextChanged(QString text);
    void onShuffleButtonClicked();
    void onContextMenuRequested(const QPoint &point);
    void onAddToNowPlaying();
    void setRingingTone();
    void onShareClicked();
    void onDeleteClicked();
};

#endif // SINGLEPLAYLISTVIEW_H
