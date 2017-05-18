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
#ifndef _DBUSADDONS_FCITXQTDBUSTYPES_H_
#define _DBUSADDONS_FCITXQTDBUSTYPES_H_

#include "fcitx5qt5dbusaddons_export.h"

#include <QDBusArgument>
#include <QList>
#include <QMetaType>

class FCITX5QT5DBUSADDONS_EXPORT FcitxQtFormattedPreedit {
public:
    const QString &string() const;
    qint32 format() const;
    void setString(const QString &str);
    void setFormat(qint32 format);

    static void registerMetaType();

    bool operator==(const FcitxQtFormattedPreedit &preedit) const;

private:
    QString m_string;
    qint32 m_format = 0;
};

typedef QList<FcitxQtFormattedPreedit> FcitxQtFormattedPreeditList;

QDBusArgument &operator<<(QDBusArgument &argument,
                          const FcitxQtFormattedPreedit &im);
const QDBusArgument &operator>>(const QDBusArgument &argument,
                                FcitxQtFormattedPreedit &im);

Q_DECLARE_METATYPE(FcitxQtFormattedPreedit)
Q_DECLARE_METATYPE(FcitxQtFormattedPreeditList)

class FCITX5QT5DBUSADDONS_EXPORT FcitxQtInputContextArgument {
public:
    FcitxQtInputContextArgument() {}
    FcitxQtInputContextArgument(const QString &name, const QString &value)
        : m_name(name), m_value(value) {}

    static void registerMetaType();

    const QString &name() const;
    const QString &value() const;
    void setName(const QString &);
    void setValue(const QString &);

private:
    QString m_name;
    QString m_value;
};

typedef QList<FcitxQtInputContextArgument> FcitxQtInputContextArgumentList;

QDBusArgument &operator<<(QDBusArgument &argument,
                          const FcitxQtInputContextArgument &im);
const QDBusArgument &operator>>(const QDBusArgument &argument,
                                FcitxQtInputContextArgument &im);
Q_DECLARE_METATYPE(FcitxQtInputContextArgument)
Q_DECLARE_METATYPE(FcitxQtInputContextArgumentList)

#endif // _DBUSADDONS_FCITXQTDBUSTYPES_H_
