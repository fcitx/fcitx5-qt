
set(plugin_SRCS
    qfcitxinputcontext.cpp
    qtkey.cpp
    main.cpp
)

add_library(qtim-fcitx5 MODULE ${plugin_SRCS})
set_target_properties(qtim-fcitx5 PROPERTIES
                         AUTOMOC TRUE
                         COMPILE_FLAGS "-fvisibility=hidden"
                         LINK_FLAGS "-Wl,--no-undefined"
                        )
target_include_directories(qtim-fcitx5 PRIVATE ${X11_X11_INCLUDE_PATH})

target_link_libraries(qtim-fcitx5
                          Qt4::QtCore
                          Qt4::QtGui
                          Qt4::QtDBus
                          XCB::XCB
                          Fcitx5::Utils
                          Fcitx5Qt4::DBusAddons
                          XKBCommon::XKBCommon
                          ${X11_LIBRARIES}
                         )

if (NOT CMAKE_INSTALL_QT4PLUGINDIR)
set(CMAKE_INSTALL_QT4PLUGINDIR ${QT_PLUGINS_DIR})
endif()
install(TARGETS qtim-fcitx5 DESTINATION ${CMAKE_INSTALL_QT4PLUGINDIR}/inputmethods)
