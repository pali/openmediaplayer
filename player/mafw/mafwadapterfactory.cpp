#include "mafwadapterfactory.h"

MafwAdapterFactory::MafwAdapterFactory(QObject *parent) :
    QObject(parent)
{
    mafwrenderer = new MafwRendererAdapter();
    mafwTrackerSource = new MafwSourceAdapter("Mafw-Tracker-Source");
    mafwRadioSource = new MafwSourceAdapter("Mafw-IRadio-Source");
    mafwUpnpSource = new MafwSourceAdapter("MAFW-UPnP-Control-Source");
    mafwTempSource = new MafwSourceAdapter();
    playlist = new MafwPlaylistAdapter(this, mafwrenderer);

#ifdef MAFW_WORKAROUNDS
    mafwrenderer->playlist = playlist;
#endif
}

MafwRendererAdapter* MafwAdapterFactory::getRenderer()
{
    return mafwrenderer;
}

MafwSourceAdapter* MafwAdapterFactory::getTrackerSource()
{
    return mafwTrackerSource;
}

MafwSourceAdapter* MafwAdapterFactory::getRadioSource()
{
    return mafwRadioSource;
}

MafwSourceAdapter* MafwAdapterFactory::getUpnpSource()
{
    return mafwUpnpSource;
}

MafwSourceAdapter* MafwAdapterFactory::getTempSource()
{
    return mafwTempSource;
}

MafwPlaylistAdapter* MafwAdapterFactory::getPlaylistAdapter()
{
    return playlist;
}

void MafwAdapterFactory::onGetStatus(MafwPlaylist*, uint, MafwPlayState state, const char *, QString)
{
    this->onStateChanged(state);
}

void MafwAdapterFactory::onStateChanged(int MafwState)
{
    this->state = MafwState;
}

int MafwAdapterFactory::mafwState()
{
    return state;
}