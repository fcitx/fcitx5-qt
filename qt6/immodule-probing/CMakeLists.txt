add_executable(fcitx5-qt6-immodule-probing main.cpp)
target_link_libraries(fcitx5-qt6-immodule-probing Qt6::Gui Qt6::GuiPrivate)

install(TARGETS fcitx5-qt6-immodule-probing DESTINATION "${CMAKE_INSTALL_BINDIR}")
