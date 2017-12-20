add_subdirectory(dbusaddons)
add_subdirectory(platforminputcontext)

if(NOT BUILD_ONLY_PLUGIN)
  add_subdirectory(guiwrapper)
  add_subdirectory(widgetsaddons)
  add_subdirectory(quickphrase-editor)
endif()
