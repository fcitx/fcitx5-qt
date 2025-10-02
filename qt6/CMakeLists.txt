
set(QT6_REQUIRED_COMPONENTS Core Gui GuiPrivate DBus Widgets)

if (ENABLE_QT6_WAYLAND_WORKAROUND)
  list(APPEND QT6_REQUIRED_COMPONENTS WaylandClient WaylandClientPrivate WaylandGlobalPrivate)
endif()
find_package(Qt6 ${REQUIRED_QT6_VERSION} CONFIG REQUIRED COMPONENTS ${QT6_REQUIRED_COMPONENTS})

add_subdirectory(dbusaddons)
add_subdirectory(platforminputcontext)

if(NOT BUILD_ONLY_PLUGIN)
  add_subdirectory(guiwrapper)
  add_subdirectory(widgetsaddons)
  add_subdirectory(quickphrase-editor)
  add_subdirectory(immodule-probing)
endif()
