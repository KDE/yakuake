/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "outputorderwatcher.h"

#include <ranges>

#include <QScreen>
#include <QTimer>

#include <KWindowSystem>

#include "qwayland-kde-output-order-v1.h"
#include <QtWaylandClient/QWaylandClientExtension>
#include <QtWaylandClient/QtWaylandClientVersion>

#if HAVE_X11
#include <X11/Xlib.h>
#include <xcb/randr.h>
#include <xcb/xcb_event.h>
#endif // HAVE_X11

template<typename T>
using ScopedPointer = QScopedPointer<T, QScopedPointerPodDeleter>;

class WaylandOutputOrder : public QWaylandClientExtensionTemplate<WaylandOutputOrder, &QtWayland::kde_output_order_v1::destroy>,
                           public QtWayland::kde_output_order_v1
{
    Q_OBJECT
public:
    WaylandOutputOrder(QObject *parent)
        : QWaylandClientExtensionTemplate(1)
    {
        setParent(parent);
        initialize();
    }

protected:
    void kde_output_order_v1_output(const QString &outputName) override
    {
        if (m_done) {
            m_outputOrder.clear();
            m_done = false;
        }
        m_outputOrder.append(outputName);
    }

    void kde_output_order_v1_done() override
    {
        // If no output arrived it means we don't have *any* usable output
        if (m_done) {
            m_outputOrder.clear();
        }
        m_done = true;
        Q_EMIT outputOrderChanged(m_outputOrder);
    }

Q_SIGNALS:
    void outputOrderChanged(const QStringList &outputName);

private:
    QStringList m_outputOrder;
    bool m_done = true;
};

OutputOrderWatcher::OutputOrderWatcher(QObject *parent)
    : QObject(parent)
{
    connect(qGuiApp, &QGuiApplication::screenAdded, this, &OutputOrderWatcher::refresh);
    connect(qGuiApp, &QGuiApplication::screenRemoved, this, &OutputOrderWatcher::refresh);
}

void OutputOrderWatcher::useFallback(bool fallback, const char *reason)
{
    m_orderProtocolPresent = !fallback;
    if (fallback) {
        if (reason) {
            qCritical() << "OutputOrderWatcher may not work as expected. Reason:" << reason;
        }
        connect(qGuiApp, &QGuiApplication::primaryScreenChanged, this, &OutputOrderWatcher::refresh, Qt::UniqueConnection);
        refresh();
    }
}

OutputOrderWatcher *OutputOrderWatcher::instance(QObject *parent)
{
#if HAVE_X11
    if (KWindowSystem::isPlatformX11()) {
        return new X11OutputOrderWatcher(parent);
    } else
#endif
        if (KWindowSystem::isPlatformWayland()) {
        return new WaylandOutputOrderWatcher(parent);
    }
    // return default impl that does something at least
    return new OutputOrderWatcher(parent);
}

void OutputOrderWatcher::refresh()
{
    Q_ASSERT(!m_orderProtocolPresent);

    QStringList pendingOutputOrder;

    pendingOutputOrder.clear();
    for (auto *s : qApp->screens()) {
        pendingOutputOrder.append(s->name());
    }

    auto outputLess = [](const QString &c1, const QString &c2) {
        if (c1 == qApp->primaryScreen()->name()) {
            return true;
        } else if (c2 == qApp->primaryScreen()->name()) {
            return false;
        } else {
            return c1 < c2;
        }
    };
    std::sort(pendingOutputOrder.begin(), pendingOutputOrder.end(), outputLess);

    if (m_outputOrder != pendingOutputOrder) {
        m_outputOrder = pendingOutputOrder;
        Q_EMIT outputOrderChanged(m_outputOrder);
    }
    return;
}

QStringList OutputOrderWatcher::outputOrder() const
{
    return m_outputOrder;
}

X11OutputOrderWatcher::X11OutputOrderWatcher(QObject *parent)
    : OutputOrderWatcher(parent)
    , m_x11Interface(qGuiApp->nativeInterface<QNativeInterface::QX11Application>())
{
    if (!m_x11Interface) [[unlikely]] {
        Q_ASSERT(false);
        return;
    }
    // This timer is used to signal only when a qscreen for every output is already created, perhaps by monitoring
    // screenadded/screenremoved and tracking the outputs still missing
    m_delayTimer = new QTimer(this);
    m_delayTimer->setSingleShot(true);
    m_delayTimer->setInterval(0);
    connect(m_delayTimer, &QTimer::timeout, this, [this]() {
        refresh();
    });

    // By default try to use the protocol on x11
    m_orderProtocolPresent = true;

    qGuiApp->installNativeEventFilter(this);
    const xcb_query_extension_reply_t *reply = xcb_get_extension_data(m_x11Interface->connection(), &xcb_randr_id);
    if (!reply || !reply->present) { // SENTRY PLASMA-WORKSPACE-1MMC: XRandr extension is not initialized when using vncserver
        useFallback(true, "XRandr extension is not initialized");
        return;
    }

    m_xrandrExtensionOffset = reply->first_event;

    constexpr const char *effectName = "_KDE_SCREEN_INDEX";
    xcb_intern_atom_cookie_t atomCookie =
        xcb_intern_atom_unchecked(m_x11Interface->connection(), false, std::char_traits<char>::length(effectName), effectName);
    xcb_intern_atom_reply_t *atom(xcb_intern_atom_reply(m_x11Interface->connection(), atomCookie, nullptr));
    if (!atom) {
        useFallback(true);
        return;
    }

    m_kdeScreenAtom = atom->atom;
    m_delayTimer->start();
}

void X11OutputOrderWatcher::refresh()
{
    if (!m_orderProtocolPresent) {
        OutputOrderWatcher::refresh();
        return;
    }
    QList<std::pair<uint, QString>> orderMap;

    ScopedPointer<xcb_randr_get_screen_resources_current_reply_t> reply(xcb_randr_get_screen_resources_current_reply(
        m_x11Interface->connection(),
        xcb_randr_get_screen_resources_current(m_x11Interface->connection(), DefaultRootWindow(m_x11Interface->display())),
        NULL));

    xcb_timestamp_t timestamp = reply->config_timestamp;
    int len = xcb_randr_get_screen_resources_current_outputs_length(reply.data());
    xcb_randr_output_t *randr_outputs = xcb_randr_get_screen_resources_current_outputs(reply.data());

    for (int i = 0; i < len; i++) {
        ScopedPointer<xcb_randr_get_output_info_reply_t> output(
            xcb_randr_get_output_info_reply(m_x11Interface->connection(),
                                            xcb_randr_get_output_info(m_x11Interface->connection(), randr_outputs[i], timestamp),
                                            NULL));

        if (output == NULL || output->connection == XCB_RANDR_CONNECTION_DISCONNECTED || output->crtc == 0) {
            continue;
        }

        auto orderCookie = xcb_randr_get_output_property(m_x11Interface->connection(), randr_outputs[i], m_kdeScreenAtom, XCB_ATOM_ANY, 0, 100, false, false);
        ScopedPointer<xcb_randr_get_output_property_reply_t> orderReply(
            xcb_randr_get_output_property_reply(m_x11Interface->connection(), orderCookie, nullptr));
        // If there is even a single screen without _KDE_SCREEN_INDEX info, fall back to alphabetical ordering
        if (!orderReply) {
            useFallback(true);
            return;
        }

        if (!(orderReply->type == XCB_ATOM_INTEGER && orderReply->format == 32 && orderReply->num_items == 1)) {
            useFallback(true);
            return;
        }

        const uint32_t order = *xcb_randr_get_output_property_data(orderReply.data());

        if (order > 0) { // 0 is the special case for disabled, so we ignore it
            orderMap.emplace_back(order,
                                  QString::fromUtf8(reinterpret_cast<const char *>(xcb_randr_get_output_info_name(output.get())),
                                                    xcb_randr_get_output_info_name_length(output.get())));
        }
    }

    const auto screens = qGuiApp->screens();
    std::vector<QString> screenNames;
    screenNames.reserve(screens.size());
    std::transform(screens.begin(), screens.end(), std::back_inserter(screenNames), [](const QScreen *screen) {
        return screen->name();
    });
    const bool isScreenPresent = std::all_of(orderMap.cbegin(), orderMap.cend(), [&screenNames](const auto &pr) {
        return std::ranges::find(screenNames, std::get<QString>(pr)) != screenNames.end();
    });
    if (!isScreenPresent) [[unlikely]] {
        // if the pending output order refers to screens
        // we don't know of yet, try again next time a screen is added
        // this seems unlikely given we have the server lock and the timing thing
        m_delayTimer->start();
        return;
    }

    std::sort(orderMap.begin(), orderMap.end());

    // Rather verbose ifdef due to clang support of ranges API
#if defined(__clang__) && __clang_major__ < 16
    const auto getAllValues = [](const QList<std::pair<uint, QString>> &orderMap) -> QList<QString> {
        QList<QString> values;
        values.reserve(orderMap.size());
        std::transform(orderMap.begin(), orderMap.end(), std::back_inserter(values), [](const auto &pair) {
            return pair.second;
        });
        return values;
    };
    if (const auto pendingOutputs = getAllValues(orderMap); pendingOutputs != m_outputOrder) {
        m_outputOrder = pendingOutputs;
#else
    if (const auto pendingOutputs = std::views::values(std::as_const(orderMap)); !std::ranges::equal(pendingOutputs, std::as_const(m_outputOrder))) {
        m_outputOrder = QStringList{pendingOutputs.begin(), pendingOutputs.end()};
#endif
        Q_EMIT outputOrderChanged(m_outputOrder);
    }
}

bool X11OutputOrderWatcher::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(result);
    // a particular edge case: when we switch the only enabled screen
    // we don't have any signal about it, the primary screen changes but we have the same old QScreen* getting recycled
    // see https://bugs.kde.org/show_bug.cgi?id=373880
    // if this slot will be invoked many times, their//second time on will do nothing as name and primaryOutputName will be the same by then
    if (eventType[0] != 'x') {
        return false;
    }

    xcb_generic_event_t *ev = static_cast<xcb_generic_event_t *>(message);

    const auto responseType = XCB_EVENT_RESPONSE_TYPE(ev);

    if (responseType == m_xrandrExtensionOffset + XCB_RANDR_NOTIFY) {
        auto *randrEvent = reinterpret_cast<xcb_randr_notify_event_t *>(ev);
        if (randrEvent->subCode == XCB_RANDR_NOTIFY_OUTPUT_PROPERTY) {
            xcb_randr_output_property_t property = randrEvent->u.op;

            if (property.atom == m_kdeScreenAtom) {
                // Force an X11 roundtrip to make sure we have all other
                // screen events in the buffer when we process the deferred refresh
                useFallback(false);
                roundtrip();
                m_delayTimer->start();
            }
        } else if (randrEvent->subCode == XCB_RANDR_NOTIFY_OUTPUT_CHANGE) {
            // When the ast screen is removed, its qscreen becomes name ":0.0" as the fake screen, but nothing happens really,
            // screenpool doesn't notice (and looking at the assert_x there are, that was expected"
            // then the screen gets connected again, a new screen gets conencted, the old 0.0 one
            // gets disconnected, but the screen order stuff doesn't say anything as it's still
            // the same connector name as before
            // so screenpool finds itself with an empty screenorder
            if (randrEvent->u.oc.connection == XCB_RANDR_CONNECTION_DISCONNECTED) {
                // Cause ScreenPool to reevaluate screenorder again, so the screen :0.0 will
                // be correctly moved to fakeScreens
                m_delayTimer->start();
            }
        }
    }
    return false;
}

void X11OutputOrderWatcher::roundtrip() const
{
    const auto cookie = xcb_get_input_focus(m_x11Interface->connection());
    xcb_generic_error_t *error = nullptr;
    ScopedPointer<xcb_get_input_focus_reply_t> sync(xcb_get_input_focus_reply(m_x11Interface->connection(), cookie, &error));
    if (error) {
        free(error);
    }
}

WaylandOutputOrderWatcher::WaylandOutputOrderWatcher(QObject *parent)
    : OutputOrderWatcher(parent)
{
    // Asking for primaryOutputName() before this happened, will return qGuiApp->primaryScreen()->name() anyways, so set it so the outputOrderChanged will
    // have parameters that are coherent
    OutputOrderWatcher::refresh();

    auto outputListManagement = new WaylandOutputOrder(this);
    m_orderProtocolPresent = outputListManagement->isActive();
    if (!m_orderProtocolPresent) {
        useFallback(true, "kde_output_order_v1 protocol is not available");
        return;
    }
    connect(outputListManagement, &WaylandOutputOrder::outputOrderChanged, this, [this](const QStringList &order) {
        m_pendingOutputOrder = order;

        if (hasAllScreens()) {
            if (m_pendingOutputOrder != m_outputOrder) {
                m_outputOrder = m_pendingOutputOrder;
                Q_EMIT outputOrderChanged(m_outputOrder);
            }
        }
        // otherwise wait for next QGuiApp screenAdded/removal
        // to keep things in sync
    });
}

bool WaylandOutputOrderWatcher::hasAllScreens() const
{
    // for each name in our ordered list, find a screen with that name
    for (const auto &name : std::as_const(m_pendingOutputOrder)) {
        bool present = false;
        for (auto *s : qApp->screens()) {
            if (s->name() == name) {
                present = true;
                break;
            }
        }
        if (!present) {
            return false;
        }
    }
    return true;
}

void WaylandOutputOrderWatcher::refresh()
{
    if (!m_orderProtocolPresent) {
        OutputOrderWatcher::refresh();
        return;
    }

    if (!hasAllScreens()) {
        return;
    }

    if (m_outputOrder != m_pendingOutputOrder) {
        m_outputOrder = m_pendingOutputOrder;
        Q_EMIT outputOrderChanged(m_outputOrder);
    }
}

#include "outputorderwatcher.moc"

#include "moc_outputorderwatcher.cpp"
