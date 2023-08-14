/*
 * This file was generated by qdbusxml2cpp version 0.8
 * Command line was: qdbusxml2cpp -N -p fcitx4inputmethodproxy -c
 * Fcitx4InputMethodProxy org.fcitx.Fcitx.InputMethod.xml
 * org.fcitx.Fcitx.InputMethod
 *
 * qdbusxml2cpp is Copyright (C) 2022 The Qt Company Ltd.
 *
 * This is an auto-generated file.
 * Do not edit! All changes made to it will be lost.
 */

#ifndef FCITX4INPUTMETHODPROXY_H
#define FCITX4INPUTMETHODPROXY_H

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>

namespace fcitx {

/*
 * Proxy class for interface org.fcitx.Fcitx.InputMethod
 */
class Fcitx4InputMethodProxy : public QDBusAbstractInterface {
    Q_OBJECT
public:
    static inline const char *staticInterfaceName() {
        return "org.fcitx.Fcitx.InputMethod";
    }

public:
    Fcitx4InputMethodProxy(const QString &service, const QString &path,
                           const QDBusConnection &connection,
                           QObject *parent = nullptr);

    ~Fcitx4InputMethodProxy();

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<int, bool, unsigned int, unsigned int,
                             unsigned int, unsigned int>
    CreateICv3(const QString &appname, int pid) {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(appname)
                     << QVariant::fromValue(pid);
        return asyncCallWithArgumentList(QStringLiteral("CreateICv3"),
                                         argumentList);
    }
    inline QDBusReply<int> CreateICv3(const QString &appname, int pid,
                                      bool &enable, unsigned int &keyval1,
                                      unsigned int &state1,
                                      unsigned int &keyval2,
                                      unsigned int &state2) {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(appname)
                     << QVariant::fromValue(pid);
        QDBusMessage reply = callWithArgumentList(
            QDBus::Block, QStringLiteral("CreateICv3"), argumentList);
        if (reply.type() == QDBusMessage::ReplyMessage &&
            reply.arguments().count() == 6) {
            enable = qdbus_cast<bool>(reply.arguments().at(1));
            keyval1 = qdbus_cast<unsigned int>(reply.arguments().at(2));
            state1 = qdbus_cast<unsigned int>(reply.arguments().at(3));
            keyval2 = qdbus_cast<unsigned int>(reply.arguments().at(4));
            state2 = qdbus_cast<unsigned int>(reply.arguments().at(5));
        }
        return reply;
    }

Q_SIGNALS: // SIGNALS
};

} // namespace fcitx

#endif