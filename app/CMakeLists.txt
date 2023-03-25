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
    KF${KF_MAJOR_VERSION}::Archive
    KF${KF_MAJOR_VERSION}::ConfigGui
    KF${KF_MAJOR_VERSION}::CoreAddons
    KF${KF_MAJOR_VERSION}::Crash
    KF${KF_MAJOR_VERSION}::DBusAddons
    KF${KF_MAJOR_VERSION}::GlobalAccel
    KF${KF_MAJOR_VERSION}::I18n
    KF${KF_MAJOR_VERSION}::IconThemes
    KF${KF_MAJOR_VERSION}::KIOCore
    KF${KF_MAJOR_VERSION}::NewStuffWidgets
    KF${KF_MAJOR_VERSION}::Notifications
    KF${KF_MAJOR_VERSION}::NotifyConfig
    KF${KF_MAJOR_VERSION}::Parts
    KF${KF_MAJOR_VERSION}::WidgetsAddons
    KF${KF_MAJOR_VERSION}::WindowSystem)

if(HAVE_X11)
    if (TARGET Qt5::X11Extras)
        target_link_libraries(yakuake Qt5::X11Extras)
    elseif (TARGET Qt6::Gui)
        target_link_libraries(yakuake Qt6::GuiPrivate) # qtx11extras_p.h in knotificationrestrictions
    endif()
endif()

if(KF${KF_MAJOR_VERSION}Wayland_FOUND)
  target_link_libraries(yakuake KF${KF_MAJOR_VERSION}::WaylandClient)
endif()

install(TARGETS yakuake ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES yakuake.knsrc DESTINATION ${KDE_INSTALL_KNSRCDIR})
