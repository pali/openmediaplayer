#ifndef CURRENTPLAYLISTADAPTER_H
#define CURRENTPLAYLISTADAPTER_H

#include "mafw/mafwplaylistadapter.h"

#include "mafw/mafwrendereradapter.h"

class CurrentPlaylistAdapter : public MafwPlaylistAdapter
{
    Q_OBJECT

public:
    CurrentPlaylistAdapter(MafwRendererAdapter *renderer, QObject *parent);

    bool isReady();

    void assignAudioPlaylist();
    void assignRadioPlaylist();
    void assignVideoPlaylist();

private:
    MafwRendererAdapter *renderer;

private slots:
    void onStatusReceived(MafwPlaylist *playlist, uint, MafwPlayState, const char *, QString);
    void onPlaylistChanged(GObject *playlist);
};

#endif // CURRENTPLAYLISTADAPTER_H