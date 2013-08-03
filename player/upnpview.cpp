#include "upnpview.h"

UpnpView::UpnpView(QWidget *parent, MafwAdapterFactory *factory, MafwSourceAdapter *source) :
    BaseWindow(parent),
    ui(new Ui::UpnpView),
    mafwFactory(factory),
    mafwSource(source),
    playlist(factory->getPlaylistAdapter())
{
    ui->setupUi(this);
    ui->searchHideButton->setIcon(QIcon::fromTheme("general_close"));

    this->setAttribute(Qt::WA_DeleteOnClose);

    ui->objectList->setItemDelegate(new MediaWithIconDelegate(ui->objectList));

    objectModel = new QStandardItemModel(this);
    objectProxyModel = new QSortFilterProxyModel(this);
    objectProxyModel->setFilterRole(UserRoleTitle);
    objectProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    objectProxyModel->setSourceModel(objectModel);
    ui->objectList->setModel(objectProxyModel);

    connect(new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Enter), this), SIGNAL(activated()), this, SLOT(onContextMenuRequested()));

    connect(ui->objectList, SIGNAL(activated(QModelIndex)), this, SLOT(onItemActivated(QModelIndex)));
    connect(ui->objectList->verticalScrollBar(), SIGNAL(valueChanged(int)), ui->indicator, SLOT(poke()));
    connect(ui->objectList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(onContextMenuRequested(QPoint)));

    connect(ui->searchEdit, SIGNAL(textChanged(QString)), this, SLOT(onSearchTextChanged(QString)));
    connect(ui->searchEdit, SIGNAL(textChanged(QString)), objectProxyModel, SLOT(setFilterFixedString(QString)));
    connect(ui->searchHideButton, SIGNAL(clicked()), this, SLOT(onSearchHideButtonClicked()));

    connect(ui->actionAdd_to_now_playing, SIGNAL(triggered()), this, SLOT(addAllToNowPlaying()));

    ui->objectList->viewport()->installEventFilter(this);

    ui->indicator->setFactory(factory);

    Rotator *rotator = Rotator::acquire();
    connect(rotator, SIGNAL(rotated(int,int)), this, SLOT(onOrientationChanged(int,int)));
    onOrientationChanged(rotator->width(), rotator->height());
}

UpnpView::~UpnpView()
{
    delete ui;
}

void UpnpView::browseObjectId(QString objectId)
{
#ifdef Q_WS_MAEMO_5
    setAttribute(Qt::WA_Maemo5ShowProgressIndicator, true);
#endif

    objectModel->clear();

    connect(mafwSource, SIGNAL(signalSourceBrowseResult(uint,int,uint,QString,GHashTable*,QString)),
            this, SLOT(onBrowseResult(uint,int,uint,QString,GHashTable*,QString)), Qt::UniqueConnection);

    browseId = mafwSource->sourceBrowse(objectId.toUtf8(), true, NULL, NULL,
                                        MAFW_SOURCE_LIST(MAFW_METADATA_KEY_TITLE,
                                                         MAFW_METADATA_KEY_DURATION,
                                                         MAFW_METADATA_KEY_MIME),
                                        0, MAFW_SOURCE_BROWSE_ALL);
}

void UpnpView::keyPressEvent(QKeyEvent *e)
{
    switch (e->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Space:
        case Qt::Key_Control:
        case Qt::Key_Shift:
            break;

        case Qt::Key_Backspace:
            this->close();
            break;

        case Qt::Key_Up:
        case Qt::Key_Down:
            ui->objectList->setFocus();
            break;

        default:
            ui->objectList->clearSelection();
            if (ui->searchWidget->isHidden()) {
                ui->indicator->inhibit();
                ui->searchWidget->show();
            }
            if (!ui->searchEdit->hasFocus()) {
                ui->searchEdit->setText(ui->searchEdit->text() + e->text());
                ui->searchEdit->setFocus();
            }
            break;
    }
}

void UpnpView::keyReleaseEvent(QKeyEvent *e)
{
    switch (e->key()) {
        case Qt::Key_Up:
        case Qt::Key_Down:
            ui->objectList->setFocus();
    }
}

bool UpnpView::eventFilter(QObject *, QEvent *e)
{
    if (e->type() == QEvent::MouseButtonPress
    && static_cast<QMouseEvent*>(e)->y() > ui->objectList->viewport()->height() - 25
    && ui->searchWidget->isHidden()) {
        ui->indicator->inhibit();
        ui->searchWidget->show();
    }
    return false;
}

void UpnpView::onSearchHideButtonClicked()
{
    if (ui->searchEdit->text().isEmpty()) {
        ui->searchWidget->hide();
        ui->indicator->restore();
    } else
        ui->searchEdit->clear();
}

void UpnpView::onSearchTextChanged(QString text)
{
    if (text.isEmpty()) {
        ui->searchWidget->hide();
        ui->indicator->restore();
    }
}

void UpnpView::onBrowseResult(uint browseId, int remainingCount, uint, QString objectId, GHashTable* metadata, QString)
{
    if (browseId != this->browseId) return;

    if (metadata != NULL) {
        QString title;
        QString mime;
        int duration;
        GValue *v;

        v = mafw_metadata_first(metadata, MAFW_METADATA_KEY_TITLE);
        title = v ? QString::fromUtf8(g_value_get_string (v)) : tr("(unknown song)");

        v = mafw_metadata_first(metadata, MAFW_METADATA_KEY_MIME);
        mime = v ? QString::fromUtf8(g_value_get_string(v)) : tr("(unknown type)");

        if (mime.startsWith("audio") || mime.startsWith("video")) {
            v = mafw_metadata_first(metadata, MAFW_METADATA_KEY_DURATION);
            duration = v ? g_value_get_int(v) : Duration::Unknown;
        } else
            duration = Duration::Blank;

        QStandardItem *item = new QStandardItem();

        item->setData(objectId, UserRoleObjectID);
        item->setData(duration, UserRoleSongDuration);
        item->setData(mime, UserRoleMIME);
        item->setData(title, UserRoleTitle);

        if (mime == "x-mafw/container")
            item->setIcon(QIcon::fromTheme("general_folder"));
        else if (mime.startsWith("audio"))
            item->setIcon(QIcon::fromTheme("general_audio_file"));
        else if (mime.startsWith("video"))
            item->setIcon(QIcon::fromTheme("general_video_file"));
        else
            item->setIcon(QIcon::fromTheme("filemanager_unknown_file"));

        objectModel->appendRow(item);
    }

    if (remainingCount == 0) {
        disconnect(mafwSource, SIGNAL(signalSourceBrowseResult(uint,int,uint,QString,GHashTable*,QString)),
                   this, SLOT(onBrowseResult(uint,int,uint,QString,GHashTable*,QString)));
#ifdef Q_WS_MAEMO_5
        setAttribute(Qt::WA_Maemo5ShowProgressIndicator, false);
#endif
    }
}

void UpnpView::onContextMenuRequested(const QPoint &pos)
{
    if (ui->objectList->currentIndex().data(UserRoleMIME).toString().startsWith("audio")) {
        QMenu *contextMenu = new KbMenu(this);
        contextMenu->setAttribute(Qt::WA_DeleteOnClose);
        contextMenu->addAction(tr("Add to now playing"), this, SLOT(onAddOneToNowPlaying()));
        contextMenu->addAction(tr("Add to a playlist"), this, SLOT(onAddOneToPlaylist()));
        contextMenu->exec(this->mapToGlobal(pos));
    }
}

void UpnpView::onItemActivated(QModelIndex index)
{
    QString objectId = index.data(UserRoleObjectID).toString();
    QString mime = index.data(UserRoleMIME).toString();

    if (mime == "x-mafw/container") {
        this->setEnabled(false);
        UpnpView *window = new UpnpView(this, mafwFactory, mafwSource);
        window->browseObjectId(objectId);
        window->setWindowTitle(index.data(UserRoleTitle).toString());
        window->show();

        connect(window, SIGNAL(destroyed()), this, SLOT(onChildClosed()));
        ui->indicator->inhibit();

    } else if (mime.startsWith("audio")) {
        this->setEnabled(false);
        playlist->assignAudioPlaylist();
        playlist->clear();
        playlist->setShuffled(false);

        bool filter = QSettings().value("main/playlistFilter", false).toBool();

        appendAllToPlaylist("audio", filter);

        int sameTypeIndex = 0;
        if (filter) {
            int selectedRow = index.row();
            for (int i = 0; i < selectedRow; i++)
                if (objectProxyModel->index(i,0).data(UserRoleMIME).toString().startsWith("audio"))
                    ++sameTypeIndex;
        } else {
            int selectedRow = objectProxyModel->mapToSource(index).row();
            for (int i = 0; i < selectedRow; i++)
                if (objectModel->item(i)->data(UserRoleMIME).toString().startsWith("audio"))
                    ++sameTypeIndex;
        }

        MafwRendererAdapter *mafwrenderer = mafwFactory->getRenderer();
        mafwrenderer->gotoIndex(sameTypeIndex);
        mafwrenderer->play();

        NowPlayingWindow *window = NowPlayingWindow::acquire(this, mafwFactory);
        window->show();

        connect(window, SIGNAL(hidden()), this, SLOT(onNowPlayingWindowHidden()));
        ui->indicator->inhibit();

    } else if (mime.startsWith("video")) {
        this->setEnabled(false);
        playlist->assignVideoPlaylist();
        playlist->clear();

        bool filter = QSettings().value("main/playlistFilter", false).toBool();

        appendAllToPlaylist("video", filter);

        int sameTypeIndex = 0;
        if (filter) {
            int selectedRow = index.row();
            for (int i = 0; i < selectedRow; i++)
                if (objectProxyModel->index(i,0).data(UserRoleMIME).toString().startsWith("video"))
                    ++sameTypeIndex;
        } else {
            int selectedRow = objectProxyModel->mapToSource(index).row();
            for (int i = 0; i < selectedRow; i++)
                if (objectModel->item(i)->data(UserRoleMIME).toString().startsWith("video"))
                    ++sameTypeIndex;
        }

        VideoNowPlayingWindow *window = new VideoNowPlayingWindow(this, mafwFactory);
        window->showFullScreen();

        connect(window, SIGNAL(destroyed()), this, SLOT(onChildClosed()));
        ui->indicator->inhibit();

        MafwRendererAdapter *mafwrenderer = mafwFactory->getRenderer();
        mafwrenderer->gotoIndex(sameTypeIndex);
        window->play();

    } else {
        ui->objectList->clearSelection();
    }
}

void UpnpView::onAddOneToNowPlaying()
{
    if (playlist->playlistName() != "FmpAudioPlaylist")
        playlist->assignAudioPlaylist();

    playlist->appendItem(ui->objectList->currentIndex().data(UserRoleObjectID).toString());

    notifyOnAddedToNowPlaying(1);
}

void UpnpView::onAddOneToPlaylist()
{
    PlaylistPicker picker(this);
    picker.exec();
    if (picker.result() == QDialog::Accepted) {
        playlist->appendItem(picker.playlist, ui->objectList->currentIndex().data(UserRoleObjectID).toString());
#ifdef Q_WS_MAEMO_5
        QMaemo5InformationBox::information(this, tr("%n clip(s) added to playlist", "", 1));
#endif
    }
}

void UpnpView::addAllToNowPlaying()
{
    if (playlist->playlistName() != "FmpAudioPlaylist")
        playlist->assignAudioPlaylist();

    notifyOnAddedToNowPlaying(appendAllToPlaylist("audio", true));
}

int UpnpView::appendAllToPlaylist(QString type, bool filter)
{
    int visibleCount = filter ? objectProxyModel->rowCount() : objectModel->rowCount();
    gchar** itemAddBuffer = new gchar*[visibleCount+1];

    int sameTypeCount = 0;
    if (filter) {
        for (int i = 0; i < visibleCount; i++)
            if (objectProxyModel->index(i,0).data(UserRoleMIME).toString().startsWith(type))
                itemAddBuffer[sameTypeCount++] = qstrdup(objectProxyModel->index(i,0).data(UserRoleObjectID).toString().toUtf8());
    } else {
        for (int i = 0; i < visibleCount; i++)
            if (objectModel->item(i)->data(UserRoleMIME).toString().startsWith(type))
                itemAddBuffer[sameTypeCount++] = qstrdup(objectModel->item(i)->data(UserRoleObjectID).toString().toUtf8());
    }

    itemAddBuffer[sameTypeCount] = NULL;

    playlist->appendItems((const gchar**)itemAddBuffer);

    for (int i = 0; i < sameTypeCount; i++)
        delete[] itemAddBuffer[i];
    delete[] itemAddBuffer;

    return sameTypeCount;
}

void UpnpView::notifyOnAddedToNowPlaying(int songCount)
{
#ifdef Q_WS_MAEMO_5
    QMaemo5InformationBox::information(this, tr("%n clip(s) added to now playing", "", songCount));
#endif
}

void UpnpView::onOrientationChanged(int w, int h)
{
    ui->indicator->setGeometry(w-(112+8), h-(70+56), 112, 70);
    ui->indicator->raise();
}

void UpnpView::onNowPlayingWindowHidden()
{
    disconnect(NowPlayingWindow::acquire(), SIGNAL(hidden()), this, SLOT(onNowPlayingWindowHidden()));
    ui->indicator->restore();
    ui->objectList->clearSelection();
    this->setEnabled(true);
}

void UpnpView::onChildClosed()
{
    ui->indicator->restore();
    ui->objectList->clearSelection();
    this->setEnabled(true);
}