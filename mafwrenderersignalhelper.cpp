#include "mafwrenderersignalhelper.h"

void MafwRendererSignalHelper::play_playback_cb(MafwRenderer*,
                                                gpointer user_data,
                                                const GError* error)
{
    QString qerror;
    if(error)
    {
        qerror = QString(error->message);
    }
    emit static_cast<MafwRendererAdapter*>(user_data)->signalPlay(qerror);
}

void MafwRendererSignalHelper::play_uri_playback_cb(MafwRenderer*,
                                                    gpointer user_data,
                                                    const GError* error)
{
    QString qerror;
    if(error)
    {
        qerror = QString(error->message);
    }
    emit static_cast<MafwRendererAdapter*>(user_data)->signalPlayURI(qerror);
}

void MafwRendererSignalHelper::play_object_playback_cb(MafwRenderer*,
                                                       gpointer user_data,
                                                       const GError* error)
{
    QString qerror;
    if(error)
    {
        qerror = QString(error->message);
    }
    emit static_cast<MafwRendererAdapter*>(user_data)->signalPlayObject(qerror);
}

void MafwRendererSignalHelper::stop_playback_cb(MafwRenderer*,
                                                gpointer user_data,
                                                const GError* error)
{
    QString qerror;
    if(error)
    {
        qerror = QString(error->message);
    }
    emit static_cast<MafwRendererAdapter*>(user_data)->signalStop(qerror);
}

void MafwRendererSignalHelper::pause_playback_cb(MafwRenderer*,
                                                 gpointer user_data,
                                                 const GError* error)
{
    QString qerror;
    if(error)
    {
        qerror = QString(error->message);
    }
    emit static_cast<MafwRendererAdapter*>(user_data)->signalPause(qerror);
}

void MafwRendererSignalHelper::resume_playback_cb(MafwRenderer*,
                                                  gpointer user_data,
                                                  const GError* error)
{
    QString qerror;
    if(error)
    {
        qerror = QString(error->message);
    }
    emit static_cast<MafwRendererAdapter*>(user_data)->signalResume(qerror);
}

void MafwRendererSignalHelper::get_status_cb(MafwRenderer*,
                                             MafwPlaylist* playlist,
                                             uint index,
                                             MafwPlayState state,
                                             const char* object_id,
                                             gpointer user_data,
                                             const GError* error)
{
    QString qerror;
    if(error)
    {
        qerror = QString(error->message);
    }
    emit static_cast<MafwRendererAdapter*>(user_data)->signalGetStatus(playlist, index, state, object_id, qerror);
}

void MafwRendererSignalHelper::next_playback_cb(MafwRenderer*,
                                                gpointer user_data,
                                                const GError* error)
{
    QString qerror;
    if(error)
    {
        qerror = QString(error->message);
    }
    emit static_cast<MafwRendererAdapter*>(user_data)->signalNext(qerror);
}

void MafwRendererSignalHelper::previous_playback_cb(MafwRenderer*,
                                                    gpointer user_data,
                                                    const GError* error)
{
    QString qerror;
    if(error)
    {
        qerror = QString(error->message);
    }
    emit static_cast<MafwRendererAdapter*>(user_data)->signalPrevious(qerror);
}

void MafwRendererSignalHelper::goto_index_playback_cb(MafwRenderer*,
                                                      gpointer user_data,
                                                      const GError* error)
{
    QString qerror;
    if(error)
    {
        qerror = QString(error->message);
    }
    emit static_cast<MafwRendererAdapter*>(user_data)->signalGotoIndex(qerror);
}

void MafwRendererSignalHelper::set_position_cb(MafwRenderer*,
                                               int position,
                                               gpointer user_data,
                                               const GError* error)
{
    QString qerror;
    if(error)
    {
        qerror = QString(error->message);
    }
    emit static_cast<MafwRendererAdapter*>(user_data)->signalSetPosition(position, qerror);
}

void MafwRendererSignalHelper::get_position_cb(MafwRenderer*,
                                               int position,
                                               gpointer user_data,
                                               const GError* error)
{
    QString qerror;
    if(error)
    {
        qerror = QString(error->message);
    }
    emit static_cast<MafwRendererAdapter*>(user_data)->signalGetPosition(position, qerror);
}

void MafwRendererSignalHelper::get_current_metadata_cb(MafwRenderer*,
                                                       const gchar*,
                                                       GHashTable* table,
                                                       gpointer user_data,
                                                       const GError* error)
{
    GValue *v;
    QString qerror;
    if(error)
    {
        qerror = QString(error->message);
    }
    v = mafw_metadata_first(table, MAFW_METADATA_KEY_ARTIST);
    QString artist = v ? QString::fromUtf8(g_value_get_string(v)) : "(unknown artist)";

    v = mafw_metadata_first(table, MAFW_METADATA_KEY_TITLE);
    QString songName = v ? QString::fromUtf8(g_value_get_string(v)) : "(unknown song)";

    v = mafw_metadata_first(table, MAFW_METADATA_KEY_ALBUM);
    QString album = v ? QString::fromUtf8(g_value_get_string(v)) : "(unknown album)";

    // Seems to always return 0 for some reason...
    int duration = g_value_get_int(mafw_metadata_first(table, MAFW_METADATA_KEY_DURATION));
    emit static_cast<MafwRendererAdapter*>(user_data)->signalGetCurrentMetadata(songName, album, artist, duration, qerror);
}
