/*
 * SPDX-FileCopyrightText: 2012~2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

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
