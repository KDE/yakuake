/*
  SPDX-FileCopyrightText: 2008-2014 Eike Hein <hein@kde.org>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "mainwindow.h"

#include <KAboutData>
#include <KCrash>
#include <KDBusService>
#include <KLocalizedString>

#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    KLocalizedString::setApplicationDomain(QByteArrayLiteral("yakuake"));

    KAboutData aboutData(QStringLiteral("yakuake"),
                         xi18nc("@title", "<application>Yakuake</application>"),
                         QStringLiteral(YAKUAKE_VERSION),
                         xi18nc("@title", "A drop-down terminal emulator based on KDE Konsole technology."),
                         KAboutLicense::GPL,
                         xi18nc("@info:credit", "(c) 2008-2018 The Yakuake Team"),
                         QString(),
                         QStringLiteral("https://apps.kde.org/yakuake/"));

    aboutData.setOrganizationDomain(QByteArray("kde.org"));
    aboutData.setProductName(QByteArray("yakuake"));

    aboutData.addAuthor(xi18nc("@info:credit", "Eike Hein"), xi18nc("@info:credit", "Maintainer, Lead Developer"), QStringLiteral("hein@kde.org"));
    aboutData.addAuthor(xi18nc("@info:credit", "Francois Chazal"), xi18nc("@info:credit", "Project Founder, Legacy skin (Inactive)"));
    aboutData.addCredit(xi18nc("@info:credit", "Daniel 'suslik' D."), xi18nc("@info:credit", "Plastik skin"), QStringLiteral("dd@accentsolution.com"));
    aboutData.addCredit(xi18nc("@info:credit", "Juan Carlos Torres"),
                        xi18nc("@info:credit", "Tab bar drag and drop support, Prevent Closing toggle"),
                        QStringLiteral("carlosdgtorres@gmail.com"));
    aboutData.addCredit(xi18nc("@info:credit", "Gustavo Ribeiro Croscato"),
                        xi18nc("@info:credit", "Icon on tabs with Prevent Closing enabled"),
                        QStringLiteral("croscato@gmail.com"));
    aboutData.addCredit(xi18nc("@info:credit", "Danilo Cesar Lemes de Paula"),
                        xi18nc("@info:credit", "Actions to grow terminals"),
                        QStringLiteral("danilo.eu@gmail.com"));

    aboutData.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"), i18nc("EMAIL OF TRANSLATORS", "Your emails"));

    KAboutData::setApplicationData(aboutData);
    QCommandLineParser parser;

    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("yakuake")));

    KDBusService service(KDBusService::Unique);

    KCrash::initialize();
    MainWindow mainWindow;
    mainWindow.hide();
    QObject::connect(&service, &KDBusService::activateRequested, &mainWindow, &MainWindow::toggleWindowState);

    return app.exec();
}
