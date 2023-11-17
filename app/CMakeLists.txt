configure_file(config-yakuake.h.in ${CMAKE_CURRENT_BINARY_DIR}/config-yakuake.h)

add_executable(yakuake)

target_sources(yakuake PRIVATE
    config/appearancesettings.cpp
    config/appearancesettings.h
    config/skinlistdelegate.cpp
    config/skinlistdelegate.h
    config/windowsettings.cpp
    config/windowsettings.h
    firstrundialog.cpp
    firstrundialog.h
    main.cpp
    mainwindow.cpp
    mainwindow.h
    session.cpp
    session.h
    sessionstack.cpp
    sessionstack.h
    skin.cpp
    skin.h
    splitter.cpp
    splitter.h
    tabbar.cpp
    tabbar.h
    terminal.cpp
    terminal.h
    titlebar.cpp
    titlebar.h
    visualeventoverlay.cpp
    visualeventoverlay.h
)

ki18n_wrap_ui(yakuake
    firstrundialog.ui
    config/windowsettings.ui
    config/behaviorsettings.ui
    config/appearancesettings.ui
)

kconfig_add_kcfg_files(yakuake config/settings.kcfgc)

file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/../data/icons/*-apps-yakuake.png")
ecm_add_app_icon(yakuake_SRCS ICONS ${ICONS_SRCS})


target_link_libraries(yakuake
    Qt::Widgets
    KF6::Archive
    KF6::ConfigGui
    KF6::CoreAddons
    KF6::Crash
    KF6::DBusAddons
    KF6::GlobalAccel
    KF6::I18n
    KF6::IconThemes
    KF6::KIOCore
    KF6::NewStuffWidgets
    KF6::Notifications
    KF6::NotifyConfig
    KF6::Parts
    KF6::WidgetsAddons
    KF6::WindowSystem
    KF6::StatusNotifierItem
)

if(HAVE_X11)
    if (TARGET Qt6::Gui)
        target_link_libraries(yakuake Qt6::GuiPrivate) # qtx11extras_p.h
    endif()
endif()

if(KWayland_FOUND)
  target_link_libraries(yakuake Plasma::KWaylandClient)
endif()

install(TARGETS yakuake ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES yakuake.knsrc DESTINATION ${KDE_INSTALL_KNSRCDIR})
