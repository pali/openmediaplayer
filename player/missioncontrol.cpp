#include "missioncontrol.h"

#define HAL_PATH_RX51_JACK "/org/freedesktop/Hal/devices/platform_soc_audio_logicaldev_input"

MissionControl* MissionControl::instance = NULL;

MissionControl* MissionControl::acquire()
{
    return instance ? instance : instance = new MissionControl();
}

MissionControl::MissionControl() :
    m_metadataWatcher(NULL),
    m_lyricsManager(NULL),
    m_sleeper(NULL)
{
    mafwState = Transitioning;

    pausedByCall = false;
    wasRinging = false;

    wiredHeadsetIsConnected = false;
    headsetPauseStamp = -1;
    wirelessResumeTimer = NULL;

}

void MissionControl::setFactory(MafwAdapterFactory *factory)
{
    mafwFactory = factory;
    mafwRenderer = factory->getRenderer();

    m_metadataWatcher = new MetadataWatcher(factory);

    connect(m_metadataWatcher, SIGNAL(metadataReady()), this, SLOT(onMetadataReady()));
    connect(mafwRenderer, SIGNAL(mediaChanged(int,char*)), this, SLOT(onMediaChanged()));

    connect(mafwRenderer, SIGNAL(rendererReady()), mafwRenderer, SLOT(getStatus()));
    connect(mafwRenderer, SIGNAL(signalGetStatus(MafwPlaylist*,uint,MafwPlayState,const char*,QString)),
            this, SLOT(onStatusReceived(MafwPlaylist*,uint,MafwPlayState,const char*,QString)));

    QDBusConnection::systemBus().connect("", "", "org.bluez.AudioSink", "Connected",
                                         this, SLOT(onWirelessHeadsetConnected()));
    QDBusConnection::systemBus().connect("", "", "org.bluez.AudioSink", "Disconnected",
                                         this, SLOT(onHeadsetDisconnected()));
    QDBusConnection::systemBus().connect("", "", "org.bluez.Headset", "Connected",
                                         this, SLOT(onWirelessHeadsetConnected()));
    QDBusConnection::systemBus().connect("", "", "org.bluez.Headset", "Disconnected",
                                         this, SLOT(onHeadsetDisconnected()));

    QDBusConnection::systemBus().connect("", "/org/freedesktop/Hal/devices/platform_headphone", "org.freedesktop.Hal.Device", "PropertyModified",
                                         this, SLOT(updateWiredHeadset()));

    QDBusConnection::systemBus().connect("", "", "org.freedesktop.Hal.Device", "Condition",
                                         this, SLOT(onHeadsetButtonPressed(QDBusMessage)));

    QDBusConnection::systemBus().connect("", "", "com.nokia.mce.signal", "sig_call_state_ind",
                                         this, SLOT(onCallStateChanged(QDBusMessage)));

    updateWiredHeadset();
}

void MissionControl::reloadSettings()
{
    if (m_lyricsManager)
        m_lyricsManager->deleteLater();

    // Replacing an old instance with a new one will cause depending objects to
    // be disconnected, but this should not really happen due to NowPlayingWindow
    // also being destroyed when reloading (in MainWindow). Anyway, it still
    // might be a good idea to implement online reloading of plugins.
    m_lyricsManager = QSettings().value("lyrics/enable").toBool()
                    ? new LyricsManager(this)
                    : NULL;
}

void MissionControl::togglePlayback()
{
    if (mafwState == Playing) {
        mafwRenderer->pause();
    } else if (mafwState == Paused) {
        mafwRenderer->resume();
    } else if (mafwState == Stopped) {
        mafwRenderer->play();
    }
}

LyricsManager* MissionControl::lyricsManager()
{
    return m_lyricsManager;
}

MetadataWatcher* MissionControl::metadataWatcher()
{
    return m_metadataWatcher;
}

Sleeper* MissionControl::sleeper()
{
    if (!m_sleeper) {
        m_sleeper = new Sleeper(this, mafwRenderer);
        connect(m_sleeper, SIGNAL(finished()), this, SLOT(onSleeperTimeout()));
    }

    return m_sleeper;
}

void MissionControl::onSleeperTimeout()
{
    m_sleeper->deleteLater();
    m_sleeper = NULL;
}


void MissionControl::onMediaChanged()
{
    disconnect(m_metadataWatcher, SIGNAL(metadataChanged(QString,QVariant)),
               this, SLOT(onMetadataChanged(QString,QVariant)));
}

void MissionControl::onMetadataReady()
{
    currentArtist = m_metadataWatcher->metadata().value(MAFW_METADATA_KEY_ARTIST).toString();
    currentTitle = m_metadataWatcher->metadata().value(MAFW_METADATA_KEY_TITLE).toString();

    if (m_lyricsManager)
        m_lyricsManager->fetchLyrics(currentArtist, currentTitle);

    connect(m_metadataWatcher, SIGNAL(metadataChanged(QString,QVariant)),
            this, SLOT(onMetadataChanged(QString,QVariant)));
}

void MissionControl::onMetadataChanged(QString key, QVariant value)
{
    // Fetch lyrics if enough details become available
    if (!value.isNull()) {
        if (key == MAFW_METADATA_KEY_ARTIST) {
            currentArtist = value.toString();
            if (m_lyricsManager)
                m_lyricsManager->fetchLyrics(currentArtist, currentTitle);
        }
        else if (key == MAFW_METADATA_KEY_TITLE) {
            currentTitle = value.toString();
            if (m_lyricsManager)
                m_lyricsManager->fetchLyrics(currentArtist, currentTitle);
        }
    }
}

void MissionControl::onStatusReceived(MafwPlaylist *, uint, MafwPlayState state, const char *, QString)
{
    disconnect(mafwRenderer, SIGNAL(signalGetStatus(MafwPlaylist*,uint,MafwPlayState,const char*,QString)),
               this, SLOT(onStatusReceived(MafwPlaylist*,uint,MafwPlayState,const char*,QString)));

    connect(mafwRenderer, SIGNAL(stateChanged(int)), this, SLOT(onStateChanged(int)));

    onStateChanged(state);

}

void MissionControl::onStateChanged(int state)
{
    mafwState = state;

    if (state == Playing) {
        headsetPauseStamp = -1;
        pausedByCall = false;
    }
}

void MissionControl::onCallStateChanged(QDBusMessage msg)
{
    QString state = msg.arguments()[0].toString();

    if (state == "ringing") {
        wasRinging = true;
        pausedByCall = mafwState == Playing;
        if (pausedByCall) mafwRenderer->pause();
    }
    else if (!wasRinging && state == "active") {
        pausedByCall = mafwState == Playing;
        if (pausedByCall) mafwRenderer->pause();
    }
    else if (state == "none") {
        if (pausedByCall && headsetPauseStamp == -1)
            mafwRenderer->resume();
        pausedByCall = false;
        wasRinging = false;
    }
}

void MissionControl::handlePhoneButton()
{
    QString action = QSettings().value("main/headsetButtonAction", "next").toString();
    if (action == "next") {
        if (mafwState == Playing) {
            mafwRenderer->next();
        } else {
            togglePlayback();
        }
    } else if (action == "previous") {
        mafwRenderer->previous();
    } else if (action == "playpause") {
        togglePlayback();
    } else if (action == "stop") {
        mafwRenderer->stop();
    }
}

// The purpose of distinction between wired and wireless headset is to postpone
// playback resuming when Bluetooth headset is connected. This is needed because
// the signal from Bluetooth daemon arrives before audio system is reconfigured,
// which results in OMP playing via internal speakers for 1-2 seconds.
void MissionControl::onWirelessHeadsetConnected()
{
    if (wirelessResumeTimer) {
        wirelessResumeTimer->stop();
    } else {
        wirelessResumeTimer = new QTimer(this);
        wirelessResumeTimer->setSingleShot(true);
        connect(wirelessResumeTimer, SIGNAL(timeout()), this, SLOT(onHeadsetConnected()));
    }
    wirelessResumeTimer->start(4000);
}

void MissionControl::onHeadsetConnected()
{
    qint64 headsetResumeTime = QSettings().value("main/headsetResumeSeconds", -1).toInt();

    if (headsetPauseStamp != -1
    &&  mafwState == Paused
    && !pausedByCall
    && (headsetResumeTime == -1 || headsetPauseStamp + headsetResumeTime*1000 > QDateTime::currentMSecsSinceEpoch()))
        mafwRenderer->resume();

    headsetPauseStamp = -1;
}

void MissionControl::onHeadsetDisconnected()
{
    if (QSettings().value("main/pauseHeadset", true).toBool()) {
        if (mafwState == Playing) {
            mafwRenderer->pause();
            headsetPauseStamp = QDateTime::currentMSecsSinceEpoch();
        } else if (pausedByCall) {
            headsetPauseStamp = QDateTime::currentMSecsSinceEpoch();
        }
    }
    if (wirelessResumeTimer) {
        disconnect(wirelessResumeTimer, SIGNAL(timeout()), this, SLOT(onHeadsetConnected()));
        wirelessResumeTimer->stop();
        wirelessResumeTimer->deleteLater();
        wirelessResumeTimer = NULL;
    }
}

void MissionControl::updateWiredHeadset()
{
    QDBusInterface jackInterface("org.freedesktop.Hal", HAL_PATH_RX51_JACK, "org.freedesktop.Hal.Device", QDBusConnection::systemBus(), this);
    QDBusInterface buttInterface("org.freedesktop.Hal", "/org/freedesktop/Hal/devices/platform_headphone", "org.freedesktop.Hal.Device", QDBusConnection::systemBus(), this);

    if (!jackInterface.isValid() || !buttInterface.isValid()) return;

    // State contains jack GPIO state -- false when nothing is connected to jack
    bool state = QDBusReply<bool>(buttInterface.call("GetProperty", "button.state.value"));

    // The list contains "headphone" when headset is connected
    bool connected = state && QDBusReply<QStringList>(jackInterface.call("GetProperty", "input.jack.type")).value().contains("headphone");

    if (connected && !wiredHeadsetIsConnected) {
        onHeadsetConnected();
        wiredHeadsetIsConnected = true;
    } else if (!connected && wiredHeadsetIsConnected) {
        onHeadsetDisconnected();
        wiredHeadsetIsConnected = false;
    }
}

void MissionControl::onHeadsetButtonPressed(QDBusMessage msg)
{
    if (msg.arguments()[0] == "ButtonPressed") {
        if (msg.arguments()[1] == "play-cd" || msg.arguments()[1] == "pause-cd")
            togglePlayback();
        else if (msg.arguments()[1] == "stop-cd")
            mafwRenderer->stop();
        else if (msg.arguments()[1] == "next-song")
            mafwRenderer->next();
        else if (msg.arguments()[1] == "previous-song")
            mafwRenderer->previous();
        else if (msg.arguments()[1] == "fast-forward")
            mafwRenderer->setPosition(SeekRelative, 3);
        else if (msg.arguments()[1] == "rewind")
            mafwRenderer->setPosition(SeekRelative, -3);
        else if (msg.arguments()[1] == "phone")
            handlePhoneButton();
        else if (msg.arguments()[1] == "jack_insert" && msg.path() == HAL_PATH_RX51_JACK) // wired headset was connected or disconnected
            updateWiredHeadset();
    }
}
