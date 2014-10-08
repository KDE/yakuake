add_subdirectory(icons)
add_subdirectory(skins)

install(PROGRAMS org.kde.yakuake.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES yakuake.notifyrc DESTINATION ${KNOTIFYRC_INSTALL_DIR})
