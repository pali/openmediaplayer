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

#include "musicwindow.h"

MusicWindow::MusicWindow(QWidget *parent, MafwAdapterFactory *factory) :
        QMainWindow(parent),
#ifdef MAFW
        ui(new Ui::MusicWindow),
        mafwFactory(factory),
        mafwrenderer(factory->getRenderer()),
        mafwTrackerSource(factory->getTrackerSource()),
        playlist(factory->getPlaylistAdapter())
#else
        ui(new Ui::MusicWindow)
#endif
{
    ui->setupUi(this);
#ifdef Q_WS_MAEMO_5
    setAttribute(Qt::WA_Maemo5StackedWindow);
    ui->searchHideButton->setIcon(QIcon::fromTheme("general_close"));
#endif
#ifdef MAFW
    mafwPlaylistManager = new MafwPlaylistManagerAdapter(this);
#endif
    ui->centralwidget->setLayout(ui->songsLayout);
    SongListItemDelegate *songDelegate = new SongListItemDelegate(ui->songList);
    ArtistListItemDelegate *artistDelegate = new ArtistListItemDelegate(ui->artistList);
    ThumbnailItemDelegate *albumDelegate = new ThumbnailItemDelegate(ui->albumList);

    ui->songList->setItemDelegate(songDelegate);
    ui->artistList->setItemDelegate(artistDelegate);
    ui->albumList->setItemDelegate(albumDelegate);
    ui->genresList->setItemDelegate(songDelegate);
    ui->playlistList->setItemDelegate(songDelegate);

    ui->songList->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->albumList->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->artistList->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->genresList->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->playlistList->setContextMenuPolicy(Qt::CustomContextMenu);

    ui->albumList->installEventFilter(this);

    this->loadViewState();
    QRect screenGeometry = QApplication::desktop()->screenGeometry();
    ui->indicator->setGeometry(screenGeometry.width()-122, screenGeometry.height()-(70+55), 112, 70);
    ui->indicator->raise();
#ifdef MAFW
    ui->indicator->setFactory(mafwFactory);
#endif
    this->connectSignals();
}

MusicWindow::~MusicWindow()
{
    delete ui;
}

void MusicWindow::onSongSelected(QListWidgetItem *item)
{
    this->setEnabled(false);

#ifdef MAFW
    if (playlist->playlistName() != "FmpAudioPlaylist")
        playlist->assignAudioPlaylist();
    playlist->clear();
    playlist->setShuffled(false);

    gchar** songAddBuffer = new gchar*[ui->songList->count()];

    int visibleCount = 0;
    for (int i = 0; i < ui->songList->count(); i++)
        if (!ui->songList->item(i)->isHidden())
            songAddBuffer[visibleCount++] = qstrdup(ui->songList->item(i)->data(UserRoleObjectID).toString().toUtf8());
    songAddBuffer[visibleCount] = NULL;

    playlist->appendItems((const gchar**)songAddBuffer);

    for (int i = 0; i < visibleCount; i++)
        delete[] songAddBuffer[i];
    delete[] songAddBuffer;

    int selectedIndex = ui->songList->row(item);
    int visibleIndex = 0;
    for (int i = 0; i < selectedIndex; i++)
        if (!ui->songList->item(i)->isHidden())
           ++visibleIndex;

    // Instant play() seems to work only for smaller libraries.
    // For bigger, something probably doesn't have enough time to ready up.
    // Possible workaround is to call getSize() first, so when it returns,
    // we know that play() can be successfully called. That's only a theory,
    // but it works for me. ;)
    playlist->getSize();
    mafwrenderer->gotoIndex(visibleIndex);
    mafwrenderer->play();

    NowPlayingWindow *window = NowPlayingWindow::acquire(this, mafwFactory);
#else
    NowPlayingWindow *window = NowPlayingWindow::acquire(this);
#endif

    window->show();

    connect(window, SIGNAL(hidden()), this, SLOT(onNowPlayingWindowHidden()));
    ui->indicator->inhibit();
}

void MusicWindow::onPlaylistSelected(QListWidgetItem *item)
{
    if (item->data(Qt::UserRole).toBool()) return;

    int row = ui->playlistList->row(item);

    if (row >= 1 && row <= 4) {
        this->setEnabled(false);
        SinglePlaylistView *playlistView = new SinglePlaylistView(this, mafwFactory);
        playlistView->setWindowTitle(item->text());
        int limit = QSettings().value("music/playlistSize").toInt();
        if (row == 1)
            playlistView->browseAutomaticPlaylist("", "-added", limit);
        else if (row == 2)
            playlistView->browseAutomaticPlaylist("(play-count>0)", "-last-played", limit);
        else if (row == 3)
            playlistView->browseAutomaticPlaylist("(play-count>0)", "-play-count,+title", limit);
        else if (row == 4)
            playlistView->browseAutomaticPlaylist("(play-count=)", "", MAFW_SOURCE_BROWSE_ALL);

        playlistView->show();
        connect(playlistView, SIGNAL(destroyed()), this, SLOT(listPlaylists()));
        connect(playlistView, SIGNAL(destroyed()), this, SLOT(onChildClosed()));

    } else if (row >= 6) {
        this->setEnabled(false);
        SinglePlaylistView *playlistView = new SinglePlaylistView(this, mafwFactory);
        playlistView->setWindowTitle(item->text());
        if (item->data(UserRoleObjectID).isNull()) // saved playlist case
            playlistView->browsePlaylist(MAFW_PLAYLIST(mafwPlaylistManager->createPlaylist(item->text())));
        else // imported playlist case
            playlistView->browseObjectId(item->data(UserRoleObjectID).toString());

        playlistView->show();
        connect(playlistView, SIGNAL(destroyed()), this, SLOT(listPlaylists()));
        connect(playlistView, SIGNAL(destroyed()), this, SLOT(onChildClosed()));
    }
}

void MusicWindow::connectSignals()
{
#ifdef Q_WS_MAEMO_5
    connect(ui->songList, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(onSongSelected(QListWidgetItem*)));
#else
    connect(ui->songList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(onSongSelected(QListWidgetItem*)));
#endif
#ifdef MAFW
    connect(ui->songList->verticalScrollBar(), SIGNAL(valueChanged(int)), ui->indicator, SLOT(poke()));
    connect(ui->albumList, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(onAlbumSelected(QListWidgetItem*)));
    connect(ui->albumList->verticalScrollBar(), SIGNAL(valueChanged(int)), ui->indicator, SLOT(poke()));
    connect(ui->artistList, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(onArtistSelected(QListWidgetItem*)));
    connect(ui->artistList->verticalScrollBar(), SIGNAL(valueChanged(int)), ui->indicator, SLOT(poke()));
    connect(ui->genresList, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(onGenreSelected(QListWidgetItem*)));
    connect(ui->genresList->verticalScrollBar(), SIGNAL(valueChanged(int)), ui->indicator, SLOT(poke()));
    connect(ui->playlistList, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(onPlaylistSelected(QListWidgetItem*)));
    connect(ui->playlistList->verticalScrollBar(), SIGNAL(valueChanged(int)), ui->indicator, SLOT(poke()));
    connect(mafwTrackerSource, SIGNAL(containerChanged(QString)), this, SLOT(onContainerChanged(QString)));
    connect(mafwTrackerSource, SIGNAL(signalSourceBrowseResult(uint,int,uint,QString,GHashTable*,QString)),
            this, SLOT(browseAutomaticPlaylists(uint,int,uint,QString,GHashTable*,QString)));
#endif
    connect(ui->songList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(onContextMenuRequested(QPoint)));
    connect(ui->albumList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(onContextMenuRequested(QPoint)));
    connect(ui->artistList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(onContextMenuRequested(QPoint)));
    connect(ui->genresList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(onContextMenuRequested(QPoint)));
    connect(ui->playlistList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(onContextMenuRequested(QPoint)));
    connect(QApplication::desktop(), SIGNAL(resized(int)), this, SLOT(orientationChanged()));
    connect(ui->searchEdit, SIGNAL(textChanged(QString)), this, SLOT(onSearchTextChanged(QString)));
    connect(ui->searchHideButton, SIGNAL(clicked()), this, SLOT(onSearchHideButtonClicked()));
}

void MusicWindow::onContextMenuRequested(QPoint point)
{
    if (this->currentList() == ui->playlistList && ui->playlistList->currentItem()->data(Qt::UserRole).toBool())
        return;

    QMenu *contextMenu = new QMenu(this);
    contextMenu->setAttribute(Qt::WA_DeleteOnClose);
    contextMenu->addAction(tr("Add to now playing"), this, SLOT(onAddToNowPlaying()));
    if (this->currentList() == ui->playlistList) {
        if (ui->playlistList->currentRow() > 4) {
            if (ui->playlistList->currentItem()->data(UserRoleObjectID).isNull())
                contextMenu->addAction(tr("Delete playlist"), this, SLOT(onDeletePlaylistClicked())); // saved playlist
            else
                contextMenu->addAction(tr("Delete playlist"), this, SLOT(onDeleteClicked())); // imported playlist
        } else
            contextMenu->exec(point);
    }
    else if (this->currentList() == ui->artistList || this->currentList() == ui->albumList || this->currentList() == ui->songList) {
        contextMenu->addAction(tr("Delete"), this, SLOT(onDeleteClicked()));
        if (this->currentList() == ui->songList) {
            contextMenu->addAction(tr("Set as ringing tone"), this, SLOT(setRingingTone()));
            contextMenu->addAction(tr("Share"), this, SLOT(onShareClicked()));
        }
    }
    contextMenu->exec(point);
}

void MusicWindow::onDeletePlaylistClicked()
{
#ifdef MAFW
    if (ui->playlistList->currentItem()->data(UserRoleObjectID).isNull()) {
        QMessageBox confirmDelete(QMessageBox::NoIcon,
                                  " ",
                                  tr("Delete selected item from device?"),
                                  QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                                  this);
        confirmDelete.button(QMessageBox::Yes)->setText(tr("Yes"));
        confirmDelete.button(QMessageBox::No)->setText(tr("No"));
        confirmDelete.exec();
        if (confirmDelete.result() == QMessageBox::Yes) {
            mafwPlaylistManager->deletePlaylist(ui->playlistList->currentItem()->text());
            delete ui->playlistList->currentItem();
        }
    }
#endif
    ui->playlistList->clearSelection();
}

void MusicWindow::setRingingTone()
{
#ifdef MAFW
    QMessageBox confirmRingtone(QMessageBox::NoIcon,
                              " ",
                              tr("Are you sure you want to set this song as ringing tone?")+ "\n\n"
                              + ui->songList->currentItem()->text() + "\n"
                              + ui->songList->currentItem()->data(UserRoleSongArtist).toString(),
                              QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                              this);
    confirmRingtone.button(QMessageBox::Yes)->setText(tr("Yes"));
    confirmRingtone.button(QMessageBox::No)->setText(tr("No"));
    confirmRingtone.exec();
    if (confirmRingtone.result() == QMessageBox::Yes) {
        mafwTrackerSource->getUri(ui->songList->currentItem()->data(UserRoleObjectID).toString().toUtf8());
        connect(mafwTrackerSource, SIGNAL(signalGotUri(QString,QString)), this, SLOT(onRingingToneUriReceived(QString,QString)));
    }
#endif
    ui->songList->clearSelection();
}

#ifdef MAFW
void MusicWindow::onRingingToneUriReceived(QString objectId, QString uri)
{
    disconnect(mafwTrackerSource, SIGNAL(signalGotUri(QString,QString)), this, SLOT(onRingingToneUriReceived(QString,QString)));

    if (objectId != ui->songList->currentItem()->data(UserRoleObjectID).toString())
        return;
#ifdef Q_WS_MAEMO_5
    QDBusInterface setRingtone("com.nokia.profiled",
                               "/com/nokia/profiled",
                               "com.nokia.profiled",
                               QDBusConnection::sessionBus(), this);
    setRingtone.call("set_value", "general", "ringing.alert.tone", uri);
    QMaemo5InformationBox::information(this, "Selected song set as ringing tone");
#endif
}
#endif

void MusicWindow::onShareClicked()
{
#ifdef MAFW
    mafwTrackerSource->getUri(ui->songList->currentItem()->data(UserRoleObjectID).toString().toUtf8());
    connect(mafwTrackerSource, SIGNAL(signalGotUri(QString,QString)), this, SLOT(onShareUriReceived(QString,QString)));
#endif
}

#ifdef MAFW
void MusicWindow::onShareUriReceived(QString objectId, QString uri)
{
    disconnect(mafwTrackerSource, SIGNAL(signalGotUri(QString,QString)), this, SLOT(onShareUriReceived(QString,QString)));

    if (objectId != ui->songList->currentItem()->data(UserRoleObjectID).toString())
        return;

    QStringList list;
    QString clip;
    clip = uri;

    list.append(clip);
    Share *share = new Share(this, list);
    share->setAttribute(Qt::WA_DeleteOnClose);
    share->show();
}
#endif

void MusicWindow::onDeleteClicked()
{
#ifdef MAFW
    QMessageBox confirmDelete(QMessageBox::NoIcon,
                              " ",
                              tr("Delete selected item from device?"),
                              QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                              this);
    confirmDelete.button(QMessageBox::Yes)->setText(tr("Yes"));
    confirmDelete.button(QMessageBox::No)->setText(tr("No"));
    confirmDelete.exec();
    if (confirmDelete.result() == QMessageBox::Yes) {
        mafwTrackerSource->destroyObject(currentList()->currentItem()->data(UserRoleObjectID).toString().toUtf8());
        delete currentList()->currentItem();
    }
#endif
    currentList()->clearSelection();
}


void MusicWindow::onSearchHideButtonClicked()
{
    if (ui->searchEdit->text().isEmpty()) {
        ui->searchWidget->hide();
        ui->indicator->restore();
    } else
        ui->searchEdit->clear();
}

void MusicWindow::onSearchTextChanged(QString text)
{

    if (this->currentList() == ui->songList) // All songs
        for (int i = 0; i < ui->songList->count(); i++)
             if (ui->songList->item(i)->text().contains(text, Qt::CaseInsensitive)
             || ui->songList->item(i)->data(UserRoleSongArtist).toString().contains(text, Qt::CaseInsensitive)
             || ui->songList->item(i)->data(UserRoleSongAlbum).toString().contains(text, Qt::CaseInsensitive))
                 ui->songList->item(i)->setHidden(false);
             else
                 ui->songList->item(i)->setHidden(true);

    else if (this->currentList() == ui->albumList) // All albums
        for (int i = 0; i < ui->albumList->count(); i++)
             if (ui->albumList->item(i)->data(UserRoleTitle).toString().contains(text, Qt::CaseInsensitive)
             || ui->albumList->item(i)->data(UserRoleValueText).toString().contains(text, Qt::CaseInsensitive))
                 ui->albumList->item(i)->setHidden(false);
             else
                 ui->albumList->item(i)->setHidden(true);

    else // Artists, Genres, Playlists
        for (int i = 0; i < this->currentList()->count(); i++)
            if (this->currentList()->item(i)->text().contains(text, Qt::CaseInsensitive))
                this->currentList()->item(i)->setHidden(false);
            else
                this->currentList()->item(i)->setHidden(true);

    if (text.isEmpty()) {
        ui->searchWidget->hide();
        ui->indicator->restore();
    }
}

void MusicWindow::orientationChanged()
{
    QRect screenGeometry = QApplication::desktop()->screenGeometry();
    ui->indicator->setGeometry(screenGeometry.width()-122, screenGeometry.height()-(70+55),
                               ui->indicator->width(),ui->indicator->height());
    ui->indicator->raise();
}

bool MusicWindow::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::Resize)
        ui->albumList->setFlow(ui->albumList->flow());
    return false;
}

void MusicWindow::populateMenuBar()
{
    ui->menubar->clear();
    if(ui->albumList->isHidden())
        ui->menubar->addAction(tr("All albums"), this, SLOT(showAlbumView()));
    if(ui->artistList->isHidden())
        ui->menubar->addAction(tr("Artists"), this, SLOT(showArtistView()));
    if(ui->songList->isHidden())
        ui->menubar->addAction(tr("All songs"), this, SLOT(showSongsView()));
    if(ui->genresList->isHidden())
        ui->menubar->addAction(tr("Genres"), this, SLOT(showGenresView()));
    if(ui->playlistList->isHidden())
        ui->menubar->addAction(tr("Playlists"), this, SLOT(showPlayListView()));
}

void MusicWindow::hideLayoutContents()
{
    if(!ui->songList->isHidden())
        ui->songList->hide();
    if(!ui->artistList->isHidden())
        ui->artistList->hide();
    if(!ui->genresList->isHidden())
        ui->genresList->hide();
    if(!ui->albumList->isHidden())
        ui->albumList->hide();
    if(!ui->playlistList->isHidden())
        ui->playlistList->hide();
}

void MusicWindow::showAlbumView()
{
    ui->searchEdit->clear();
    this->hideLayoutContents();
    ui->albumList->show();
    this->populateMenuBar();
    QMainWindow::setWindowTitle(tr("Albums"));
    this->saveViewState("albums");
#ifdef MAFW
    if (ui->albumList->count() == 0) {
        if(mafwTrackerSource->isReady())
            this->listAlbums();
        else
            connect(mafwTrackerSource, SIGNAL(sourceReady()), this, SLOT(listAlbums()));
    }
#endif
}

void MusicWindow::showArtistView()
{
    ui->searchEdit->clear();
    this->hideLayoutContents();
    ui->artistList->show();
    this->populateMenuBar();
    QMainWindow::setWindowTitle(tr("Artists"));
    this->saveViewState("artists");
#ifdef MAFW
    if (ui->artistList->count() == 0) {
        if(mafwTrackerSource->isReady())
            this->listArtists();
        else
            connect(mafwTrackerSource, SIGNAL(sourceReady()), this, SLOT(listArtists()));
    }
#endif
}

void MusicWindow::showGenresView()
{
    ui->searchEdit->clear();
    this->hideLayoutContents();
    ui->genresList->show();
    this->populateMenuBar();
    QMainWindow::setWindowTitle(tr("Genres"));
    this->saveViewState("genres");
#ifdef MAFW
    if (ui->genresList->count() == 0) {
        if (mafwTrackerSource->isReady())
            this->listGenres();
        else
            connect(mafwTrackerSource, SIGNAL(sourceReady()), this, SLOT(listGenres()));
    }
#endif
}

void MusicWindow::showSongsView()
{
    ui->searchEdit->clear();
    this->hideLayoutContents();
    ui->songList->show();
    this->populateMenuBar();
    QMainWindow::setWindowTitle(tr("Songs"));
    this->saveViewState("songs");
#ifdef MAFW
    if (ui->songList->count() == 0) {
        if(mafwTrackerSource->isReady())
            this->listSongs();
        else
            connect(mafwTrackerSource, SIGNAL(sourceReady()), this, SLOT(listSongs()));
    }
#endif
}

void MusicWindow::showPlayListView()
{
    ui->searchEdit->clear();
    this->hideLayoutContents();
    ui->playlistList->show();
    this->populateMenuBar();
    QMainWindow::setWindowTitle(tr("Playlists"));
    this->saveViewState("playlists");
#ifdef MAFW
    if (mafwTrackerSource->isReady())
        this->listPlaylists();
    else
        connect(mafwTrackerSource, SIGNAL(sourceReady()), this, SLOT(listPlaylists()));
#endif
}

QListWidget* MusicWindow::currentList()
{
    if (!ui->songList->isHidden())
        return ui->songList;
    else if (!ui->artistList->isHidden())
        return ui->artistList;
    else if (!ui->genresList->isHidden())
        return ui->genresList;
    else if (!ui->albumList->isHidden())
        return ui->albumList;
    else if (!ui->playlistList->isHidden())
        return ui->playlistList;
    else
        return 0;
}

void MusicWindow::saveViewState(QVariant view)
{
    QSettings().setValue("music/view", view);
}

void MusicWindow::loadViewState()
{
    if (QSettings().contains("music/view")) {
        QString state = QSettings().value("music/view").toString();
        if (state == "songs")
            this->showSongsView();
        else if (state == "albums")
            this->showAlbumView();
        else if (state == "artists")
            this->showArtistView();
        else if (state == "genres")
            this->showGenresView();
        else if (state == "playlists")
            this->showPlayListView();
    } else {
        this->showSongsView();
    }
}

void MusicWindow::onChildClosed()
{
    ui->indicator->restore();
    this->currentList()->clearSelection();
    this->setEnabled(true);
}

#ifdef MAFW
void MusicWindow::onAlbumSelected(QListWidgetItem *item)
{
    this->setEnabled(false);

    SingleAlbumView *albumView = new SingleAlbumView(this, mafwFactory);
    albumView->browseAlbumByObjectId(item->data(UserRoleObjectID).toString());
    albumView->setWindowTitle(item->data(UserRoleTitle).toString());

    albumView->show();

    connect(albumView, SIGNAL(destroyed()), this, SLOT(onChildClosed()));
    ui->indicator->inhibit();
}

void MusicWindow::onArtistSelected(QListWidgetItem *item)
{
    int songCount = item->data(UserRoleAlbumCount).toInt();

    if (songCount == 0 || songCount == 1) {
        this->setEnabled(false);

        SingleAlbumView *albumView = new SingleAlbumView(this, mafwFactory);
        albumView->browseAlbumByObjectId(item->data(UserRoleObjectID).toString());
        albumView->setWindowTitle(item->text());

        albumView->show();
        connect(albumView, SIGNAL(destroyed()), this, SLOT(onChildClosed()));
        ui->indicator->inhibit();

    } else if (songCount > 1) {
        this->setEnabled(false);

        SingleArtistView *artistView = new SingleArtistView(this, mafwFactory);
        artistView->browseAlbum(item->data(UserRoleObjectID).toString());
        artistView->setWindowTitle(item->text());
        artistView->setSongCount(item->data(UserRoleSongCount).toInt());

        artistView->show();
        connect(artistView, SIGNAL(destroyed()), this, SLOT(onChildClosed()));
        ui->indicator->inhibit();
    }
}

void MusicWindow::onGenreSelected(QListWidgetItem *item)
{
    this->setEnabled(false);

    SingleGenreView *genreView = new SingleGenreView(this, mafwFactory);
    genreView->setWindowTitle(item->text());

    genreView->show();

    connect(genreView, SIGNAL(destroyed()), this, SLOT(onChildClosed()));
    ui->indicator->inhibit();

    genreView->setSongCount(item->data(UserRoleSongCount).toInt());
    genreView->browseGenre(item->data(UserRoleObjectID).toString());
}

void MusicWindow::listSongs()
{
#ifdef DEBUG
    qDebug() << "MusicWindow: Source ready";
#endif
#ifdef Q_WS_MAEMO_5
    this->setAttribute(Qt::WA_Maemo5ShowProgressIndicator, true);
#endif

    ui->songList->clear();
    connect(mafwTrackerSource, SIGNAL(signalSourceBrowseResult(uint, int, uint, QString, GHashTable*, QString)),
            this, SLOT(browseAllSongs(uint, int, uint, QString, GHashTable*, QString)));

    this->browseAllSongsId = mafwTrackerSource->sourceBrowse("localtagfs::music/songs", false, NULL, NULL,
                                                             MAFW_SOURCE_LIST(MAFW_METADATA_KEY_TITLE,
                                                                              MAFW_METADATA_KEY_ALBUM,
                                                                              MAFW_METADATA_KEY_ARTIST,
                                                                              MAFW_METADATA_KEY_DURATION),
                                                             0, MAFW_SOURCE_BROWSE_ALL);
}

void MusicWindow::listArtists()
{
#ifdef DEBUG
    qDebug("Source ready");
#endif
#ifdef Q_WS_MAEMO_5
    this->setAttribute(Qt::WA_Maemo5ShowProgressIndicator, true);
#endif

    ui->artistList->clear();
    connect(mafwTrackerSource, SIGNAL(signalSourceBrowseResult(uint, int, uint, QString, GHashTable*, QString)),
            this, SLOT(browseAllArtists(uint, int, uint, QString, GHashTable*, QString)));

    this->browseAllArtistsId = mafwTrackerSource->sourceBrowse("localtagfs::music/artists", false, NULL, NULL,
                                                               MAFW_SOURCE_LIST(MAFW_METADATA_KEY_TITLE,
                                                                                MAFW_METADATA_KEY_DURATION,
                                                                                MAFW_METADATA_KEY_CHILDCOUNT_1,
                                                                                MAFW_METADATA_KEY_CHILDCOUNT_2,
                                                                                MAFW_METADATA_KEY_ALBUM_ART_SMALL_URI),
                                                               0, MAFW_SOURCE_BROWSE_ALL);
}

void MusicWindow::listAlbums()
{
#ifdef Q_WS_MAEMO_5
    this->setAttribute(Qt::WA_Maemo5ShowProgressIndicator, true);
#endif

    ui->albumList->clear();
    connect(mafwTrackerSource, SIGNAL(signalSourceBrowseResult(uint, int, uint, QString, GHashTable*, QString)),
            this, SLOT(browseAllAlbums(uint, int, uint, QString, GHashTable*, QString)));

    this->browseAllAlbumsId = mafwTrackerSource->sourceBrowse("localtagfs::music/albums", false, NULL, NULL,
                                                              MAFW_SOURCE_LIST(MAFW_METADATA_KEY_ALBUM,
                                                                               MAFW_METADATA_KEY_ARTIST,
                                                                               MAFW_METADATA_KEY_CHILDCOUNT_1,
                                                                               MAFW_METADATA_KEY_ALBUM_ART_MEDIUM_URI),
                                                              0, MAFW_SOURCE_BROWSE_ALL);
}

void MusicWindow::listGenres()
{
#ifdef Q_WS_MAEMO_5
    this->setAttribute(Qt::WA_Maemo5ShowProgressIndicator, true);
#endif

    ui->genresList->clear();
    connect(mafwTrackerSource, SIGNAL(signalSourceBrowseResult(uint, int, uint, QString, GHashTable*, QString)),
            this, SLOT(browseAllGenres(uint, int, uint, QString, GHashTable*, QString)));

    this->browseAllGenresId = mafwTrackerSource->sourceBrowse("localtagfs::music/genres", false, NULL, NULL,
                                                              MAFW_SOURCE_LIST(MAFW_METADATA_KEY_TITLE,
                                                                               MAFW_METADATA_KEY_CHILDCOUNT_1,
                                                                               MAFW_METADATA_KEY_CHILDCOUNT_2,
                                                                               MAFW_METADATA_KEY_CHILDCOUNT_3),
                                                              0, MAFW_SOURCE_BROWSE_ALL);
}

void MusicWindow::listPlaylists()
{
    ui->playlistList->clear();

    recentlyAddedCount = 0;
    recentlyPlayedCount = 0;
    mostPlayedCount = 0;
    neverPlayedCount = 0;

    this->listAutoPlaylists();

    GArray* playlists = mafwPlaylistManager->listPlaylists();
    QString playlistName;

    if (playlists->len != 0) {
        QListWidgetItem *item = new QListWidgetItem();
        item->setText(tr("Saved"));
        item->setData(Qt::UserRole, true);
        item->setData(UserRoleSongDuration, Duration::Blank);
        ui->playlistList->addItem(item);
    }

    for (uint i = 0; i < playlists->len; i++) {
        MafwPlaylistManagerItem* item = &g_array_index(playlists, MafwPlaylistManagerItem, i);

        playlistName = QString::fromUtf8(item->name);
        int playlistCount = playlist->getSizeOf(MAFW_PLAYLIST (mafwPlaylistManager->getPlaylist(item->id)));

        if (playlistName != "FmpAudioPlaylist" && playlistName != "FmpVideoPlaylist" && playlistName != "FmpRadioPlaylist"
            && playlistCount != 0) {
            QListWidgetItem *listItem = new QListWidgetItem();
            listItem->setText(playlistName);
            listItem->setData(UserRoleSongCount, playlistCount);
            listItem->setData(UserRoleSongDuration, Duration::Blank);
            listItem->setData(UserRoleValueText, tr("%n song(s)", "", playlistCount));

            ui->playlistList->addItem(listItem);
        }
    }
    mafw_playlist_manager_free_list_of_playlists(playlists);

    QListWidgetItem *item = new QListWidgetItem();
    item->setData(Qt::UserRole, true);
    item->setText(tr("Imported playlists"));
    item->setData(UserRoleSongDuration, Duration::Blank);
    ui->playlistList->addItem(item);

    this->listImportedPlaylists();
}

void MusicWindow::listAutoPlaylists()
{
    QListWidgetItem *listItem = new QListWidgetItem();
    listItem->setText(tr("Automatic playlists"));
    listItem->setData(Qt::UserRole, true);
    listItem->setData(UserRoleSongDuration, Duration::Blank);
    ui->playlistList->insertItem(0, listItem);

    int limit = QSettings().value("music/playlistSize").toInt();
    QStringList playlists;
    playlists << tr("Recently added") << tr("Recently played") << tr("Most played") << tr("Never played");
    foreach (QString string, playlists) {
        QListWidgetItem *listItem = new QListWidgetItem();
        listItem->setText(string);
        listItem->setData(UserRoleSongDuration, Duration::Blank);
        ui->playlistList->addItem(listItem);
    }

    this->browseNeverPlayedId = mafwTrackerSource->sourceBrowse("localtagfs::music/songs", false,
                                                                "(play-count=)",
                                                                NULL,
                                                                MAFW_SOURCE_LIST(MAFW_METADATA_KEY_CHILDCOUNT_1,
                                                                                 MAFW_METADATA_KEY_DURATION),
                                                                0, MAFW_SOURCE_BROWSE_ALL);

    this->browseMostPlayedId = mafwTrackerSource->sourceBrowse("localtagfs::music/songs", false,
                                                               "(play-count>0)",
                                                               "-play-count,+title",
                                                               MAFW_SOURCE_LIST(MAFW_METADATA_KEY_CHILDCOUNT_1,
                                                                                MAFW_METADATA_KEY_DURATION),
                                                               0, limit);

    this->browseRecentlyPlayedId = mafwTrackerSource->sourceBrowse("localtagfs::music/songs", false,
                                                                   "(play-count>0)",
                                                                   "-last-played",
                                                                   MAFW_SOURCE_LIST(MAFW_METADATA_KEY_CHILDCOUNT_1,
                                                                                    MAFW_METADATA_KEY_DURATION),
                                                                   0, limit);

    this->browseRecentlyAddedId = mafwTrackerSource->sourceBrowse("localtagfs::music/songs", false, NULL, "-added",
                                                                  MAFW_SOURCE_LIST(MAFW_METADATA_KEY_CHILDCOUNT_1,
                                                                                   MAFW_METADATA_KEY_DURATION),
                                                                  0, limit);
}

void MusicWindow::listImportedPlaylists()
{
    browseImportedPlaylistsId = mafwTrackerSource->sourceBrowse("localtagfs::music/playlists", false, NULL, NULL,
                                                                MAFW_SOURCE_LIST(MAFW_METADATA_KEY_TITLE,
                                                                                 MAFW_METADATA_KEY_CHILDCOUNT_1,
                                                                                 MAFW_METADATA_KEY_DURATION),
                                                                0, MAFW_SOURCE_BROWSE_ALL);
}

void MusicWindow::browseAutomaticPlaylists(uint browseId, int remainingCount, uint, QString objectId, GHashTable *metadata, QString) // not really, imported playlists too
{
    GValue *v;

    if (browseId == this->browseRecentlyAddedId) {
        recentlyAddedCount++;
        ui->playlistList->item(1)->setData(UserRoleValueText, tr("%n song(s)", "", recentlyAddedCount));

    } else if (browseId == this->browseRecentlyPlayedId) {
        recentlyPlayedCount++;
        ui->playlistList->item(2)->setData(UserRoleValueText, tr("%n song(s)", "", recentlyPlayedCount));

    } else if (browseId == this->browseMostPlayedId) {
        mostPlayedCount++;
        ui->playlistList->item(3)->setData(UserRoleValueText, tr("%n song(s)", "", mostPlayedCount));

    } else if (browseId == this->browseNeverPlayedId) {
        neverPlayedCount++;
        ui->playlistList->item(4)->setData(UserRoleValueText, tr("%n song(s)", "", neverPlayedCount));

    } else if (browseId == this->browseImportedPlaylistsId) {
        QListWidgetItem *listItem = new QListWidgetItem();

        v = mafw_metadata_first (metadata, MAFW_METADATA_KEY_TITLE);
        listItem->setText(QString::fromUtf8(g_value_get_string(v)));

        v = mafw_metadata_first (metadata, MAFW_METADATA_KEY_CHILDCOUNT_1);
        listItem->setData(UserRoleValueText, tr("%n song(s)", "", g_value_get_int(v)));

        listItem->setData(UserRoleSongDuration, Duration::Blank);
        listItem->setData(UserRoleObjectID, objectId);

        ui->playlistList->addItem(listItem);
    }
    ui->playlistList->scroll(0,0);

    if (remainingCount == 0)
        if (!ui->searchEdit->text().isEmpty())
            this->onSearchTextChanged(ui->searchEdit->text());
}

void MusicWindow::browseAllSongs(uint browseId, int remainingCount, uint, QString objectId, GHashTable* metadata, QString error)
{
    if (browseId != browseAllSongsId) return;

    if (metadata != NULL) {
        QString title;
        QString artist;
        QString album;
        int duration;
        GValue *v;

        QListWidgetItem *item = new QListWidgetItem();

        v = mafw_metadata_first(metadata, MAFW_METADATA_KEY_TITLE);
        title = v ? QString::fromUtf8(g_value_get_string (v)) : tr("(unknown song)");

        v = mafw_metadata_first(metadata, MAFW_METADATA_KEY_ARTIST);
        artist = v ? QString::fromUtf8(g_value_get_string(v)) : tr("(unknown artist)");

        v = mafw_metadata_first(metadata, MAFW_METADATA_KEY_ALBUM);
        album = v ? QString::fromUtf8(g_value_get_string(v)) : tr("(unknown album)");

        v = mafw_metadata_first(metadata, MAFW_METADATA_KEY_DURATION);
        duration = v ? g_value_get_int (v) : Duration::Unknown;

        item->setText(title);
        item->setData(UserRoleSongArtist, artist);
        item->setData(UserRoleSongAlbum, album);
        item->setData(UserRoleObjectID, objectId);
        item->setData(UserRoleSongDuration, duration);

        ui->songList->addItem(item);
    }

    if (remainingCount == 0) {
        disconnect(mafwTrackerSource, SIGNAL(signalSourceBrowseResult(uint, int, uint, QString, GHashTable*, QString)),
                   this, SLOT(browseAllSongs(uint, int, uint, QString, GHashTable*, QString)));
        if (!ui->searchEdit->text().isEmpty())
            this->onSearchTextChanged(ui->searchEdit->text());
#ifdef Q_WS_MAEMO_5
        this->setAttribute(Qt::WA_Maemo5ShowProgressIndicator, false);
#endif
    }
}

void MusicWindow::browseAllArtists(uint browseId, int remainingCount, uint, QString objectId, GHashTable* metadata, QString error)
{
    if (browseId != browseAllArtistsId) return;

    if (metadata != NULL) {
        QString title;
        int songCount;
        int albumCount;
        GValue *v;

        QListWidgetItem *item = new QListWidgetItem();

        v = mafw_metadata_first(metadata, MAFW_METADATA_KEY_TITLE);
        title = v ? QString::fromUtf8(g_value_get_string(v)) : tr("(unknown artist)");

        v = mafw_metadata_first(metadata, MAFW_METADATA_KEY_CHILDCOUNT_1);
        albumCount = v ? g_value_get_int (v) : -1;

        v = mafw_metadata_first(metadata, MAFW_METADATA_KEY_CHILDCOUNT_2);
        songCount = v ? g_value_get_int (v) : -1;

        v = mafw_metadata_first(metadata, MAFW_METADATA_KEY_ALBUM_ART_SMALL_URI);
        if (v != NULL) {
            const gchar* file_uri = g_value_get_string(v);
            gchar* filename = NULL;
            if (file_uri != NULL && (filename = g_filename_from_uri(file_uri, NULL, NULL)) != NULL) {
                item->setData(UserRoleAlbumArt, filename);
            }
        }

        if (title.isEmpty()) title = tr("(unknown artist)");

        item->setText(title);
        item->setData(UserRoleSongCount, songCount);
        item->setData(UserRoleAlbumCount, albumCount);
        item->setData(UserRoleObjectID, objectId);

        ui->artistList->addItem(item);
    }

    if (!error.isEmpty())
        qDebug() << error;

    if (remainingCount == 0) {
        disconnect(mafwTrackerSource, SIGNAL(signalSourceBrowseResult(uint, int, uint, QString, GHashTable*, QString)),
                   this, SLOT(browseAllArtists(uint, int, uint, QString, GHashTable*, QString)));
        if (!ui->searchEdit->text().isEmpty())
            this->onSearchTextChanged(ui->searchEdit->text());
#ifdef Q_WS_MAEMO_5
        this->setAttribute(Qt::WA_Maemo5ShowProgressIndicator, false);
#endif
    }
}


void MusicWindow::browseAllAlbums(uint browseId, int remainingCount, uint, QString objectId, GHashTable* metadata, QString error)
{
    if (browseId != browseAllAlbumsId) return;

    if (metadata != NULL) {
        QString albumTitle;
        QString artist;
        QString albumArt;
        int songCount;
        GValue *v;

        QListWidgetItem *item = new QListWidgetItem();

        v = mafw_metadata_first(metadata, MAFW_METADATA_KEY_ALBUM);
        albumTitle = v ? QString::fromUtf8(g_value_get_string(v)) : tr("(unknown album)");

        v = mafw_metadata_first(metadata, MAFW_METADATA_KEY_ARTIST);
        artist = v ? QString::fromUtf8(g_value_get_string(v)) : tr("(unknown artist)");

        v = mafw_metadata_first(metadata, MAFW_METADATA_KEY_CHILDCOUNT_1);
        songCount = v ? g_value_get_int(v) : Duration::Unknown;

        v = mafw_metadata_first(metadata, MAFW_METADATA_KEY_ALBUM_ART_MEDIUM_URI);
        if (v != NULL) {
            const gchar* file_uri = g_value_get_string(v);
            gchar* filename = NULL;
            if (file_uri != NULL && (filename = g_filename_from_uri(file_uri, NULL, NULL)) != NULL)
                item->setIcon(QIcon(QString::fromUtf8(filename)));
        } else {
            item->setIcon(QIcon::fromTheme(defaultAlbumIcon));
        }

        if (artist == "__VV__") artist = tr("Various artists");

        item->setData(UserRoleValueText, artist);
        item->setData(UserRoleTitle, albumTitle);
        item->setData(UserRoleObjectID, objectId);
        item->setData(UserRoleSongCount, songCount);

        ui->albumList->addItem(item);
    }

    if (!error.isEmpty())
        qDebug() << error;

    if (remainingCount == 0) {
        disconnect(mafwTrackerSource, SIGNAL(signalSourceBrowseResult(uint, int, uint, QString, GHashTable*, QString)),
                   this, SLOT(browseAllAlbums(uint, int, uint, QString, GHashTable*, QString)));
        if (!ui->searchEdit->text().isEmpty())
            this->onSearchTextChanged(ui->searchEdit->text());
#ifdef Q_WS_MAEMO_5
        this->setAttribute(Qt::WA_Maemo5ShowProgressIndicator, false);
#endif
    }
}

void MusicWindow::browseAllGenres(uint browseId, int remainingCount, uint, QString objectId, GHashTable *metadata, QString error)
{
    if (this->browseAllGenresId != browseId) return;

    QString title;
    int songCount;
    int albumCount;
    int artistCount;
    GValue *v;

    QListWidgetItem *item = new QListWidgetItem();

    v = mafw_metadata_first (metadata, MAFW_METADATA_KEY_TITLE);
    title = v ? QString::fromUtf8(g_value_get_string (v)) : tr("(unknown genre)");

    v = mafw_metadata_first (metadata, MAFW_METADATA_KEY_CHILDCOUNT_1);
    artistCount = v ? g_value_get_int (v) : -1;

    v = mafw_metadata_first (metadata, MAFW_METADATA_KEY_CHILDCOUNT_2);
    albumCount = v ? g_value_get_int (v) : -1;

    v = mafw_metadata_first (metadata, MAFW_METADATA_KEY_CHILDCOUNT_3);
    songCount = v ? g_value_get_int (v) : -1;

    if (title.isEmpty()) title = tr("(unknown genre)");

    item->setText(title);
    item->setData(UserRoleSongCount, songCount);
    item->setData(UserRoleArtistCount, artistCount);
    item->setData(UserRoleAlbumCount, albumCount);
    item->setData(UserRoleObjectID, objectId);
    item->setData(UserRoleSongDuration, Duration::Blank);

    QString valueText = tr("%n song(s)", "", songCount) + ", "
                      + tr("%n album(s)", "", albumCount) + ", "
                      + tr("%n artist(s)", "", artistCount);
    item->setData(UserRoleValueText, valueText);

    ui->genresList->addItem(item);

    if (!error.isEmpty())
        qDebug() << error;

    if (remainingCount == 0) {
        disconnect(mafwTrackerSource, SIGNAL(signalSourceBrowseResult(uint, int, uint, QString, GHashTable*, QString)),
                   this, SLOT(browseAllGenres(uint, int, uint, QString, GHashTable*, QString)));
        if (!ui->searchEdit->text().isEmpty())
            this->onSearchTextChanged(ui->searchEdit->text());
#ifdef Q_WS_MAEMO_5
        this->setAttribute(Qt::WA_Maemo5ShowProgressIndicator, false);
#endif
    }
}
#endif



void MusicWindow::focusInEvent(QFocusEvent *)
{
    ui->indicator->triggerAnimation();
}

void MusicWindow::focusOutEvent(QFocusEvent *)
{
    ui->indicator->stopAnimation();
}

void MusicWindow::keyPressEvent(QKeyEvent *)
{

}

void MusicWindow::keyReleaseEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Left || e->key() == Qt::Key_Right || e->key() == Qt::Key_Backspace)
        return;
    else if (e->key() == Qt::Key_Up || e->key() == Qt::Key_Down)
        this->currentList()->setFocus();
    else {
        this->currentList()->clearSelection();
        if (ui->searchWidget->isHidden()) {
            ui->indicator->inhibit();
            ui->searchWidget->show();
        }
        if (!ui->searchEdit->hasFocus())
            ui->searchEdit->setText(ui->searchEdit->text() + e->text());
        ui->searchEdit->setFocus();
    }
}

void MusicWindow::onAddToNowPlaying()
{
#ifdef MAFW
    if (playlist->playlistName() != "FmpAudioPlaylist")
        playlist->assignAudioPlaylist();

    if (this->currentList() == ui->songList) {
        playlist->appendItem(ui->songList->currentItem()->data(UserRoleObjectID).toString());
#ifdef Q_WS_MAEMO_5
        this->notifyOnAddedToNowPlaying(1);
#endif
    }
    else if (this->currentList() == ui->artistList || this->currentList() == ui->albumList || this->currentList() == ui->genresList) {
#ifdef Q_WS_MAEMO_5
        this->setAttribute(Qt::WA_Maemo5ShowProgressIndicator, true);
#endif

        QString objectIdToBrowse = this->currentList()->currentItem()->data(UserRoleObjectID).toString();
        songAddBufferSize = 0;

        qDebug() << "connecting MusicWindow to signalSourceBrowseResult";
        connect(mafwTrackerSource, SIGNAL(signalSourceBrowseResult(uint,int,uint,QString,GHashTable*,QString)),
                this, SLOT(onAddToNowPlayingCallback(uint,int,uint,QString,GHashTable*,QString)));

        this->addToNowPlayingId = mafwTrackerSource->sourceBrowse(objectIdToBrowse.toUtf8(), true, NULL, NULL, 0,
                                                                  0, MAFW_SOURCE_BROWSE_ALL);
    }
    else if (this->currentList() == ui->playlistList) {
#ifdef Q_WS_MAEMO_5
        this->setAttribute(Qt::WA_Maemo5ShowProgressIndicator, true);
#endif
        QListWidgetItem *item = this->currentList()->currentItem();

        qDebug() << "item id: " << item->data(UserRoleObjectID).toString();

        int row = this->currentList()->row(item);
        if (row < 5) { // automatic playlist case
            QString filter;
            QString sorting;
            songAddBufferSize = 0;
            int limit = QSettings().value("music/playlistSize").toInt();
            switch (row) {
                case 1: filter = ""; sorting = "-added"; break;
                case 2: filter = "(play-count>0)"; sorting = "-last-played"; break;
                case 3: filter = "(play-count>0)"; sorting = "-play-count,+title"; break;
                case 4: filter = "(play-count=)"; sorting = "";
            }

            qDebug() << "connecting MusicWindow to signalSourceBrowseResult";
            connect(mafwTrackerSource, SIGNAL(signalSourceBrowseResult(uint,int,uint,QString,GHashTable*,QString)),
                    this, SLOT(onAddToNowPlayingCallback(uint,int,uint,QString,GHashTable*,QString)));

            this->addToNowPlayingId = mafwTrackerSource->sourceBrowse("localtagfs::music/songs", true, filter.toUtf8(), sorting.toUtf8(),
                                                                      MAFW_SOURCE_NO_KEYS, 0, limit);
        }
        else if (item->data(UserRoleObjectID).isNull()) { // saved playlist case
            MafwPlaylist *mafwplaylist = MAFW_PLAYLIST(mafwPlaylistManager->createPlaylist(item->text()));
            songAddBufferSize = numberOfSongsToAdd = playlist->getSizeOf(mafwplaylist);

            if (numberOfSongsToAdd > 0) {
                songAddBuffer = new gchar*[songAddBufferSize+1];
                songAddBuffer[songAddBufferSize] = NULL;

                qDebug() << "connecting MusicWindow to onGetItems";
                connect(playlist, SIGNAL(onGetItems(QString,GHashTable*,guint,gpointer)),
                        this, SLOT(onGetItems(QString,GHashTable*,guint,gpointer)));

                this->addToNowPlayingId = (uint)playlist->getItemsOf(mafwplaylist);
            }
        }
        else { // imported playlist case
            QString objectId = item->data(UserRoleObjectID).toString();
            songAddBufferSize = 0;

            qDebug() << "connecting MusicWindow to signalSourceBrowseResult";
            connect(mafwTrackerSource, SIGNAL(signalSourceBrowseResult(uint,int,uint,QString,GHashTable*,QString)),
                    this, SLOT(onAddToNowPlayingCallback(uint,int,uint,QString,GHashTable*,QString)));

            // for some reason, if metadata fetching is disabled here, IDs for filesystem instead of localtagfs are returned
            this->addToNowPlayingId = mafwTrackerSource->sourceBrowse(objectId.toUtf8(), true, NULL, NULL,
                                                                      MAFW_SOURCE_LIST (MAFW_METADATA_KEY_TITLE,
                                                                               MAFW_METADATA_KEY_DURATION,
                                                                               MAFW_METADATA_KEY_ARTIST,
                                                                               MAFW_METADATA_KEY_ALBUM),
                                                                      0, MAFW_SOURCE_BROWSE_ALL);
        }
    }
#endif
}

void MusicWindow::onGetItems(QString objectId, GHashTable*, guint index, gpointer op)
{
    if ((uint)op != addToNowPlayingId) return;

    qDebug() << "MusicWindow::onGetItems | index: " << index;
    songAddBuffer[index] = qstrdup(objectId.toUtf8());
    numberOfSongsToAdd--;

    if (numberOfSongsToAdd == 0) {
        qDebug() << "disconnecting MusicWindow from onGetItems";
        disconnect(playlist, SIGNAL(onGetItems(QString,GHashTable*,guint,gpointer)),
                   this, SLOT(onGetItems(QString,GHashTable*,guint,gpointer)));

        playlist->appendItems((const gchar**)songAddBuffer);

        for (int i = 0; i < songAddBufferSize; i++)
            delete[] songAddBuffer[i];
        delete[] songAddBuffer;

#ifdef Q_WS_MAEMO_5
        setAttribute(Qt::WA_Maemo5ShowProgressIndicator, false);
        this->notifyOnAddedToNowPlaying(songAddBufferSize);
#endif
        songAddBufferSize = 0;
    }
}

void MusicWindow::onContainerChanged(QString objectId)
{
    if (objectId == "localtagfs::music") {
        this->listPlaylists();
        this->listArtists();
        this->listGenres();
        this->listAlbums();
        this->listSongs();
    }
}

void MusicWindow::onAddToNowPlayingCallback(uint browseId, int remainingCount, uint index, QString objectId, GHashTable*, QString)
{
    if (browseId != this->addToNowPlayingId) return;

    if (songAddBufferSize == 0) {
        songAddBufferSize = remainingCount+1;
        songAddBuffer = new gchar*[songAddBufferSize+1];
        songAddBuffer[songAddBufferSize] = NULL;
    }

    songAddBuffer[index] = qstrdup(objectId.toUtf8());

    if (remainingCount == 0) {
        playlist->appendItems((const gchar**)songAddBuffer);

        qDebug() << "disconnecting MusicWindow from signalSourceBrowseResult";
        disconnect(mafwTrackerSource, SIGNAL(signalSourceBrowseResult(uint,int,uint,QString,GHashTable*,QString)),
                   this, SLOT(onAddToNowPlayingCallback(uint,int,uint,QString,GHashTable*,QString)));

        for (int i = 0; i < songAddBufferSize; i++)
            delete[] songAddBuffer[i];
        delete[] songAddBuffer;
        this->addToNowPlayingId = 0;
#ifdef Q_WS_MAEMO_5
        setAttribute(Qt::WA_Maemo5ShowProgressIndicator, false);
        this->notifyOnAddedToNowPlaying(songAddBufferSize);
#endif
        songAddBufferSize = 0;
    }
}

#ifdef Q_WS_MAEMO_5
void MusicWindow::notifyOnAddedToNowPlaying(int songCount)
{
    QMaemo5InformationBox::information(this, tr("%n clip(s) added to now playing", "", songCount));
}
#endif

void MusicWindow::showEvent(QShowEvent *)
{
    emit shown();
}

void MusicWindow::hideEvent(QHideEvent *)
{
    QMainWindow *child = findChild<QMainWindow*>();
    if (child) child->close();

    emit hidden();
}

void MusicWindow::onNowPlayingWindowHidden()
{
    disconnect(NowPlayingWindow::acquire(), SIGNAL(hidden()), this, SLOT(onNowPlayingWindowHidden()));
    this->onChildClosed();
}
