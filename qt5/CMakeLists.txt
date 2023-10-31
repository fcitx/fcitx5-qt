find_package(Qt5 ${REQUIRED_QT5_VERSION} CONFIG REQUIRED Core DBus Widgets)
find_package(Qt5Gui ${REQUIRED_QT5_VERSION} REQUIRED Private)

add_subdirectory(dbusaddons)
add_subdirectory(platforminputcontext)

if(NOT BUILD_ONLY_PLUGIN)
  add_subdirectory(guiwrapper)
  add_subdirectory(widgetsaddons)
  add_subdirectory(quickphrase-editor)
  add_subdirectory(immodule-probing)
endif()
