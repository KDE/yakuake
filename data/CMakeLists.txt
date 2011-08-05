add_subdirectory( icons )
add_subdirectory( skins )
add_subdirectory( updaters )

install(PROGRAMS yakuake.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES yakuake.notifyrc DESTINATION ${DATA_INSTALL_DIR}/yakuake)
