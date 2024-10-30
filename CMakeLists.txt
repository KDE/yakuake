cmake_minimum_required (VERSION 3.16 FATAL_ERROR)

# KDE Application Version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "24")
set (RELEASE_SERVICE_VERSION_MINOR "08")
set (RELEASE_SERVICE_VERSION_MICRO "3")
set (RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(yakuake VERSION ${RELEASE_SERVICE_VERSION})

# KF6 only
set(QT_MIN_VERSION "6.5.0")
set(KF_MIN_VERSION "5.240.0")

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMInstallIcons)
include(ECMAddAppIcon)
include(FeatureSummary)
include(KDEClangFormat)
include(KDEGitCommitHooks)
include(ECMDeprecationSettings)
include(ECMGenerateDBusServiceFile)

ecm_set_disabled_deprecation_versions(QT 5.15.2
    KF 5.100.0
    )

### Security concerns about sendText and runCommand dbus methods being public
option(REMOVE_SENDTEXT_RUNCOMMAND_DBUS_METHODS "yakuake: remove runCommand dbus methods" OFF)

find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED Core Widgets Svg)

find_package(KF6 ${KF_MIN_VERSION} REQUIRED
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
    StatusNotifierItem
)

option(WITH_X11 "Build with X11 integration" ON)
if(WITH_X11)
    set(HAVE_X11 TRUE)
    find_package(Qt6 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Gui)
else()
    set(HAVE_X11 FALSE)
endif()

find_package(KWayland CONFIG)
set(HAVE_KWAYLAND ${KWayland_FOUND})

add_subdirectory(app)
add_subdirectory(data)

ki18n_install(po)

# add clang-format target for all our real source files
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)


feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
