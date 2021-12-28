cmake_minimum_required(VERSION 3.16)
project(fcitx5-qt VERSION 5.0.9)
set(FCITX5_QT_VERSION ${PROJECT_VERSION})

set(REQUIRED_QT4_VERSION 4.8.0)
set(REQUIRED_QT5_VERSION 5.1.0)
set(REQUIRED_QT6_VERSION 6.0.0)

find_package(ECM 1.4.0 REQUIRED)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

option(ENABLE_QT4 "Enable Qt 4" On)
option(ENABLE_QT5 "Enable Qt 5" On)
option(ENABLE_QT6 "Enable Qt 6" Off)
option(BUILD_ONLY_PLUGIN "Build only plugin" Off)
option(BUILD_STATIC_PLUGIN "Build plugin as static" Off)
option(WITH_FCITX_PLUGIN_NAME "Enable plugin name with fcitx" On)

if (BUILD_ONLY_PLUGIN)
  set(LIBRARY_TYPE STATIC)
  set(LIBRARY_COMPILE_FLAGS "-fPIC")
else()
  set(LIBRARY_TYPE SHARED)
  add_definitions(-DFCITX_GETTEXT_DOMAIN=\"fcitx5-qt\")
endif()

if (BUILD_ONLY_PLUGIN AND BUILD_STATIC_PLUGIN)
  set(PLUGIN_LIBRARY_TYPE STATIC)
else()
  set(PLUGIN_LIBRARY_TYPE MODULE)
endif()

add_definitions(-DFCITX5_QT_VERSION=\"${FCITX5_QT_VERSION}\" -DQT_NO_KEYWORDS)

include(GNUInstallDirs)
include(FeatureSummary)
include(GenerateExportHeader)
include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(ECMUninstallTarget)

find_package(XCB REQUIRED COMPONENTS XCB)
find_package(XKBCommon 0.5.0 REQUIRED COMPONENTS XKBCommon)
if (NOT BUILD_ONLY_PLUGIN)
find_package(Fcitx5Utils REQUIRED)
endif()
find_package(X11 REQUIRED)

add_library(X11Import UNKNOWN IMPORTED)
set_target_properties(X11Import PROPERTIES
    IMPORTED_LOCATION "${X11_X11_LIB}"
    INTERFACE_INCLUDE_DIRECTORIES "${X11_X11_INCLUDE_PATH}")

if (BUILD_ONLY_PLUGIN)
# Dup a little bit option here.
include(CompilerSettings)
else()
include("${FCITX_INSTALL_CMAKECONFIG_DIR}/Fcitx5Utils/Fcitx5CompilerSettings.cmake")
endif()

set(Fcitx5Qt6_INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR}/Fcitx5Qt6)
set(Fcitx5Qt5_INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR}/Fcitx5Qt5)
set(Fcitx5Qt4_INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR}/Fcitx5Qt4)

if(ENABLE_QT4)
  find_package(Qt4 ${REQUIRED_QT4_VERSION} REQUIRED)
  add_subdirectory(qt4)
endif()

if(ENABLE_QT5)
  find_package(Qt5 ${REQUIRED_QT5_VERSION} CONFIG REQUIRED Core DBus Widgets)
  find_package(Qt5Gui ${REQUIRED_QT5_VERSION} REQUIRED Private)
  add_subdirectory(qt5)
endif()

if(ENABLE_QT6)
  find_package(Qt6 ${REQUIRED_QT6_VERSION} CONFIG REQUIRED Core DBus)
  find_package(Qt6Gui ${REQUIRED_QT6_VERSION} REQUIRED Private)
  add_subdirectory(qt6)
endif()

if(NOT BUILD_ONLY_PLUGIN)
find_package(Gettext REQUIRED)
add_subdirectory(po)
endif()

enable_testing()
add_subdirectory(test)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
