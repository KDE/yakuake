/*
  SPDX-FileCopyrightText: 2008-2014 Eike Hein <hein@kde.org>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "terminal.h"
#include "settings.h"

#include <KActionCollection>
#include <KColorScheme>
#include <KLocalizedString>
#include <KParts/PartLoader>
#include <KXMLGUIBuilder>
#include <KXMLGUIFactory>
#include <kde_terminal_interface.h>

#include <QAction>
#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QWidget>

#include <QKeyEvent>

int Terminal::m_availableTerminalId = 0;

Terminal::Terminal(const QString &workingDir, QWidget *parent)
    : QObject(nullptr)
{
    m_terminalId = m_availableTerminalId;
    m_availableTerminalId++;
    m_parentSplitter = parent;

    KPluginMetaData part(QStringLiteral("kf6/parts/konsolepart"));

    m_part = KParts::PartLoader::instantiatePart<KParts::Part>(part, parent).plugin;
    if (!m_part) {
        displayKPartLoadError();
        return;
    }

    connect(m_part, SIGNAL(setWindowCaption(QString)), this, SLOT(setTitle(QString)));
    connect(m_part, SIGNAL(overrideShortcut(QKeyEvent *, bool &)), this, SLOT(overrideShortcut(QKeyEvent *, bool &)));
    connect(m_part, &KParts::Part::destroyed, this, [this] {
        m_part = nullptr;

        if (!m_destroying) {
            Q_EMIT closeRequested(m_terminalId);
        }
    });

    m_partWidget = m_part->widget();

    m_terminalWidget = m_part->widget()->focusWidget();
    if (m_terminalWidget) {
        m_terminalWidget->setFocusPolicy(Qt::WheelFocus);
        m_terminalWidget->installEventFilter(this);

        if (!m_part->factory() && m_partWidget) {
            if (!m_part->clientBuilder()) {
                m_part->setClientBuilder(new KXMLGUIBuilder(m_partWidget));
            }

            auto factory = new KXMLGUIFactory(m_part->clientBuilder(), this);
            factory->addClient(m_part);

            // Prevents the KXMLGui warning about removing the client
            connect(m_partWidget, &QObject::destroyed, this, [factory, this] {
                factory->removeClient(m_part);
            });
        }
    }

    disableOffendingPartActions();

    m_terminalInterface = qobject_cast<TerminalInterface *>(m_part);
    if (!m_terminalInterface) {
        qFatal("Version of Konsole is outdated. Konsole didn't return a valid TerminalInterface.");
        return;
    }

    bool startInWorkingDir = m_terminalInterface->profileProperty(QStringLiteral("StartInCurrentSessionDir")).toBool();
    if (startInWorkingDir && !workingDir.isEmpty()) {
        m_terminalInterface->showShellInDir(workingDir);
    }

    QMetaObject::invokeMethod(m_part, "isBlurEnabled", Qt::DirectConnection, Q_RETURN_ARG(bool, m_wantsBlur));

    // Remove shortcut from close action because it conflicts with the shortcut from out own close action
    // https://bugs.kde.org/show_bug.cgi?id=319172
    const auto childClients = m_part->childClients();
    for (const auto childClient : childClients) {
        QAction *closeSessionAction = childClient->actionCollection()->action(QStringLiteral("close-session"));
        if (closeSessionAction) {
            closeSessionAction->setShortcut(QKeySequence());
        }
    }
}

Terminal::~Terminal()
{
    m_destroying = true;
    // The ownership of m_part is a mess
    // When the terminal exits, e.g. the user pressed Ctrl+D, the part deletes itself
    // When we close a terminal we need to delete the part ourselves
    if (m_part) {
        delete m_part;
    }
}

bool Terminal::eventFilter(QObject * /* watched */, QEvent *event)
{
    if (event->type() == QEvent::FocusIn) {
        Q_EMIT activated(m_terminalId);

        QFocusEvent *focusEvent = static_cast<QFocusEvent *>(event);

        if (focusEvent->reason() == Qt::MouseFocusReason || focusEvent->reason() == Qt::OtherFocusReason || focusEvent->reason() == Qt::BacktabFocusReason)
            Q_EMIT manuallyActivated(this);
    } else if (event->type() == QEvent::MouseMove) {
        if (Settings::focusFollowsMouse() && m_terminalWidget && !m_terminalWidget->hasFocus())
            m_terminalWidget->setFocus();
    }

    if (!m_keyboardInputEnabled) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

            if (keyEvent->modifiers() == Qt::NoModifier)
                Q_EMIT keyboardInputBlocked(this);

            return true;
        } else if (event->type() == QEvent::KeyRelease)
            return true;
    }

    return false;
}

void Terminal::displayKPartLoadError()
{
    KColorScheme colorScheme(QPalette::Active);
    QColor warningColor = colorScheme.background(KColorScheme::NeutralBackground).color();
    QColor warningColorLight = KColorScheme::shade(warningColor, KColorScheme::LightShade, 0.1);
    QString gradient = QStringLiteral("qlineargradient(x1:0, y1:0, x2:0, y2:1,stop: 0 %1, stop: 0.6 %1, stop: 1.0 %2)");
    gradient = gradient.arg(warningColor.name(), warningColorLight.name());
    QString styleSheet = QStringLiteral("QLabel { background: %1; }");

    QWidget *widget = new QWidget(m_parentSplitter);
    widget->setStyleSheet(styleSheet.arg(gradient));
    m_partWidget = widget;
    m_terminalWidget = widget;
    m_terminalWidget->setFocusPolicy(Qt::WheelFocus);
    m_terminalWidget->installEventFilter(this);

    QLabel *label = new QLabel(widget);
    label->setContentsMargins(10, 10, 10, 10);
    label->setWordWrap(false);
    label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setText(xi18nc("@info",
                          "<application>Yakuake</application> was unable to load "
                          "the <application>Konsole</application> component.<nl/> "
                          "A <application>Konsole</application> installation is "
                          "required to use Yakuake."));

    QLabel *icon = new QLabel(widget);
    icon->setContentsMargins(10, 10, 10, 10);
    icon->setPixmap(QIcon::fromTheme(QStringLiteral("dialog-warning")).pixmap(QSize(48, 48)));
    icon->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    QHBoxLayout *layout = new QHBoxLayout(widget);
    layout->addWidget(icon);
    layout->addWidget(label);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setStretchFactor(icon, 1);
    layout->setStretchFactor(label, 5);
}

void Terminal::disableOffendingPartActions()
{
    // This is an unwelcome stop-gap that will be removed once we can
    // count on a Konsole version that doesn't pollute a KPart user's
    // shortcut "namespace".

    if (!m_part)
        return;

    KActionCollection *actionCollection = m_part->actionCollection();

    if (actionCollection) {
        QAction *action = nullptr;

        action = actionCollection->action(QStringLiteral("next-view"));
        if (action)
            action->setEnabled(false);

        action = actionCollection->action(QStringLiteral("previous-view"));
        if (action)
            action->setEnabled(false);

        action = actionCollection->action(QStringLiteral("close-active-view"));
        if (action)
            action->setEnabled(false);

        action = actionCollection->action(QStringLiteral("split-view-left-right"));
        if (action)
            action->setEnabled(false);

        action = actionCollection->action(QStringLiteral("split-view-top-bottom"));
        if (action)
            action->setEnabled(false);

        action = actionCollection->action(QStringLiteral("rename-session"));
        if (action)
            action->setEnabled(false);

        action = actionCollection->action(QStringLiteral("enlarge-font"));
        if (action)
            action->setEnabled(false);

        action = actionCollection->action(QStringLiteral("shrink-font"));
        if (action)
            action->setEnabled(false);
    }
}

void Terminal::setTitle(const QString &title)
{
    m_title = title;

    Q_EMIT titleChanged(m_terminalId, m_title);
}

void Terminal::runCommand(const QString &command)
{
    m_terminalInterface->sendInput(command + QStringLiteral("\n"));
}

void Terminal::manageProfiles()
{
    QMetaObject::invokeMethod(m_part, "showManageProfilesDialog", Qt::QueuedConnection, Q_ARG(QWidget *, QApplication::activeWindow()));
}

void Terminal::editProfile()
{
    QMetaObject::invokeMethod(m_part, "showEditCurrentProfileDialog", Qt::QueuedConnection, Q_ARG(QWidget *, QApplication::activeWindow()));
}

void Terminal::overrideShortcut(QKeyEvent * /* event */, bool &override)
{
    override = false;
}

void Terminal::setMonitorActivityEnabled(bool enabled)
{
    m_monitorActivityEnabled = enabled;

    if (enabled) {
        connect(m_part, SIGNAL(activityDetected()), this, SLOT(activityDetected()), Qt::UniqueConnection);

        QMetaObject::invokeMethod(m_part, "setMonitorActivityEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
    } else {
        disconnect(m_part, SIGNAL(activityDetected()), this, SLOT(activityDetected()));

        QMetaObject::invokeMethod(m_part, "setMonitorActivityEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
    }
}

void Terminal::setMonitorSilenceEnabled(bool enabled)
{
    m_monitorSilenceEnabled = enabled;

    if (enabled) {
        connect(m_part, SIGNAL(silenceDetected()), this, SLOT(silenceDetected()), Qt::UniqueConnection);

        QMetaObject::invokeMethod(m_part, "setMonitorSilenceEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
    } else {
        disconnect(m_part, SIGNAL(silenceDetected()), this, SLOT(silenceDetected()));

        QMetaObject::invokeMethod(m_part, "setMonitorSilenceEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
    }
}

void Terminal::activityDetected()
{
    Q_EMIT activityDetected(this);
}

void Terminal::silenceDetected()
{
    Q_EMIT silenceDetected(this);
}

QString Terminal::currentWorkingDirectory() const
{
    return m_terminalInterface->currentWorkingDirectory();
}

KActionCollection *Terminal::actionCollection()
{
    if (m_part->factory()) {
        const auto guiClients = m_part->childClients();
        for (auto *client : guiClients) {
            if (client->actionCollection()->associatedWidgets().contains(m_terminalWidget)) {
                return client->actionCollection();
            }
        }
    }

    return nullptr;
}

#include "moc_terminal.cpp"
