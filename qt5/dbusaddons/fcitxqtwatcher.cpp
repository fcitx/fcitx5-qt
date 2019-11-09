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
    return d->availability_;
}

void FcitxQtWatcher::setConnection(const QDBusConnection &connection) {
    Q_D(FcitxQtWatcher);
    return d->serviceWatcher_.setConnection(connection);
}

QDBusConnection FcitxQtWatcher::connection() const {
    Q_D(const FcitxQtWatcher);
    return d->serviceWatcher_.connection();
}

void FcitxQtWatcher::setWatchPortal(bool portal) {
    Q_D(FcitxQtWatcher);
    d->watchPortal_ = portal;
}

bool FcitxQtWatcher::watchPortal() const {
    Q_D(const FcitxQtWatcher);
    return d->watchPortal_;
}

QString FcitxQtWatcher::serviceName() const {
    Q_D(const FcitxQtWatcher);
    if (d->mainPresent_) {
        return FCITX_MAIN_SERVICE_NAME;
    }
    if (d->portalPresent_) {
        return FCITX_PORTAL_SERVICE_NAME;
    }
    return QString();
}

void FcitxQtWatcher::setAvailability(bool availability) {
    Q_D(FcitxQtWatcher);
    if (d->availability_ != availability) {
        d->availability_ = availability;
        emit availabilityChanged(d->availability_);
    }
}

void FcitxQtWatcher::watch() {
    Q_D(FcitxQtWatcher);
    if (d->watched_) {
        return;
    }

    connect(&d->serviceWatcher_, &QDBusServiceWatcher::serviceOwnerChanged,
            this, &FcitxQtWatcher::imChanged);
    d->serviceWatcher_.addWatchedService(FCITX_MAIN_SERVICE_NAME);
    if (d->watchPortal_) {
        d->serviceWatcher_.addWatchedService(FCITX_PORTAL_SERVICE_NAME);
    }

    if (connection().interface()->isServiceRegistered(
            FCITX_MAIN_SERVICE_NAME)) {
        d->mainPresent_ = true;
    }
    if (d->watchPortal_ && connection().interface()->isServiceRegistered(
                               FCITX_PORTAL_SERVICE_NAME)) {
        d->portalPresent_ = true;
    }

    updateAvailability();

    d->watched_ = true;
}

void FcitxQtWatcher::unwatch() {
    Q_D(FcitxQtWatcher);
    if (!d->watched_) {
        return;
    }
    disconnect(&d->serviceWatcher_, &QDBusServiceWatcher::serviceOwnerChanged,
               this, &FcitxQtWatcher::imChanged);
    d->mainPresent_ = false;
    d->portalPresent_ = false;
    d->watched_ = false;
    updateAvailability();
}

bool FcitxQtWatcher::isWatching() const {
    Q_D(const FcitxQtWatcher);
    return d->watched_;
}

void FcitxQtWatcher::imChanged(const QString &service, const QString &,
                               const QString &newOwner) {
    Q_D(FcitxQtWatcher);
    if (service == FCITX_MAIN_SERVICE_NAME) {
        if (!newOwner.isEmpty()) {
            d->mainPresent_ = true;
        } else {
            d->mainPresent_ = false;
        }
    } else if (service == FCITX_PORTAL_SERVICE_NAME) {
        if (!newOwner.isEmpty()) {
            d->portalPresent_ = true;
        } else {
            d->portalPresent_ = false;
        }
    }

    updateAvailability();
}

void FcitxQtWatcher::updateAvailability() {
    Q_D(FcitxQtWatcher);
    setAvailability(d->mainPresent_ || d->portalPresent_);
}
} // namespace fcitx
