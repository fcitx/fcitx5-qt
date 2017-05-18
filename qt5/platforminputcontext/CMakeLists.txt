include_directories(${Qt5Gui_PRIVATE_INCLUDE_DIRS})


set(plugin_SRCS
    qfcitxplatforminputcontext.cpp
    main.cpp
)

set(plugin_MOC_HDRS
    qfcitxplatforminputcontext.h
    main.h
)

add_library(fcitx5platforminputcontextplugin MODULE ${plugin_SRCS})
set_target_properties(fcitx5platforminputcontextplugin PROPERTIES
                         AUTOMOC TRUE
                         COMPILE_FLAGS "-fvisibility=hidden"
                         LINK_FLAGS "-Wl,--no-undefined"
                        )

target_link_libraries(fcitx5platforminputcontextplugin
                          Qt5::Core
                          Qt5::Gui
                          Qt5::DBus
                          Qt5::X11Extras
                          XCB::XCB
                          Fcitx5::Utils
                          Fcitx5Qt5::DBusAddons
                          XKBCommon::XKBCommon
                         )

include(ECMQueryQmake)
if (NOT CMAKE_INSTALL_QT5PLUGINDIR)
query_qmake(CMAKE_INSTALL_QT5PLUGINDIR QT_INSTALL_PLUGINS)
endif()
install(TARGETS fcitx5platforminputcontextplugin DESTINATION ${CMAKE_INSTALL_QT5PLUGINDIR}/platforminputcontexts)
