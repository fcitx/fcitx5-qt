
set(plugin_SRCS
    qfcitxinputcontext.cpp
    qtkey.cpp
    main.cpp
)

add_library(qtim-fcitx5 MODULE ${plugin_SRCS})
set_target_properties(qtim-fcitx5 PROPERTIES
                         AUTOMOC TRUE
                         COMPILE_FLAGS "-fvisibility=hidden"
                        )

target_link_libraries(qtim-fcitx5
                          Fcitx5::Utils
                          Qt4::QtCore
                          Qt4::QtGui
                          Qt4::QtDBus
                          XCB::XCB
                          Fcitx5Qt4::DBusAddons
                          XKBCommon::XKBCommon
                          X11Import
                         )

if (NOT CMAKE_INSTALL_QT4PLUGINDIR)
set(CMAKE_INSTALL_QT4PLUGINDIR ${QT_PLUGINS_DIR})
endif()
install(TARGETS qtim-fcitx5 DESTINATION ${CMAKE_INSTALL_QT4PLUGINDIR}/inputmethods)
