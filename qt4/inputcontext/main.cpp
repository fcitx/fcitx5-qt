//
// Copyright (C) 2012~2017 by CSSlayer
// wengxt@gmail.com
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// 1. Redistributions of source code must retain the above Copyright
//    notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above Copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the authors nor the names of its contributors
//    may be used to endorse or promote products derived from this
//    software without specific prior written permission.
//

#include "main.h"

namespace fcitx {

namespace {
bool isFcitx(const QString &key) {
    return key.toLower() == "fcitx5" || key.toLower() == "fcitx";
}
} // namespace

QStringList QFcitxInputContextPlugin::keys() const {
    return QStringList{"fcitx5", "fcitx"};
}

QInputContext *QFcitxInputContextPlugin::create(const QString &key) {
    if (!isFcitx(key)) {
        return nullptr;
    }

    return static_cast<QInputContext *>(new QFcitxInputContext());
}

QString QFcitxInputContextPlugin::description(const QString &key) {
    if (!isFcitx(key)) {
        return QString("");
    }

    return QString::fromUtf8("Qt immodule plugin for Fcitx 5");
}

QStringList QFcitxInputContextPlugin::languages(const QString &key) {
    QStringList result;
    if (!isFcitx(key)) {
        result << "zh";
        result << "ja";
        result << "ko";
    }

    return result;
}

QString QFcitxInputContextPlugin::displayName(const QString &key) {
    return key;
}
} // namespace fcitx

Q_EXPORT_PLUGIN2(QFcitxInputContextPlugin, fcitx::QFcitxInputContextPlugin)
