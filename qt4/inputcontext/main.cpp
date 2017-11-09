/*
 * Copyright (C) 2012~2017 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */

#include "main.h"

namespace fcitx {

namespace {
bool isFcitx(const QString &key) {
    return key.toLower() == "fcitx5" || key.toLower() == "fcitx";
}
}

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
}

Q_EXPORT_PLUGIN2(QFcitxInputContextPlugin, fcitx::QFcitxInputContextPlugin)
