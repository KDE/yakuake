add_subdirectory(icons)
add_subdirectory(skins)

install(FILES org.kde.yakuake.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES org.kde.yakuake.appdata.xml DESTINATION  ${CMAKE_INSTALL_METAINFODIR})
install(FILES yakuake.notifyrc DESTINATION ${KNOTIFYRC_INSTALL_DIR})
