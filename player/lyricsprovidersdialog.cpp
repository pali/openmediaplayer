#include "lyricsprovidersdialog.h"

LyricsProvidersDialog::LyricsProvidersDialog(QString state, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LyricsProvidersDialog)
{
    ui->setupUi(this);

    ui->upButton->setIcon(QIcon::fromTheme("keyboard_move_up"));
    ui->checkButton->setIcon(QIcon::fromTheme("widgets_tickmark_grid"));
    ui->downButton->setIcon(QIcon::fromTheme("keyboard_move_down"));
    ui->providersList->setItemDelegate(new ProviderListItemDelegate(ui->providersList));

    QMap<QString,QString> availableProviders = LyricsManager::listProviders();
    QStringList configList = state.split(',', QString::SkipEmptyParts);

    foreach(QString config, configList) {
        bool active;

        if (active = config.startsWith('+'))
            config = config.mid(1);

        if (availableProviders.contains(config))
            addProvider(config, availableProviders.value(config), active);

        availableProviders.remove(config);
    }

    foreach (QString provider, availableProviders.keys())
        addProvider(provider, availableProviders.value(provider), false);

    if (ui->providersList->count()) {
        connect(ui->checkButton, SIGNAL(toggled(bool)), this, SLOT(checkProvider(bool)));
        connect(ui->upButton, SIGNAL(clicked()), this, SLOT(moveProviderUp()));
        connect(ui->downButton, SIGNAL(clicked()), this, SLOT(moveProviderDown()));
        connect(ui->providersList, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
                this, SLOT(onProviderChanged(QListWidgetItem*)));

        ui->providersList->setCurrentRow(0);
    } else {
        ui->checkButton->setEnabled(false);
        ui->upButton->setEnabled(false);
        ui->downButton->setEnabled(false);
    }

    Rotator::acquire()->addClient(this);
}

LyricsProvidersDialog::~LyricsProvidersDialog()
{
    delete ui;
}

void LyricsProvidersDialog::addProvider(QString name, QString description, bool active)
{
    QListWidgetItem *item = new QListWidgetItem();

    item->setText(name);
    item->setData(UserRoleValueText, description);
    item->setData(Qt::UserRole, active);

    ui->providersList->addItem(item);
}


void LyricsProvidersDialog::checkProvider(bool checked)
{
    QListWidgetItem *item = ui->providersList->currentItem();
    item->setData(Qt::UserRole, checked);
}

void LyricsProvidersDialog::onProviderChanged(QListWidgetItem *item)
{
    ui->checkButton->setChecked(item->data(Qt::UserRole).toBool());
}

void LyricsProvidersDialog::moveProviderUp()
{
    int row = ui->providersList->currentRow();
    if (row > 0) {
        ui->providersList->insertItem(row-1, ui->providersList->takeItem(row));
        ui->providersList->setCurrentRow(row-1);
    }
}

void LyricsProvidersDialog::moveProviderDown()
{
    int row = ui->providersList->currentRow();
    if (row < ui->providersList->count()-1) {
        ui->providersList->insertItem(row+1, ui->providersList->takeItem(row));
        ui->providersList->setCurrentRow(row+1);
    }
}

QString LyricsProvidersDialog::state()
{
    QString state;

    for (int i = 0; i < ui->providersList->count(); i++) {
        QListWidgetItem *item = ui->providersList->item(i);
        if (item->data(Qt::UserRole).toBool())
            state.append('+' + item->text() + ',');
        else
            state.append(item->text() + ',');
    }

    return state;
}

void LyricsProvidersDialog::onOrientationChanged(int w, int h)
{
    if (w < h) { // Portrait
        ui->mainLayout->removeWidget(ui->controlsWidget);
        ui->controlsLayout->setDirection(QBoxLayout::LeftToRight);
        ui->mainLayout->addWidget(ui->controlsWidget, 1, 0);
    } else { // Landscape
        ui->mainLayout->removeWidget(ui->controlsWidget);
        ui->controlsLayout->setDirection(QBoxLayout::TopToBottom);
        ui->mainLayout->addWidget(ui->controlsWidget, 0, 1);
    }
}
