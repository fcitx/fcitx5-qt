include_directories(${Qt5Gui_PRIVATE_INCLUDE_DIRS})


set(plugin_SRCS
    qfcitxplatforminputcontext.cpp
    qtkey.cpp
    main.cpp
)

set(plugin_MOC_HDRS
    qfcitxplatforminputcontext.h
    main.h
)

add_library(fcitx5platforminputcontextplugin ${PLUGIN_LIBRARY_TYPE} ${plugin_SRCS})
set_target_properties(fcitx5platforminputcontextplugin PROPERTIES
                         AUTOMOC TRUE
                         COMPILE_FLAGS "-fvisibility=hidden"
                        )
if (BUILD_ONLY_PLUGIN AND BUILD_STATIC_PLUGIN)
target_compile_definitions(fcitx5platforminputcontextplugin PRIVATE "-DQT_STATICPLUGIN")
endif()

target_link_libraries(fcitx5platforminputcontextplugin
                          Qt5::Core
                          Qt5::Gui
                          Qt5::DBus
                          Qt5::X11Extras
                          XCB::XCB
                          Fcitx5Qt5::DBusAddons
                          XKBCommon::XKBCommon
                         )

include(ECMQueryQmake)
query_qmake(_QT5PLUGINDIR QT_INSTALL_PLUGINS)
set(CMAKE_INSTALL_QT5PLUGINDIR ${_QT5PLUGINDIR} CACHE PATH "Qt5 plugin dir")
install(TARGETS fcitx5platforminputcontextplugin DESTINATION ${CMAKE_INSTALL_QT5PLUGINDIR}/platforminputcontexts)
