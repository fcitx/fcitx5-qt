add_subdirectory(dbusaddons)
add_subdirectory(inputcontext)

if(NOT BUILD_ONLY_PLUGIN)
  add_subdirectory(immodule-probing)
endif()
