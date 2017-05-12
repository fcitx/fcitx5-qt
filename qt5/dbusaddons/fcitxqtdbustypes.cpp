/*
 * Copyright (C) 2017~2017 by CSSlayer
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

#include <QDBusMetaType>

#include "fcitxqtdbustypes.h"

void FcitxQtFormattedPreedit::registerMetaType() {
    qRegisterMetaType<FcitxQtFormattedPreedit>("FcitxQtFormattedPreedit");
    qDBusRegisterMetaType<FcitxQtFormattedPreedit>();
    qRegisterMetaType<FcitxQtFormattedPreeditList>(
        "FcitxQtFormattedPreeditList");
    qDBusRegisterMetaType<FcitxQtFormattedPreeditList>();
}

qint32 FcitxQtFormattedPreedit::format() const { return m_format; }

const QString &FcitxQtFormattedPreedit::string() const { return m_string; }

void FcitxQtFormattedPreedit::setFormat(qint32 format) { m_format = format; }

void FcitxQtFormattedPreedit::setString(const QString &str) { m_string = str; }

bool FcitxQtFormattedPreedit::
operator==(const FcitxQtFormattedPreedit &preedit) const {
    return (preedit.m_format == m_format) && (preedit.m_string == m_string);
}

FCITX5QT5DBUSADDONS_EXPORT
QDBusArgument &operator<<(QDBusArgument &argument,
                          const FcitxQtFormattedPreedit &preedit) {
    argument.beginStructure();
    argument << preedit.string();
    argument << preedit.format();
    argument.endStructure();
    return argument;
}

FCITX5QT5DBUSADDONS_EXPORT
const QDBusArgument &operator>>(const QDBusArgument &argument,
                                FcitxQtFormattedPreedit &preedit) {
    QString str;
    qint32 format;
    argument.beginStructure();
    argument >> str >> format;
    argument.endStructure();
    preedit.setString(str);
    preedit.setFormat(format);
    return argument;
}

void FcitxQtInputContextArgument::registerMetaType() {
    qRegisterMetaType<FcitxQtInputContextArgument>(
        "FcitxQtInputContextArgument");
    qDBusRegisterMetaType<FcitxQtInputContextArgument>();
    qRegisterMetaType<FcitxQtInputContextArgumentList>(
        "FcitxQtInputContextArgumentList");
    qDBusRegisterMetaType<FcitxQtInputContextArgumentList>();
}

const QString &FcitxQtInputContextArgument::name() const { return m_name; }

void FcitxQtInputContextArgument::setName(const QString &name) {
    m_name = name;
}

const QString &FcitxQtInputContextArgument::value() const { return m_value; }

void FcitxQtInputContextArgument::setValue(const QString &value) {
    m_value = value;
}

FCITX5QT5DBUSADDONS_EXPORT
QDBusArgument &operator<<(QDBusArgument &argument,
                          const FcitxQtInputContextArgument &arg) {
    argument.beginStructure();
    argument << arg.name();
    argument << arg.value();
    argument.endStructure();
    return argument;
}

FCITX5QT5DBUSADDONS_EXPORT
const QDBusArgument &operator>>(const QDBusArgument &argument,
                                FcitxQtInputContextArgument &arg) {
    QString name, value;
    argument.beginStructure();
    argument >> name >> value;
    argument.endStructure();
    arg.setName(name);
    arg.setValue(value);
    return argument;
}
