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

#include "fcitxqtwatcher_p.h"
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>
#include <QDir>

namespace fcitx {

FcitxQtWatcher::FcitxQtWatcher(QObject *parent)
    : QObject(parent), d_ptr(new FcitxQtWatcherPrivate(this)) {}

FcitxQtWatcher::FcitxQtWatcher(const QDBusConnection &connection,
                               QObject *parent)
    : FcitxQtWatcher(parent) {
    setConnection(connection);
}

FcitxQtWatcher::~FcitxQtWatcher() { delete d_ptr; }

bool FcitxQtWatcher::availability() const {
    Q_D(const FcitxQtWatcher);
    return d->m_availability;
}

void FcitxQtWatcher::setConnection(const QDBusConnection &connection) {
    Q_D(FcitxQtWatcher);
    return d->m_serviceWatcher.setConnection(connection);
}

QDBusConnection FcitxQtWatcher::connection() const {
    Q_D(const FcitxQtWatcher);
    return d->m_serviceWatcher.connection();
}

void FcitxQtWatcher::setWatchPortal(bool portal) {
    Q_D(FcitxQtWatcher);
    d->m_watchPortal = portal;
}

bool FcitxQtWatcher::watchPortal() const {
    Q_D(const FcitxQtWatcher);
    return d->m_watchPortal;
}

QString FcitxQtWatcher::serviceName() const {
    Q_D(const FcitxQtWatcher);
    if (d->m_mainPresent) {
        return FCITX_MAIN_SERVICE_NAME;
    }
    if (d->m_portalPresent) {
        return FCITX_PORTAL_SERVICE_NAME;
    }
    return QString();
}

void FcitxQtWatcher::setAvailability(bool availability) {
    Q_D(FcitxQtWatcher);
    if (d->m_availability != availability) {
        d->m_availability = availability;
        emit availabilityChanged(d->m_availability);
    }
}

void FcitxQtWatcher::watch() {
    Q_D(FcitxQtWatcher);
    if (d->m_watched) {
        return;
    }

    connect(&d->m_serviceWatcher,
            SIGNAL(serviceOwnerChanged(QString, QString, QString)), this,
            SLOT(imChanged(QString, QString, QString)));
    d->m_serviceWatcher.addWatchedService(FCITX_MAIN_SERVICE_NAME);
    if (d->m_watchPortal) {
        d->m_serviceWatcher.addWatchedService(FCITX_PORTAL_SERVICE_NAME);
    }

    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(
            FCITX_MAIN_SERVICE_NAME)) {
        d->m_mainPresent = true;
    }
    if (d->m_watchPortal &&
        QDBusConnection::sessionBus().interface()->isServiceRegistered(
            FCITX_PORTAL_SERVICE_NAME)) {
        d->m_portalPresent = true;
    }

    updateAvailability();

    d->m_watched = true;
}

void FcitxQtWatcher::unwatch() {
    Q_D(FcitxQtWatcher);
    if (!d->m_watched) {
        return;
    }
    disconnect(&d->m_serviceWatcher,
               SIGNAL(serviceOwnerChanged(QString, QString, QString)), this,
               SLOT(imChanged(QString, QString, QString)));
    d->m_mainPresent = false;
    d->m_portalPresent = false;
    d->m_watched = false;
    updateAvailability();
}

bool FcitxQtWatcher::isWatching() const {
    Q_D(const FcitxQtWatcher);
    return d->m_watched;
}

void FcitxQtWatcher::imChanged(const QString &service, const QString &,
                               const QString &newOwner) {
    Q_D(FcitxQtWatcher);
    if (service == FCITX_MAIN_SERVICE_NAME) {
        if (!newOwner.isEmpty()) {
            d->m_mainPresent = true;
        } else {
            d->m_mainPresent = false;
        }
    } else if (service == FCITX_PORTAL_SERVICE_NAME) {
        if (!newOwner.isEmpty()) {
            d->m_portalPresent = true;
        } else {
            d->m_portalPresent = false;
        }
    }

    updateAvailability();
}

void FcitxQtWatcher::updateAvailability() {
    Q_D(FcitxQtWatcher);
    setAvailability(d->m_mainPresent || d->m_portalPresent);
}
} // namespace fcitx
