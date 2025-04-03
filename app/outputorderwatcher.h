/*
    SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QAbstractNativeEventFilter>
#include <QGuiApplication>
#include <QObject>

// #include "kworkspace_export.h"
// #include <config-outputorder.h>

#define HAVE_X11 1

#if HAVE_X11
#include <xcb/xcb.h>
#endif

class QScreen;
class QTimer;

/**
 * This class watches for output ordering changes from
 * the relevant backend
 */
class OutputOrderWatcher : public QObject
{
    Q_OBJECT
public:
    /**
     * Create the correct OutputOrderWatcher
     */
    static OutputOrderWatcher *instance(QObject *parent);

    /**
     * Returns the list of outputs in order
     *
     * At the time of change signalling all Screens are
     * guaranteed to exist in QGuiApplication.
     *
     * After screen removal this can temporarily contain
     * dead entries.
     */
    QStringList outputOrder() const;

    /**
     * @internal
     * For X11 we know libkscreen takes a server grab whilst changing properties.
     * This means we know at the time of any runtime screen addition and removal the priorities will
     * be up-to-date when we poll them.
     *
     * For dynamic ordering changes without screen changes we will get the change property multiple times.
     * A timer is used to batch them.
     *
     *  A caveat on X11 is the initial startup where plasmashell may query screen information before
     * kscreen has set properties.
     * This should resolve itself as a dynamic re-ordering when kscreen does start.
     *
     * For wayland we know kwin sends the priority order whenever screen changes are made.
     * As creating screens requires an extra async call to kwin (to bind to the output)
     * we always get the new priority ordering before and screen additions.
     * We should always have correct values on startup.
     */
    virtual void refresh();
Q_SIGNALS:
    void outputOrderChanged(const QStringList &outputOrder);

protected:
    OutputOrderWatcher(QObject *parent);
    /**
     * Backend failed, use QScreen based implementaion
     */
    void useFallback(bool fallback, const char *reason = nullptr);

    QStringList m_outputOrder;
    bool m_orderProtocolPresent = false;

private:
};

#if HAVE_X11
class X11OutputOrderWatcher : public OutputOrderWatcher, public QAbstractNativeEventFilter
{
    Q_OBJECT
public:
    X11OutputOrderWatcher(QObject *parent);
    void refresh() override;

protected:
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

private:
    void roundtrip() const;

    QNativeInterface::QX11Application *m_x11Interface = nullptr;
    QTimer *m_delayTimer = nullptr; // This is just to simulate the protocol that will be guaranteed to always arrive after screens additions and removals
    // Xrandr
    int m_xrandrExtensionOffset;
    xcb_atom_t m_kdeScreenAtom = XCB_ATOM_NONE;
};
#endif

class WaylandOutputOrderWatcher : public OutputOrderWatcher
{
    Q_OBJECT
public:
    WaylandOutputOrderWatcher(QObject *parent);
    void refresh() override;

private:
    bool hasAllScreens() const;
    QStringList m_pendingOutputOrder;
};
