/*
  Copyright (C) 2008-2014 by Eike Hein <hein@kde.org>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License or (at your option) version 3 or any later version
  accepted by the membership of KDE e.V. (or its successor appro-
  ved by the membership of KDE e.V.), which shall act as a proxy
  defined in Section 14 of version 3 of the license.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see http://www.gnu.org/licenses/.
*/

#include "mainwindow.h"

#include <KAboutData>
#include <KCrash>
#include <KDBusService>
#include <KLocalizedString>

#include <QApplication>


int main (int argc, char *argv[])
{
#if HAVE_X11
    qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("xcb"));
#endif
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    KLocalizedString::setApplicationDomain("yakuake");

    KAboutData aboutData(QStringLiteral("yakuake"),
        xi18nc("@title", "<application>Yakuake</application>"),
        QStringLiteral("3.0.2+"),
        xi18nc("@title", "A drop-down terminal emulator based on KDE Konsole technology."),
        KAboutLicense::GPL,
        xi18nc("@info:credit", "(c) 2008-2016 The Yakuake Team"),
        QString(),
        QStringLiteral("http://yakuake.kde.org/"));

    aboutData.setOrganizationDomain(QByteArray("kde.org"));
    aboutData.setProductName(QByteArray("yakuake"));

    aboutData.addAuthor(xi18nc("@info:credit", "Eike Hein"),
        xi18nc("@info:credit", "Maintainer, Lead Developer"), QStringLiteral("hein@kde.org"));
    aboutData.addAuthor(xi18nc("@info:credit", "Francois Chazal"),
        xi18nc("@info:credit", "Project Founder, Legacy skin (Inactive)"));
    aboutData.addCredit(xi18nc("@info:credit", "Daniel 'suslik' D."),
        xi18nc("@info:credit", "Plastik skin"), QStringLiteral("dd@accentsolution.com"));
    aboutData.addCredit(xi18nc("@info:credit", "Juan Carlos Torres"),
        xi18nc("@info:credit", "Tab bar drag and drop support, Prevent Closing toggle"), QStringLiteral("carlosdgtorres@gmail.com"));
    aboutData.addCredit(xi18nc("@info:credit", "Gustavo Ribeiro Croscato"),
        xi18nc("@info:credit", "Icon on tabs with Prevent Closing enabled"), QStringLiteral("croscato@gmail.com"));
    aboutData.addCredit(xi18nc("@info:credit", "Danilo Cesar Lemes de Paula"),
        xi18nc("@info:credit", "Actions to grow terminals"), QStringLiteral("danilo.eu@gmail.com"));

    aboutData.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"),
                            i18nc("EMAIL OF TRANSLATORS", "Your emails"));

    KAboutData::setApplicationData(aboutData);

    app.setApplicationName(aboutData.componentName());
    app.setApplicationDisplayName(aboutData.displayName());
    app.setOrganizationDomain(aboutData.organizationDomain());
    app.setApplicationVersion(aboutData.version());
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("yakuake")));

    KDBusService service(KDBusService::Unique);

    KCrash::initialize();
    MainWindow mainWindow;
    mainWindow.hide();
    QObject::connect(&service, SIGNAL(activateRequested(QStringList, QString)), &mainWindow, SLOT(toggleWindowState()));

    return app.exec();
}
