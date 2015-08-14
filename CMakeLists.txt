cmake_minimum_required(VERSION 2.8.9)
project(yakuake)

set(KDE_MIN_VERSION "4.7.1")

find_package(KDE4 ${KDE_MIN_VERSION} REQUIRED)

include(KDE4Defaults)

add_subdirectory( app )
add_subdirectory( data )
