cmake_minimum_required(VERSION 3.6.0)

project(fcitx5-qt VERSION 0.0.1)

set(REQUIRED_QT_VERSION 5.1.0)

find_package(ECM 1.4.0 REQUIRED)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(GNUInstallDirs)
include(FeatureSummary)
include(GenerateExportHeader)
include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(ECMUninstallTarget)
include(FcitxCompilerSettings)

find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Core DBus Widgets X11Extras)
find_package(XCB REQUIRED COMPONENTS XCB)
find_package(Qt5Gui ${REQUIRED_QT_VERSION} REQUIRED Private)
find_package(XKBCommon 0.5.0 REQUIRED COMPONENTS XKBCommon)
find_package(LibIntl REQUIRED)
find_package(Fcitx5Utils REQUIRED)
find_package(Qt4 4.8 REQUIRED)
find_package(X11 REQUIRED)

set(Fcitx5Qt5_INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_FULL_INCLUDEDIR}/Fcitx5Qt5)
set(Fcitx5Qt4_INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_FULL_INCLUDEDIR}/Fcitx5Qt4)
add_subdirectory(qt4)
add_subdirectory(qt5)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
