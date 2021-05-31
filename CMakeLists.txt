project(yakuake)

# KDE Application Version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "21")
set (RELEASE_SERVICE_VERSION_MINOR "07")
set (RELEASE_SERVICE_VERSION_MICRO "70")
set (RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

cmake_minimum_required (VERSION 3.16 FATAL_ERROR)
set (QT_MIN_VERSION "5.15.0")
set (KF5_MIN_VERSION "5.82")

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

add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x050f00)
add_definitions(-DKF_DISABLE_DEPRECATED_BEFORE_AND_AT=0x055200)

### Security concerns about sendText and runCommand dbus methods being public
option(REMOVE_SENDTEXT_RUNCOMMAND_DBUS_METHODS "yakuake: remove runCommand dbus methods" OFF)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED Core Widgets Svg)

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
  find_package(X11)
  set(HAVE_X11 ${X11_FOUND})

  if(X11_FOUND)
    find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED X11Extras)
  endif(X11_FOUND)

  find_package(KF5Wayland 5.5 CONFIG)
  set(HAVE_KWAYLAND ${KF5Wayland_FOUND})
endif(NOT APPLE)

add_definitions(
  -DQT_USE_QSTRINGBUILDER
  -DQT_NO_CAST_TO_ASCII
  -DQT_NO_CAST_FROM_ASCII
  -DQT_STRICT_ITERATORS
  -DQT_NO_URL_CAST_FROM_STRING
  -DQT_NO_CAST_FROM_BYTEARRAY
  -DQT_NO_SIGNALS_SLOTS_KEYWORDS
  -DQT_USE_FAST_OPERATOR_PLUS
)

configure_file(org.kde.yakuake.service.in
	       ${CMAKE_CURRENT_BINARY_DIR}/org.kde.yakuake.service)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.yakuake.service
	DESTINATION ${KDE_INSTALL_DBUSSERVICEDIR})

add_subdirectory(app)
add_subdirectory(data)

# add clang-format target for all our real source files
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)


feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
