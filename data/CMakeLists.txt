add_subdirectory( icons )
add_subdirectory( skins )
add_subdirectory( updaters )

install(PROGRAMS yakuake.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})

macro_ensure_version("4.7.1" ${KDE_VERSION} HAVEKDE4_7_1)

if (HAVEKDE4_7_1)
    install(FILES yakuake.notifyrc DESTINATION ${DATA_INSTALL_DIR}/yakuake)
else (HAVEKDE4_7_1)
    install(FILES yakuake.notifyrc-pre-4.7.1 DESTINATION ${DATA_INSTALL_DIR}/yakuake RENAME yakuake.notifyrc)
endif (HAVEKDE4_7_1)
