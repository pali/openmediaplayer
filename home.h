#ifndef HOME_H
#define HOME_H

#include <QDialog>
#include <QDir>
#include <QFileInfo>
#include <QListWidgetItem>
#include <QMaemo5ValueButton>

#include "includes.h"
#include "ui_home.h"
#include "qmaemo5rotator.h"
#include "mediaart.h"

namespace Ui {
    class Home;
}

class Home : public QDialog
{
    Q_OBJECT

public:
    explicit Home(QWidget *parent = 0, QString target = "", QString path = "", QString album = "");
    ~Home();
    QMaemo5ValueButton* button;
    QString target;
    QString album;
    QString newAlbumArt;

public slots:
    void openBrowser(QString directory);

private:
    Ui::Home *ui;

private slots:
    void on_pushButton_clicked();
    void on_listWidget_itemClicked(QListWidgetItem* item);
};

#endif // HOME_H
