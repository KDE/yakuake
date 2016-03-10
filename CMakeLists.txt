project(yakuake)

cmake_minimum_required (VERSION 2.8.12 FATAL_ERROR)
set (QT_MIN_VERSION "5.2.0")

find_package(ECM 1.2.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMInstallIcons)
include(ECMAddAppIcon)
include(FeatureSummary)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED Core Widgets)

find_package(KF5 5.15 REQUIRED
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

add_subdirectory(app)
add_subdirectory(data)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
