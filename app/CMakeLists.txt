configure_file(config-yakuake.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-yakuake.h)

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
    KF5::Archive
    KF5::ConfigGui
    KF5::CoreAddons
    KF5::Crash
    KF5::DBusAddons
    KF5::GlobalAccel
    KF5::I18n
    KF5::IconThemes
    KF5::KIOCore
    KF5::NewStuff
    KF5::Notifications
    KF5::NotifyConfig
    KF5::Parts
    KF5::WidgetsAddons
    KF5::WindowSystem)

if(X11_FOUND)
    if (TARGET Qt5::X11Extras)
        target_link_libraries(yakuake Qt5::X11Extras)
    elseif (TARGET Qt6::Gui)
        target_link_libraries(yakuake Qt6::GuiPrivate) # qtx11extras_p.h in knotificationrestrictions
    endif()
endif(X11_FOUND)

if(KF5Wayland_FOUND)
  target_link_libraries(yakuake KF5::WaylandClient)
endif()

install(TARGETS yakuake ${INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES yakuake.knsrc DESTINATION ${KDE_INSTALL_KNSRCDIR})
