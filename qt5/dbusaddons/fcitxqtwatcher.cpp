/***************************************************************************
 *   Copyright (C) 2011~2012 by CSSlayer                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

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
        emit availibilityChanged(d->m_availability);
    }
}

void FcitxQtWatcher::watch() {
    Q_D(FcitxQtWatcher);
    if (d->m_watched) {
        return;
    }

    connect(&d->m_serviceWatcher, &QDBusServiceWatcher::serviceOwnerChanged,
            this, &FcitxQtWatcher::imChanged);
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

    updateAvailbility();

    d->m_watched = true;
}

void FcitxQtWatcher::unwatch() {
    Q_D(FcitxQtWatcher);
    if (!d->m_watched) {
        return;
    }
    disconnect(&d->m_serviceWatcher, &QDBusServiceWatcher::serviceOwnerChanged,
               this, &FcitxQtWatcher::imChanged);
    d->m_mainPresent = false;
    d->m_portalPresent =false;
    d->m_watched = false;
    updateAvailbility();
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

    updateAvailbility();
}

void FcitxQtWatcher::updateAvailbility() {
    Q_D(FcitxQtWatcher);
    setAvailability(d->m_mainPresent || d->m_portalPresent);
}
}
