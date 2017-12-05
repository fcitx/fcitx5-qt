set(QT_WRAPPER_SRCS
  main.cpp
  wrapperapp.cpp
  mainwindow.cpp
  )

qt5_wrap_ui(QT_WRAPPER_SRCS mainwindow.ui)

add_executable(fcitx5-qt5-gui-wrapper ${QT_WRAPPER_SRCS})

set_target_properties(fcitx5-qt5-gui-wrapper
  PROPERTIES AUTOMOC TRUE)

target_link_libraries(fcitx5-qt5-gui-wrapper
      Fcitx5::Utils
      Qt5::Core
      Qt5::Gui
      Qt5::Widgets
      Fcitx5Qt5::DBusAddons
      Fcitx5Qt5::WidgetsAddons
)

target_include_directories(fcitx5-qt5-gui-wrapper PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

install(TARGETS fcitx5-qt5-gui-wrapper DESTINATION "${CMAKE_INSTALL_LIBDIR}/fcitx5/libexec")
