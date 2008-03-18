add_subdirectory( icons )

include_directories( ${KDE4_INCLUDES} )
 
set(yakuake_SRCS
    main.cpp
    application.cpp
    mainwindow.cpp
    skin.cpp
    titlebar.cpp
    tabbar.cpp
    sessionstack.cpp
    session.cpp
    terminal.cpp
    splitter.cpp
    firstrundialog.cpp
    config/windowsettings.cpp
    config/appearancesettings.cpp
    config/skinlistdelegate.cpp
)
 
kde4_add_kcfg_files(yakuake_SRCS config/settings.kcfgc)

kde4_add_ui_files(yakuake_SRCS 
    firstrundialog.ui
    config/windowsettings.ui
    config/behaviorsettings.ui
    config/appearancesettings.ui
)

kde4_add_executable(yakuake ${yakuake_SRCS})

target_link_libraries(yakuake ${KDE4_KPARTS_LIBS})

install(TARGETS yakuake DESTINATION ${BIN_INSTALL_DIR})

install(FILES yakuake.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
