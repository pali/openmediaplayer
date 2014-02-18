#include "browserwindow.h"

BrowserWindow::BrowserWindow(QWidget *parent, MafwAdapterFactory *mafwFactory) :
    BaseWindow(parent),
    ui(new Ui::BrowserWindow)
{
    ui->setupUi(this);

    ui->searchHideButton->setIcon(QIcon::fromTheme("general_close"));

    ui->indicator->setFactory(mafwFactory);

    this->setAttribute(Qt::WA_DeleteOnClose);

    objectModel = new QStandardItemModel(this);
    objectProxyModel = new HeaderAwareProxyModel(this);
    objectProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    objectProxyModel->setSourceModel(objectModel);
    ui->objectList->setModel(objectProxyModel);

    ui->objectList->viewport()->installEventFilter(this);

    connect(ui->objectList->verticalScrollBar(), SIGNAL(valueChanged(int)), ui->indicator, SLOT(poke()));

    connect(ui->searchEdit, SIGNAL(textChanged(QString)), this, SLOT(onSearchTextChanged(QString)));
    connect(ui->searchHideButton, SIGNAL(clicked()), this, SLOT(onSearchHideButtonClicked()));

    connect(Rotator::acquire(), SIGNAL(rotated(int,int)), this, SLOT(onOrientationChanged(int,int)));

    // Set the initial orientation later, after child class constructor
    QTimer::singleShot(0, this, SLOT(orientationInit()));
}

BrowserWindow::~BrowserWindow()
{
    delete ui;
}

bool BrowserWindow::eventFilter(QObject *, QEvent *e)
{
    if (e->type() == QEvent::MouseButtonPress
    && static_cast<QMouseEvent*>(e)->y() > ui->objectList->viewport()->height() - 25
    && ui->searchWidget->isHidden()) {
        ui->indicator->inhibit();
        ui->searchWidget->show();
    }
    return false;
}

void BrowserWindow::keyPressEvent(QKeyEvent *e)
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

void BrowserWindow::keyReleaseEvent(QKeyEvent *e)
{
    switch (e->key()) {
        case Qt::Key_Up:
        case Qt::Key_Down:
            ui->objectList->setFocus();
    }
}

void BrowserWindow::orientationInit()
{
    Rotator *rotator = Rotator::acquire();

    this->onOrientationChanged(rotator->width(), rotator->height());
}

void BrowserWindow::onOrientationChanged(int w, int h)
{
    if (ui->objectList->viewMode() == QListView::IconMode)
        ui->objectList->setGridSize(QSize(155, w > h ? 212 : 186));

    ui->indicator->setGeometry(w-(112+8), h-(70+56), 112, 70);
    ui->indicator->raise();
}

void BrowserWindow::onSearchHideButtonClicked()
{
    if (ui->searchEdit->text().isEmpty()) {
        ui->searchWidget->hide();
        ui->indicator->restore();
    } else {
        ui->searchEdit->clear();
    }
}

void BrowserWindow::onSearchTextChanged(QString text)
{
    objectProxyModel->setFilterFixedString(text);

    if (text.isEmpty()) {
        ui->searchWidget->hide();
        ui->indicator->restore();
    }
}

void BrowserWindow::onChildClosed()
{
    ui->indicator->restore();
    ui->objectList->clearSelection();
    this->setEnabled(true);
}