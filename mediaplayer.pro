#-------------------------------------------------
#
# Project created by QtCreator 2010-12-07T20:00:31
#
#-------------------------------------------------

QT       += core gui dbus

TARGET = mediaplayer
TEMPLATE = app
#DEFINES += DEBUG


SOURCES += main.cpp\
        mainwindow.cpp \
    musicwindow.cpp \
    nowplayingwindow.cpp \
    videoswindow.cpp \
    internetradiowindow.cpp \
    share.cpp \
    mafwrenderersignalhelper.cpp \
    mafwrendereradapter.cpp \
    fmtxdialog.cpp \
    freqpickselector.cpp \
    videonowplayingwindow.cpp

HEADERS  += mainwindow.h \
    musicwindow.h \
    nowplayingwindow.h \
    videoswindow.h \
    internetradiowindow.h \
    share.h \
    mafwrenderersignalhelper.h \
    mafwrendereradapter.h \
    fmtxdialog.h \
    freqpickselector.h \
    videonowplayingwindow.h \
    mirror.h

FORMS    += mainwindow.ui \
    musicwindow.ui \
    nowplayingwindow.ui \
    videoswindow.ui \
    internetradiowindow.ui \
    share.ui \
    fmtxdialog.ui \
    videonowplayingwindow.ui

CONFIG += mobility
MOBILITY =

symbian {
    TARGET.UID3 = 0xedf29700
    # TARGET.CAPABILITY += 
    TARGET.EPOCSTACKSIZE = 0x14000
    TARGET.EPOCHEAPSIZE = 0x020000 0x800000
}

maemo5 {
    CONFIG += link_pkgconfig
    PKGCONFIG += libosso mafw mafw-shared glib-2.0
    QT += maemo5
}

RESOURCES += \
    images.qrc
