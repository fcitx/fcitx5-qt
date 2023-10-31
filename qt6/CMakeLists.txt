
find_package(Qt6 ${REQUIRED_QT6_VERSION} CONFIG REQUIRED Core DBus Widgets)
find_package(Qt6Gui ${REQUIRED_QT6_VERSION} REQUIRED Private)
find_package(Qt6WaylandClient ${REQUIRED_QT6_VERSION} REQUIRED Private)
find_package(Qt6WaylandGlobalPrivate ${REQUIRED_QT6_VERSION} REQUIRED)

add_subdirectory(dbusaddons)
add_subdirectory(platforminputcontext)

if(NOT BUILD_ONLY_PLUGIN)
  add_subdirectory(immodule-probing)
endif()
