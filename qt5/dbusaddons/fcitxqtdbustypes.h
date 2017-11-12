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

#ifndef _DBUSADDONS_FCITXQTDBUSTYPES_H_
#define _DBUSADDONS_FCITXQTDBUSTYPES_H_

#include "fcitx5qt5dbusaddons_export.h"

#include <QDBusArgument>
#include <QList>
#include <QMetaType>
#include <type_traits>

namespace fcitx {

FCITX5QT5DBUSADDONS_EXPORT void registerFcitxQtDBusTypes();

#define FCITX5_QT_BEGIN_DECLARE_DBUS_TYPE(TYPE)                                \
    class FCITX5QT5DBUSADDONS_EXPORT TYPE {                                    \
    public:

#define FCITX5_QT_DECLARE_FIELD(TYPE, GETTER, SETTER)                          \
public:                                                                        \
    std::conditional_t<std::is_class<TYPE>::value, const TYPE &, TYPE>         \
    GETTER() const {                                                           \
        return m_##GETTER;                                                     \
    }                                                                          \
    void SETTER(                                                               \
        std::conditional_t<std::is_class<TYPE>::value, const TYPE &, TYPE>     \
            value) {                                                           \
        m_##GETTER = value;                                                    \
    }                                                                          \
                                                                               \
private:                                                                       \
    TYPE m_##GETTER = TYPE();

#define FCITX5_QT_END_DECLARE_DBUS_TYPE(TYPE)                                  \
    }                                                                          \
    ;                                                                          \
    typedef QList<TYPE> TYPE##List;                                            \
    FCITX5QT5DBUSADDONS_EXPORT QDBusArgument &operator<<(                      \
        QDBusArgument &argument, const TYPE &value);                           \
    FCITX5QT5DBUSADDONS_EXPORT const QDBusArgument &operator>>(                \
        const QDBusArgument &argument, TYPE &value);

FCITX5_QT_BEGIN_DECLARE_DBUS_TYPE(FcitxQtFormattedPreedit);
FCITX5_QT_DECLARE_FIELD(QString, string, setString);
FCITX5_QT_DECLARE_FIELD(qint32, format, setFormat);

public:
bool operator==(const FcitxQtFormattedPreedit &preedit) const;
FCITX5_QT_END_DECLARE_DBUS_TYPE(FcitxQtFormattedPreedit);

FCITX5_QT_BEGIN_DECLARE_DBUS_TYPE(FcitxQtStringKeyValue);
FCITX5_QT_DECLARE_FIELD(QString, key, setKey);
FCITX5_QT_DECLARE_FIELD(QString, value, setValue);
FCITX5_QT_END_DECLARE_DBUS_TYPE(FcitxQtStringKeyValue);

FCITX5_QT_BEGIN_DECLARE_DBUS_TYPE(FcitxQtInputMethodEntry);
FCITX5_QT_DECLARE_FIELD(QString, uniqueName, setUniqueName);
FCITX5_QT_DECLARE_FIELD(QString, name, setName);
FCITX5_QT_DECLARE_FIELD(QString, nativeName, setNativeName);
FCITX5_QT_DECLARE_FIELD(QString, icon, setIcon);
FCITX5_QT_DECLARE_FIELD(QString, label, setLabel);
FCITX5_QT_DECLARE_FIELD(QString, languageCode, setLanguageCode);
FCITX5_QT_END_DECLARE_DBUS_TYPE(FcitxQtInputMethodEntry);

FCITX5_QT_BEGIN_DECLARE_DBUS_TYPE(FcitxQtVariantInfo);
FCITX5_QT_DECLARE_FIELD(QString, variant, setVariant);
FCITX5_QT_DECLARE_FIELD(QString, description, setDescription);
FCITX5_QT_DECLARE_FIELD(QStringList, languages, setLanguages);
FCITX5_QT_END_DECLARE_DBUS_TYPE(FcitxQtVariantInfo);

FCITX5_QT_BEGIN_DECLARE_DBUS_TYPE(FcitxQtLayoutInfo);
FCITX5_QT_DECLARE_FIELD(QString, layout, setLayout);
FCITX5_QT_DECLARE_FIELD(QString, description, setDescription);
FCITX5_QT_DECLARE_FIELD(QStringList, languages, setLanguages);
FCITX5_QT_DECLARE_FIELD(FcitxQtVariantInfoList, variants, setVariants);
FCITX5_QT_END_DECLARE_DBUS_TYPE(FcitxQtLayoutInfo);
}

Q_DECLARE_METATYPE(fcitx::FcitxQtFormattedPreedit)
Q_DECLARE_METATYPE(fcitx::FcitxQtFormattedPreeditList)

Q_DECLARE_METATYPE(fcitx::FcitxQtStringKeyValue)
Q_DECLARE_METATYPE(fcitx::FcitxQtStringKeyValueList)

Q_DECLARE_METATYPE(fcitx::FcitxQtInputMethodEntry)
Q_DECLARE_METATYPE(fcitx::FcitxQtInputMethodEntryList)

Q_DECLARE_METATYPE(fcitx::FcitxQtVariantInfo)
Q_DECLARE_METATYPE(fcitx::FcitxQtVariantInfoList)

Q_DECLARE_METATYPE(fcitx::FcitxQtLayoutInfo)
Q_DECLARE_METATYPE(fcitx::FcitxQtLayoutInfoList)

#endif // _DBUSADDONS_FCITXQTDBUSTYPES_H_
