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
#include <KWindowSystem>

#include <QTextStream>

#include <cstdio>

#if defined(Q_WS_X11) && !KDE_IS_VERSION(4,2,68)
#include <cstdlib>

#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>


void getDisplayInformation(Display*& display, Visual*& visual, Colormap& colormap);
#endif


int main (int argc, char *argv[])
{
    KAboutData aboutData("yakuake",
        0,
        ki18nc("@title", "<application>Yakuake</application>"),
        "2.9.6+",
        ki18nc("@title", "A drop-down terminal emulator based on KDE Konsole technology."),
        KAboutData::License_GPL,
        ki18nc("@info:credit", "(c) 2008-2009 The Yakuake Team"),
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

    KCmdLineArgs::init(argc, argv, &aboutData);

    if (!KUniqueApplication::start())
    {
        QTextStream err(stderr);

        err << i18nc("@info:shell", "Yakuake is already running, opening window....") << '\n';

        return 0;
    }

#if defined(Q_WS_X11) && !KDE_IS_VERSION(4,2,68)
    if (KWindowSystem::compositingActive())
    {
            Display* display = 0;
            Visual* visual = 0;
            Colormap colormap = 0;

            getDisplayInformation(display, visual, colormap);

            Application app(display, (Qt::HANDLE)visual, (Qt::HANDLE)colormap);

            return app.exec();
    }
    else
#endif
    {
        Application app;

        return app.exec();
    }
}

// Code from the Qt 4 graphics dojo examples at http://labs.trolltech.com
#if defined(Q_WS_X11) && !KDE_IS_VERSION(4,2,68)
void getDisplayInformation(Display*& display, Visual*& visual, Colormap& colormap)
{
    display = XOpenDisplay(0);

    if (!display)
    {
        QTextStream err(stderr);

        err << i18nc("@info:shell", "Cannot connect to X server.") << '\n';

        exit(1);
    }

    int screen = DefaultScreen(display);
    int eventBase, errorBase;

    if (XRenderQueryExtension(display, &eventBase, &errorBase))
    {
        int nvi;
        XVisualInfo templ;
        templ.screen  = screen;
        templ.depth = 32;
        templ.c_class = TrueColor;
        XVisualInfo *xvi = XGetVisualInfo(display, VisualScreenMask |
                                          VisualDepthMask |
                                          VisualClassMask, &templ, &nvi);

        for (int i = 0; i < nvi; ++i)
        {
            XRenderPictFormat* format = XRenderFindVisualFormat(display, xvi[i].visual);

            if (format->type == PictTypeDirect && format->direct.alphaMask)
            {
                visual = xvi[i].visual;
                colormap = XCreateColormap(display, RootWindow(display, screen),
                                           visual, AllocNone);

                // Found ARGB visual.
                break;
            }
        }
    }
}
#endif
