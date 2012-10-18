/*
  Copyright (C) 2008-2009 by Eike Hein <hein@kde.org>

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


#include "application.h"

#include <KAboutData>
#include <KCmdLineArgs>

#include <QTextStream>

#include <cstdio>


int main (int argc, char *argv[])
{
    KAboutData aboutData("yakuake",
        0,
        ki18nc("@title", "<application>Yakuake</application>"),
        "2.9.9",
        ki18nc("@title", "A drop-down terminal emulator based on KDE Konsole technology."),
        KAboutData::License_GPL,
        ki18nc("@info:credit", "(c) 2008-2012 The Yakuake Team"),
        ki18n(0),
        "http://yakuake.kde.org/");

    aboutData.setProductName("yakuake");

    aboutData.addAuthor(ki18nc("@info:credit", "Eike Hein"),
        ki18nc("@info:credit", "Maintainer, Lead Developer"), "hein@kde.org");
    aboutData.addAuthor(ki18nc("@info:credit", "Francois Chazal"),
        ki18nc("@info:credit", "Project Founder, Default skin (Inactive)"));
    aboutData.addCredit(ki18nc("@info:credit", "Daniel 'suslik' D."),
        ki18nc("@info:credit", "Plastik skin"), "dd@accentsolution.com");
    aboutData.addCredit(ki18nc("@info:credit", "Juan Carlos Torres"),
        ki18nc("@info:credit", "Tab bar drag and drop support, Prevent Closing toggle"), "carlosdgtorres@gmail.com");
    aboutData.addCredit(ki18nc("@info:credit", "Gustavo Ribeiro Croscato"),
        ki18nc("@info:credit", "Icon on tabs with Prevent Closing enabled"), "croscato@gmail.com");
    aboutData.addCredit(ki18nc("@info:credit", "Danilo Cesar Lemes de Paula"),
        ki18nc("@info:credit", "Actions to grow terminals"), "danilo.eu@gmail.com");

    KCmdLineArgs::init(argc, argv, &aboutData);

    if (!KUniqueApplication::start())
    {
        QTextStream err(stderr);

        err << i18nc("@info:shell", "Yakuake is already running, toggling window ...") << '\n';

        return 0;
    }

    Application app;

    return app.exec();
}
