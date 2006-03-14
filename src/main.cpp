/*****************************************************************************
 *                                                                           *
 *   Copyright (C) 2005 by Chazal Francois             <neptune3k@free.fr>   *
 *   website : http://workspace.free.fr                                      *
 *                                                                           *
 *                     =========  GPL Licence  =========                     *
 *    This program is free software; you can redistribute it and/or modify   *
 *   it under the terms of the  GNU General Public License as published by   *
 *   the  Free  Software  Foundation ; either version 2 of the License, or   *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *****************************************************************************/

//== INCLUDE REQUIREMENTS =====================================================

/*
** KDE libraries */
#include <klocale.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kuniqueapplication.h>

/*
** Local libraries */
#include "main_window.h"



//== SOFTWARE INFORMATIONS ====================================================

static const char version[] = "2.7.3";
static const char description[] = I18N_NOOP("Yet Another KDE konsole which resembles those found in Quake.");
static KCmdLineOptions options[] =
    {
        KCmdLineLastOption
    };



//== SOFTWARE MAIN FUNCTION ===================================================

int main(int argc, char ** argv)
{
    KAboutData about("yakuake", I18N_NOOP("YaKuake"), version, description,
                     KAboutData::License_GPL, "(C) 2005 Francois Chazal", 0, 0, "neptune3k@free.fr");
    about.addAuthor("Francois Chazal",  0, "neptune3k@free.fr");
    about.addAuthor("Martin Galpin",    0, "martin@nemohackers.org");
    about.addAuthor("Thomas Tischler",  0, "Tischler123@t-online.de");
    about.addAuthor("Stefan Bogner",    0, "bochi@kmobiletools.org");
    about.addCredit("Georg Wittenburg", 0, "georg.wittenburg@gmx.net");
    about.addCredit("Dominik Seichter", 0, "domseichter@web.de");
    about.addCredit("Bert Speckels",    0, "bert@speckels.de");

    KCmdLineArgs::init(argc, argv, &about);
    KCmdLineArgs::addCmdLineOptions(options);
    KUniqueApplication::addCmdLineOptions();

    if (!KUniqueApplication::start())
    {
        kdDebug() << "Yakuake is already running !!!" << endl;
        return(0);
    }

    KUniqueApplication  app;
    MainWindow *        win = new MainWindow();

    win->hide();
    return app.exec();
}

