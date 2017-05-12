#!/bin/sh

qdbusxml2cpp-qt5 -N -p fcitxqtinputcontextproxy -c FcitxQtInputContextProxy interfaces/org.fcitx.Fcitx.InputContext1.xml -i fcitxqtdbustypes.h -i fcitxqtdbusaddons_export.h
qdbusxml2cpp-qt5 -N -p fcitxqtinputmethodproxy -c FcitxQtInputMethodProxy interfaces/org.fcitx.Fcitx.InputMethod1.xml -i fcitxqtdbustypes.h -i fcitxqtdbusaddons_export.h
