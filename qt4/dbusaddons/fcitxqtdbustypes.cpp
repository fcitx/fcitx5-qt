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

#include <QDBusMetaType>

#include "fcitxqtdbustypes.h"

namespace fcitx {

#define FCITX5_QT_DEFINE_DBUS_TYPE(TYPE)                                       \
    qRegisterMetaType<TYPE>(#TYPE);                                            \
    qDBusRegisterMetaType<TYPE>();                                             \
    qRegisterMetaType<TYPE##List>(#TYPE "List");                               \
    qDBusRegisterMetaType<TYPE##List>();

void registerFcitxQtDBusTypes() {
    FCITX5_QT_DEFINE_DBUS_TYPE(FcitxQtFormattedPreedit);
    FCITX5_QT_DEFINE_DBUS_TYPE(FcitxQtStringKeyValue);
    FCITX5_QT_DEFINE_DBUS_TYPE(FcitxQtInputMethodEntry);
    FCITX5_QT_DEFINE_DBUS_TYPE(FcitxQtLayoutInfo);
    FCITX5_QT_DEFINE_DBUS_TYPE(FcitxQtVariantInfo);
}

bool FcitxQtFormattedPreedit::
operator==(const FcitxQtFormattedPreedit &preedit) const {
    return (preedit.m_format == m_format) && (preedit.m_string == m_string);
}

QDBusArgument &operator<<(QDBusArgument &argument,
                          const FcitxQtFormattedPreedit &preedit) {
    argument.beginStructure();
    argument << preedit.string();
    argument << preedit.format();
    argument.endStructure();
    return argument;
}

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

QDBusArgument &operator<<(QDBusArgument &argument,
                          const FcitxQtStringKeyValue &arg) {
    argument.beginStructure();
    argument << arg.key();
    argument << arg.value();
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument,
                                FcitxQtStringKeyValue &arg) {
    QString key, value;
    argument.beginStructure();
    argument >> key >> value;
    argument.endStructure();
    arg.setKey(key);
    arg.setValue(value);
    return argument;
}

QDBusArgument &operator<<(QDBusArgument &argument,
                          const FcitxQtInputMethodEntry &arg) {
    argument.beginStructure();
    argument << arg.uniqueName();
    argument << arg.name();
    argument << arg.nativeName();
    argument << arg.icon();
    argument << arg.label();
    argument << arg.languageCode();
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument,
                                FcitxQtInputMethodEntry &arg) {
    QString uniqueName, name, nativeName, icon, label, languageCode;
    argument.beginStructure();
    argument >> uniqueName >> name >> nativeName >> icon >> label >>
        languageCode;
    argument.endStructure();
    arg.setUniqueName(uniqueName);
    arg.setName(name);
    arg.setNativeName(nativeName);
    arg.setIcon(icon);
    arg.setLabel(label);
    arg.setLanguageCode(languageCode);
    return argument;
}

QDBusArgument &operator<<(QDBusArgument &argument,
                          const FcitxQtVariantInfo &arg) {
    argument.beginStructure();
    argument << arg.variant();
    argument << arg.description();
    argument << arg.languages();
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument,
                                FcitxQtVariantInfo &arg) {
    QString variant, description;
    QStringList languages;
    argument.beginStructure();
    argument >> variant >> description >> languages;
    argument.endStructure();
    arg.setVariant(variant);
    arg.setDescription(description);
    arg.setLanguages(languages);
    return argument;
}

QDBusArgument &operator<<(QDBusArgument &argument,
                          const FcitxQtLayoutInfo &arg) {
    argument.beginStructure();
    argument << arg.layout();
    argument << arg.description();
    argument << arg.languages();
    argument << arg.variants();
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument,
                                FcitxQtLayoutInfo &arg) {
    QString layout, description;
    QStringList languages;
    FcitxQtVariantInfoList variants;
    argument.beginStructure();
    argument >> layout >> description >> languages >> variants;
    argument.endStructure();
    arg.setLayout(layout);
    arg.setDescription(description);
    arg.setLanguages(languages);
    arg.setVariants(variants);
    return argument;
}
}
