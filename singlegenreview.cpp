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

#include "singlegenreview.h"

SingleGenreView::SingleGenreView(QWidget *parent, MafwRendererAdapter* mra, MafwSourceAdapter* msa, MafwPlaylistAdapter* pls) :
    QMainWindow(parent),
    ui(new Ui::SingleGenreView)
#ifdef MAFW
    ,mafwrenderer(mra),
    mafwTrackerSource(msa),
    playlist(pls)
#endif
{
    ui->setupUi(this);
    QString shuffleText(tr("Shuffle songs"));
    ui->centralwidget->setLayout(ui->verticalLayout);

#ifdef MAFW
    ui->indicator->setSources(this->mafwrenderer, this->mafwTrackerSource, this->playlist);
#endif

#ifdef Q_WS_MAEMO_5
    setAttribute(Qt::WA_Maemo5StackedWindow);
    shuffleAllButton = new QMaemo5ValueButton(shuffleText, this);
    shuffleAllButton->setValueLayout(QMaemo5ValueButton::ValueUnderTextCentered);
    shuffleAllButton->setValueText("  songs");
    ui->searchHideButton->setIcon(QIcon::fromTheme("general_close"));
#else
    shuffleAllButton = new QPushButton(shuffleText, this);
#endif
    ArtistListItemDelegate *delegate = new ArtistListItemDelegate(ui->artistList);
    ui->artistList->setItemDelegate(delegate);

    ui->artistList->setContextMenuPolicy(Qt::CustomContextMenu);

    this->isShuffling = false;

    shuffleAllButton->setIcon(QIcon(shuffleButtonIcon));
    ui->verticalLayout->removeWidget(ui->artistList);
    ui->verticalLayout->removeWidget(ui->searchWidget);
    ui->verticalLayout->addWidget(shuffleAllButton);
    ui->verticalLayout->addWidget(ui->artistList);
    ui->verticalLayout->addWidget(ui->searchWidget);
    ui->searchWidget->hide();
    connect(ui->artistList, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(onItemSelected(QListWidgetItem*)));
    connect(ui->artistList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(onContextMenuRequested(QPoint)));
    connect(shuffleAllButton, SIGNAL(clicked()), this, SLOT(onShuffleButtonClicked()));
    connect(QApplication::desktop(), SIGNAL(resized(int)), this, SLOT(orientationChanged()));
    connect(ui->searchEdit, SIGNAL(textChanged(QString)), this, SLOT(onSearchTextChanged(QString)));
    connect(ui->searchHideButton, SIGNAL(clicked()), ui->searchWidget, SLOT(hide()));
    connect(ui->searchHideButton, SIGNAL(clicked()), ui->searchEdit, SLOT(clear()));
    connect(ui->actionAdd_to_now_playing, SIGNAL(triggered()), this, SLOT(addAllToNowPlaying()));
#ifdef MAFW
    connect(mafwTrackerSource, SIGNAL(signalSourceBrowseResult(uint, int, uint, QString, GHashTable*, QString)),
            this, SLOT(browseAllGenres(uint, int, uint, QString, GHashTable*, QString)));
#endif
    this->orientationChanged();
}

SingleGenreView::~SingleGenreView()
{
    delete ui;
}

void SingleGenreView::orientationChanged()
{
    ui->artistList->scroll(1,1);

    QRect screenGeometry = QApplication::desktop()->screenGeometry();
    ui->indicator->setGeometry(screenGeometry.width()-122, screenGeometry.height()-(70+55), 112, 70);
    ui->indicator->raise();
}

void SingleGenreView::onItemSelected(QListWidgetItem *item)
{
    int songCount = item->data(UserRoleAlbumCount).toInt();
    if(songCount == 0 || songCount == 1) {
        SingleAlbumView *albumView = new SingleAlbumView(this, this->mafwrenderer, this->mafwTrackerSource, this->playlist);
        if (songCount == 1)
            albumView->isSingleAlbum = true;
        albumView->browseAlbumByObjectId(item->data(UserRoleObjectID).toString());
        albumView->setAttribute(Qt::WA_DeleteOnClose);
        albumView->setWindowTitle(item->data(UserRoleSongName).toString());
        connect(albumView, SIGNAL(destroyed()), ui->indicator, SLOT(show()));
        albumView->show();
        ui->indicator->hide();
    } else if(songCount > 1) {
        SingleArtistView *artistView = new SingleArtistView(this, this->mafwrenderer, this->mafwTrackerSource, this->playlist);
        artistView->browseAlbum(item->data(UserRoleObjectID).toString());
        artistView->setWindowTitle(item->data(UserRoleSongName).toString());
        artistView->setSongCount(item->data(UserRoleSongCount).toInt());
        artistView->setAttribute(Qt::WA_DeleteOnClose);
        connect(artistView, SIGNAL(destroyed()), ui->indicator, SLOT(show()));
        artistView->show();
        ui->indicator->hide();
    }

    ui->artistList->clearSelection();
}

#ifdef MAFW
void SingleGenreView::browseGenre(QString objectId)
{
    objectIdToBrowse = objectId;
    currentObjectId = objectId;
    if (mafwTrackerSource->isReady())
        this->listGenres();
    else
        connect(mafwTrackerSource, SIGNAL(sourceReady()), this, SLOT(listGenres()));
}

void SingleGenreView::listGenres()
{
#ifdef Q_WS_MAEMO_5
    setAttribute(Qt::WA_Maemo5ShowProgressIndicator);
#endif

    this->browseGenreId = mafwTrackerSource->sourceBrowse(this->objectIdToBrowse.toUtf8(), false, NULL, NULL,
                                                          MAFW_SOURCE_LIST(MAFW_METADATA_KEY_TITLE,
                                                                           MAFW_METADATA_KEY_ALBUM_ART_SMALL_URI,
                                                                           MAFW_METADATA_KEY_CHILDCOUNT_1,
                                                                           MAFW_METADATA_KEY_CHILDCOUNT_2),
                                                          0, MAFW_SOURCE_BROWSE_ALL);
}

void SingleGenreView::browseAllGenres(uint browseId, int remainingCount, uint, QString objectId, GHashTable* metadata, QString error)
{
    if(browseId != this->browseGenreId)
      return;

    QString title;
    int songCount = -1;
    int albumCount = -1;
    QListWidgetItem *item = new QListWidgetItem();
    if(metadata != NULL) {
        GValue *v;
        v = mafw_metadata_first(metadata,
                                MAFW_METADATA_KEY_TITLE);
        title = v ? QString::fromUtf8(g_value_get_string(v)) : "(unknown artist)";
        v = mafw_metadata_first(metadata,
                                MAFW_METADATA_KEY_CHILDCOUNT_1);
        albumCount = v ? g_value_get_int (v) : -1;

        v = mafw_metadata_first(metadata,
                                MAFW_METADATA_KEY_CHILDCOUNT_2);
        songCount = v ? g_value_get_int (v) : -1;

        v = mafw_metadata_first(metadata, MAFW_METADATA_KEY_ALBUM_ART_SMALL_URI);
        if(v != NULL) {
            const gchar* file_uri = g_value_get_string(v);
            gchar* filename = NULL;
            if(file_uri != NULL && (filename = g_filename_from_uri(file_uri, NULL, NULL)) != NULL) {
                item->setData(UserRoleAlbumArt, filename);
            }
        }
    }


    if (title.isEmpty())
        title = tr("(unknown artist)");

    item->setText(title);
    item->setData(UserRoleSongName, title);
    item->setData(UserRoleSongCount, songCount);
    item->setData(UserRoleAlbumCount, albumCount);
    item->setData(UserRoleObjectID, objectId);
    ui->artistList->addItem(item);
    if(!error.isEmpty())
        qDebug() << error;
#ifdef Q_WS_MAEMO_5
        if(remainingCount != 0)
            this->setAttribute(Qt::WA_Maemo5ShowProgressIndicator, true);
        else
            this->setAttribute(Qt::WA_Maemo5ShowProgressIndicator, false);
#endif

#ifdef Q_WS_MAEMO_5
    if (remainingCount == 0)
        setAttribute(Qt::WA_Maemo5ShowProgressIndicator, false);
    else
        setAttribute(Qt::WA_Maemo5ShowProgressIndicator, true);
#endif
}
#endif

void SingleGenreView::setSongCount(int count)
{
    this->songsInGenre = count;
#ifdef Q_WS_MAEMO_5
    QString songCount;
    songCount = QString::number(count);
    songCount.append(" ");
    if(count != 1)
        songCount.append(tr("songs"));
    else
        songCount.append(tr("song"));
    shuffleAllButton->setValueText(songCount);
#endif
}

void SingleGenreView::keyReleaseEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Left || e->key() == Qt::Key_Right || e->key() == Qt::Key_Backspace)
        return;
    else if ((e->key() == Qt::Key_Up || e->key() == Qt::Key_Down) && !ui->searchWidget->isHidden())
        ui->artistList->setFocus();
    else {
        ui->artistList->clearSelection();
        if (ui->searchWidget->isHidden())
            ui->searchWidget->show();
        if (!ui->searchEdit->hasFocus())
            ui->searchEdit->setText(ui->searchEdit->text() + e->text());
        ui->searchEdit->setFocus();
    }
}

void SingleGenreView::onSearchTextChanged(QString text)
{
    if (!ui->indicator->isHidden())
        ui->indicator->hide();

    for (int i=0; i < ui->artistList->count(); i++) {
        if (ui->artistList->item(i)->text().toLower().indexOf(text.toLower()) == -1)
            ui->artistList->item(i)->setHidden(true);
        else
            ui->artistList->item(i)->setHidden(false);
    }

    if (text.isEmpty()) {
        ui->searchWidget->hide();
        if (ui->indicator->isHidden())
            ui->indicator->show();
    }
}

void SingleGenreView::onContextMenuRequested(QPoint point)
{
    QMenu *contextMenu = new QMenu(this);
    contextMenu->setAttribute(Qt::WA_DeleteOnClose);
    contextMenu->addAction(tr("Add to now playing"), this, SLOT(addItemToNowPlaying()));
    contextMenu->addAction(tr("Delete"));
    contextMenu->exec(point);
}

void SingleGenreView::addItemToNowPlaying()
{
#ifdef Q_WS_MAEMO_5
    this->setAttribute(Qt::WA_Maemo5ShowProgressIndicator, true);
#endif

#ifdef MAFW
    if (playlist->playlistName() == "FmpVideoPlaylist" || playlist->playlistName() == "FmpRadioPlaylist")
        playlist->assignAudioPlaylist();

    this->numberOfSongsToAdd = ui->artistList->currentItem()->data(UserRoleSongCount).toInt();
    this->objectIdToBrowse = ui->artistList->currentItem()->data(UserRoleObjectID).toString();

    ui->artistList->clearSelection();

    this->isShuffling = false;

    connect(mafwTrackerSource, SIGNAL(signalSourceBrowseResult(uint,int,uint,QString,GHashTable*,QString)),
            this, SLOT(onNowPlayingBrowseResult(uint,int,uint,QString,GHashTable*,QString)));
    addToNowPlayingId = mafwTrackerSource->sourceBrowse(this->objectIdToBrowse.toUtf8(), true, NULL, NULL, 0, 0, MAFW_SOURCE_BROWSE_ALL);
#endif
}

#ifdef MAFW
void SingleGenreView::onNowPlayingBrowseResult(uint browseId, int remainingCount, uint, QString objectId, GHashTable*,QString)
{
    if (this->addToNowPlayingId != browseId)
        return;

    playlist->appendItem(objectId);

#ifdef Q_WS_MAEMO_5
    if (remainingCount != 0)
        this->setAttribute(Qt::WA_Maemo5ShowProgressIndicator, true);
    else
       this->setAttribute(Qt::WA_Maemo5ShowProgressIndicator, false);
#endif

    if (remainingCount == 0) {
#ifdef Q_WS_MAEMO_5
        if (!isShuffling)
            this->notifyOnAddedToNowPlaying(numberOfSongsToAdd);
#endif

        disconnect(mafwTrackerSource, SIGNAL(signalSourceBrowseResult(uint,int,uint,QString,GHashTable*,QString)),
                   this, SLOT(onNowPlayingBrowseResult(uint,int,uint,QString,GHashTable*,QString)));

        if (this->isShuffling) {
            mafwrenderer->play();
            mafwrenderer->resume();

            NowPlayingWindow *window = new NowPlayingWindow(this, mafwrenderer, mafwTrackerSource, playlist);
            window->setAttribute(Qt::WA_DeleteOnClose);
            connect(window, SIGNAL(destroyed()), ui->indicator, SLOT(show()));
            window->show();
            ui->indicator->hide();
            this->isShuffling = false;
        }
    }
}
#endif

void SingleGenreView::addAllToNowPlaying()
{
#ifdef Q_WS_MAEMO_5
    this->setAttribute(Qt::WA_Maemo5ShowProgressIndicator, true);
#endif

#ifdef MAFW
    if (playlist->playlistName() == "FmpVideoPlaylist" || playlist->playlistName() == "FmpRadioPlaylist")
        playlist->assignAudioPlaylist();

    this->numberOfSongsToAdd = this->songsInGenre;
    this->objectIdToBrowse = this->currentObjectId;

    ui->artistList->clearSelection();

    connect(mafwTrackerSource, SIGNAL(signalSourceBrowseResult(uint,int,uint,QString,GHashTable*,QString)),
            this, SLOT(onNowPlayingBrowseResult(uint,int,uint,QString,GHashTable*,QString)));
    addToNowPlayingId = mafwTrackerSource->sourceBrowse(this->objectIdToBrowse.toUtf8(), true, NULL, NULL, 0, 0, MAFW_SOURCE_BROWSE_ALL);
#endif
}

void SingleGenreView::onShuffleButtonClicked()
{
#ifdef MAFW
    if (playlist->playlistName() == "FmpVideoPlaylist" || playlist->playlistName() == "FmpRadioPlaylist")
        playlist->assignAudioPlaylist();

    playlist->clear();
    playlist->setShuffled(true);
    this->isShuffling = true;

    this->addAllToNowPlaying();
#endif
}

#ifdef Q_WS_MAEMO_5
void SingleGenreView::notifyOnAddedToNowPlaying(int songCount)
{
        QString addedToNp;
        if (songCount == 1)
            addedToNp = tr("clip added to now playing");
        else
            addedToNp = tr("clips added to now playing");
        QMaemo5InformationBox::information(this, QString::number(songCount) + " " + addedToNp);
}
#endif
