find_package(Qt5Concurrent REQUIRED)

set(QUICKPHRASE_EDITOR_SRCS 
  main.cpp
  model.cpp
  editor.cpp
  editordialog.cpp
  batchdialog.cpp
  filelistmodel.cpp
  )

add_library(fcitx-quickphrase-editor5
  MODULE ${QUICKPHRASE_EDITOR_SRCS})
set_target_properties(fcitx-quickphrase-editor5 PROPERTIES
                      AUTOMOC TRUE AUTOUIC TRUE AUTOUIC_OPTIONS "-tr=fcitx::tr2fcitx;--include=fcitxqti18nhelper.h"
)
target_link_libraries(fcitx-quickphrase-editor5
  Fcitx5::Utils
  Qt5::Core
  Qt5::Gui
  Qt5::Concurrent
  Fcitx5Qt5::WidgetsAddons
)

install(TARGETS fcitx-quickphrase-editor5 DESTINATION ${CMAKE_INSTALL_LIBDIR}/fcitx5/qt5)
