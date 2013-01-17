/**************************************************************************
    Open MediaPlayer
    Copyright (C) 2010-2012 Mohammad Abu-Garbeyyeh
                            Grzegorz Gidel
                            Matias Perez
                            Pali Rohár
                            Nicolai Hess
                            Timur Kristof

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

#include <QtGui/QApplication>

#include "mainwindow.h"
#include <QTime>
#include <QTextStream>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication::setOrganizationName("openmediaplayer");
    QApplication::setApplicationName("openmediaplayer");
    QApplication::setApplicationVersion("0.1");
    QApplication a(argc, argv);

    if (!QDBusConnection::sessionBus().isConnected())
        qWarning("Cannot connect to the D-Bus session bus.");

    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(DBUS_SERVICE)) {
        qWarning("Mediaplayer already running.");
        QDBusConnection::sessionBus().send(QDBusMessage::createMethodCall(DBUS_SERVICE, DBUS_PATH, DBUS_INTERFACE, "top_application"));
        return 0;
    }

    QString locale = QSettings().value("main/language").toString();
    QString langPath = "/opt/openmediaplayer/lang/";

    if (locale.isEmpty()) {
        // This thing returns the device language, not the current language
        //QString lang = QLocale::languageToString(QLocale::system().language());

        // Open locale file to check current locale
        QFile localeFile("/etc/osso-af-init/locale");
        if (localeFile.open(QFile::ReadOnly)) {
            QTextStream in(&localeFile);
            while(!in.atEnd()) {
                QString line = in.readLine();
                if (line.startsWith("export LANG")) {
                    locale = line.remove("export LANG=");
                    break;
                }
            }
        }
        localeFile.close();

        qDebug() << "Your locale is" << locale << "(detected)";
    } else {
        qDebug() << "Your locale is" << locale << "(forced)";
    }

    QLocale::setDefault(locale);

    // Install language file
    QTranslator translator;
    if (translator.load("openmediaplayer." + locale, langPath)) {
        qDebug() << "Translator successfully loaded";
        a.installTranslator(&translator);
    } else {
        qDebug() << "Translator failed to load";
        qDebug() << "Falling back to English";
        if (translator.load("openmediaplayer.en", langPath)) {
            qDebug() << "Translator successfully loaded";
            a.installTranslator(&translator);
        } else
            qDebug() << "Translator failed to load";
    }

    QTime t(0,0);
    t.start();
    // TODO: Add a full list of contributors here when we're ready to release.
    /*QTextStream out(stdout);
    out << "Open MediaPlayer, version: " << QApplication::applicationVersion() << " "
        << "Running with PID: " << QApplication::applicationPid() << endl
        << "Copyright (C) 2010-2011 Mohammad Abu-Garbeyyeh" << endl
        << "Licensed under GPLv3" << endl
        << "This program comes with ABSOLUTELY NO WARRANTY" << endl
        << "This is free software, and you are welcome to redistribute it" << endl
        << "under certain conditions; visit http://www.gnu.org/licenses/gpl.txt for details." << endl;*/
    MainWindow w;

#if defined(Q_WS_S60)
    w.showMaximized();
#else
    w.show();
#endif
    //out << QString("MediaPlayer startup took %1 ms").arg(t.elapsed()) << endl;
    return a.exec();
}
