
set(QT6_REQUIRED_COMPONENTS Core Gui DBus Widgets)

if (ENABLE_QT6_WAYLAND_WORKAROUND)
  list(APPEND QT6_REQUIRED_COMPONENTS WaylandClient WaylandGlobalPrivate)
endif()
find_package(Qt6 ${REQUIRED_QT6_VERSION} CONFIG REQUIRED COMPONENTS ${QT6_REQUIRED_COMPONENTS})

if (NOT TARGET Qt6::GuiPrivate)
    find_package(Qt6GuiPrivate CONFIG REQUIRED)
endif()

if (ENABLE_QT6_WAYLAND_WORKAROUND AND NOT TARGET Qt6::WaylandClientPrivate)
    find_package(Qt6WaylandClientPrivate CONFIG REQUIRED)
endif()


add_subdirectory(dbusaddons)
add_subdirectory(platforminputcontext)

if(NOT BUILD_ONLY_PLUGIN)
  add_subdirectory(guiwrapper)
  add_subdirectory(widgetsaddons)
  add_subdirectory(quickphrase-editor)
  add_subdirectory(immodule-probing)
endif()
