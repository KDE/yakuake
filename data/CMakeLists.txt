add_subdirectory(icons)
add_subdirectory(skins)

ecm_generate_dbus_service_file(
    NAME org.kde.yakuake
    EXECUTABLE ${KDE_INSTALL_FULL_BINDIR}/yakuake
    DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR}
)

install(FILES org.kde.yakuake.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.kde.yakuake.appdata.xml DESTINATION  ${KDE_INSTALL_METAINFODIR})
install(FILES yakuake.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR})
