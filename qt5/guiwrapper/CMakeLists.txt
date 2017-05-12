set(QT_WRAPPER_SRCS
  main.cpp
  wrapperapp.cpp
  mainwindow.cpp
  )

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  )

link_directories(${FCITX4_FCITX_UTILS_LIBRARY_DIRS})

qt5_wrap_ui(QT_WRAPPER_SRCS mainwindow.ui)

add_executable(fcitx-qt5-gui-wrapper ${QT_WRAPPER_SRCS})

set_target_properties(fcitx-qt5-gui-wrapper
  PROPERTIES AUTOMOC TRUE)

target_link_libraries(fcitx-qt5-gui-wrapper
      Qt5::Core
      Qt5::Gui
      Qt5::Widgets
      Fcitx5Qt5::DBusAddons
      Fcitx5Qt5::WidgetsAddons
      LibIntl::LibIntl
      Fcitx5::Utils
)

install(TARGETS fcitx-qt5-gui-wrapper DESTINATION "${CMAKE_INSTALL_LIBDIR}/fcitx/libexec")
