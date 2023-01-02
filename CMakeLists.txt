cmake_minimum_required (VERSION 3.16 FATAL_ERROR)

# KDE Application Version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "22")
set (RELEASE_SERVICE_VERSION_MINOR "12")
set (RELEASE_SERVICE_VERSION_MICRO "1")
set (RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(yakuake VERSION ${RELEASE_SERVICE_VERSION})

set (QT_MIN_VERSION "5.15.2")
set (KF5_MIN_VERSION "5.90")
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMInstallIcons)
include(ECMAddAppIcon)
include(FeatureSummary)
include(KDEClangFormat)
include(KDEGitCommitHooks)
include(ECMGenerateDBusServiceFile)

add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x050f00)
add_definitions(-DKF_DISABLE_DEPRECATED_BEFORE_AND_AT=0x055900)

### Security concerns about sendText and runCommand dbus methods being public
option(REMOVE_SENDTEXT_RUNCOMMAND_DBUS_METHODS "yakuake: remove runCommand dbus methods" OFF)

find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} CONFIG REQUIRED Core Widgets Svg)

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED
    Archive
    Config
    CoreAddons
    Crash
    DBusAddons
    GlobalAccel
    I18n
    IconThemes
    KIO
    NewStuff
    Notifications
    NotifyConfig
    Parts
    WidgetsAddons
    WindowSystem
)

if(NOT APPLE)
    option(WITHOUT_X11 "Build without X11 integration (skips finding X11)" OFF)
    if(NOT WITHOUT_X11)
        find_package(X11)
        set(HAVE_X11 ${X11_FOUND})
        if(X11_FOUND)
            if (QT_MAJOR_VERSION EQUAL "5")
                find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED X11Extras)
            else()
                find_package(Qt6 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Gui)
            endif()
        endif()
    endif()

    find_package(KF5Wayland ${KF5_MIN_VERSION} CONFIG)
    set(HAVE_KWAYLAND ${KF5Wayland_FOUND})
endif()

add_subdirectory(app)
add_subdirectory(data)

ki18n_install(po)

# add clang-format target for all our real source files
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)


feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
