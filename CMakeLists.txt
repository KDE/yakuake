project(yakuake)

set(KDE_MIN_VERSION "4.4.0")

find_package(KDE4 ${KDE_MIN_VERSION} REQUIRED)

include(KDE4Defaults)

add_subdirectory( app )
add_subdirectory( skins )
