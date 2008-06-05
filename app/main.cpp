/*
  Copyright (C) 2008 by Eike Hein <hein@kde.org>

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
#include <KLocalizedString>
#include <KWindowSystem>

#include <QString>

#ifdef Q_WS_X11
#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>
#endif


#ifdef Q_WS_X11
void getDisplayInformation(Display*& display, Visual*& visual, Colormap& colormap);
#endif


int main (int argc, char *argv[])
{
    KAboutData aboutData("yakuake",
        0,
        ki18nc("@title", "<application>Yakuake</application>"),
        "2.9.3",
        ki18nc("@title", "A drop-down terminal emulator based on KDE Konsole technology."),
        KAboutData::License_GPL,
        ki18nc("@info:credit", "(c) 2008 The Yakuake Team"),
        ki18n(0),
        "http://yakuake.kde.org/");

    aboutData.setProductName("yakuake");

    aboutData.addAuthor(ki18nc("@info:credit", "Eike Hein"), 
        ki18nc("@info:credit", "Maintainer, Lead Developer"), "hein@kde.org");
    aboutData.addAuthor(ki18nc("@info:credit", "Francois Chazal"), 
        ki18nc("@info:credit", "Project Founder, Default skin (Inactive)"));
    aboutData.addCredit(ki18nc("@info:credit", "Daniel 'suslik' D."), 
        ki18nc("@info:credit", "Plastik skin"), "dd@accentsolution.com");

    KCmdLineArgs::init(argc, argv, &aboutData);

    if (!KUniqueApplication::start()) 
    {
        fprintf(stderr, "Yakuake is already running! Opening window ...\n");
        exit(0);
    }

#ifdef Q_WS_X11
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
#ifdef Q_WS_X11
void getDisplayInformation(Display*& display, Visual*& visual, Colormap& colormap)
{
    display = XOpenDisplay(0);

    if (!display)
    {
        fprintf(stderr, "Cannot connect to X server.\n");
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
