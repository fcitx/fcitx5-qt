add_executable(fcitx5-qt5-immodule-probing main.cpp)
target_include_directories(fcitx5-qt5-immodule-probing PRIVATE ${Qt5Gui_PRIVATE_INCLUDE_DIRS})
target_link_libraries(fcitx5-qt5-immodule-probing Qt5::Gui)
