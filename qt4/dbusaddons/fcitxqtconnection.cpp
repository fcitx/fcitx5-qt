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

#include "fcitxqtconnection_p.h"
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusReply>
#include <QDBusServiceWatcher>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QTimer>

#include <errno.h>
#include <signal.h>

// utils function in fcitx-utils and fcitx-config
bool _pid_exists(pid_t pid) {
    if (pid <= 0)
        return 0;
    return !(kill(pid, 0) && (errno == ESRCH));
}

FcitxQtConnection::FcitxQtConnection(QObject *parent)
    : QObject(parent), d_ptr(new FcitxQtConnectionPrivate(this)) {}

void FcitxQtConnection::startConnection() {
    Q_D(FcitxQtConnection);
    if (!d->m_initialized) {
        d->initialize();
        d->createConnection();
    }
}

void FcitxQtConnection::endConnection() {
    Q_D(FcitxQtConnection);
    d->cleanUp();
    d->finalize();
    d->m_connectedOnce = false;
}

bool FcitxQtConnection::autoReconnect() {
    Q_D(FcitxQtConnection);
    return d->m_autoReconnect;
}

void FcitxQtConnection::setAutoReconnect(bool a) {
    Q_D(FcitxQtConnection);
    d->m_autoReconnect = a;
}

QDBusConnection *FcitxQtConnection::connection() {
    Q_D(FcitxQtConnection);
    return d->m_connection;
}

const QString &FcitxQtConnection::serviceName() {
    Q_D(FcitxQtConnection);
    return d->m_serviceName;
}

bool FcitxQtConnection::isConnected() {
    Q_D(FcitxQtConnection);
    return d->isConnected();
}

FcitxQtConnection::~FcitxQtConnection() {}

FcitxQtConnectionPrivate::FcitxQtConnectionPrivate(FcitxQtConnection *conn)
    : QObject(conn), q_ptr(conn), m_serviceName("org.fcitx.Fcitx5"),
      m_connection(nullptr), m_serviceWatcher(new QDBusServiceWatcher(this)),
      m_autoReconnect(true), m_connectedOnce(false), m_initialized(false) {}

FcitxQtConnectionPrivate::~FcitxQtConnectionPrivate() {
    if (m_connection)
        delete m_connection;
}

void FcitxQtConnectionPrivate::initialize() {
    m_serviceWatcher->setConnection(QDBusConnection::sessionBus());
    m_serviceWatcher->addWatchedService(m_serviceName);
    m_initialized = true;
}

void FcitxQtConnectionPrivate::finalize() {
    m_serviceWatcher->removeWatchedService(m_serviceName);
    m_initialized = false;
}

void FcitxQtConnectionPrivate::createConnection() {
    if (m_connectedOnce && !m_autoReconnect) {
        return;
    }

    disconnect(m_serviceWatcher,
               SIGNAL(serviceOwnerChanged(QString, QString, QString)), this,
               SLOT(imChanged(QString, QString, QString)));
    if (!m_connection) {
        QDBusConnection *connection =
            new QDBusConnection(QDBusConnection::sessionBus());
        connect(m_serviceWatcher,
                SIGNAL(serviceOwnerChanged(QString, QString, QString)), this,
                SLOT(imChanged(QString, QString, QString)));
        QDBusReply<bool> registered =
            connection->interface()->isServiceRegistered(m_serviceName);
        if (!registered.isValid() || !registered.value()) {
            delete connection;
        } else {
            m_connection = connection;
        }
    }

    Q_Q(FcitxQtConnection);
    if (m_connection) {

        m_connection->connect("org.freedesktop.DBus.Local",
                              "/org/freedesktop/DBus/Local",
                              "org.freedesktop.DBus.Local", "Disconnected",
                              this, SLOT(dbusDisconnected()));
        m_connectedOnce = true;
        emit q->connected();
    }
}

void FcitxQtConnectionPrivate::dbusDisconnected() {
    cleanUp();

    createConnection();
}

void FcitxQtConnectionPrivate::imChanged(const QString &service,
                                         const QString &oldowner,
                                         const QString &newowner) {
    if (service == m_serviceName) {
        /* old die */
        if (oldowner.length() > 0 || newowner.length() > 0)
            cleanUp();

        /* new rise */
        if (newowner.length() > 0) {
            QTimer::singleShot(100, this, SLOT(newServiceAppear()));
        }
    }
}

void FcitxQtConnectionPrivate::cleanUp() {
    Q_Q(FcitxQtConnection);
    bool doemit = false;
    if (m_connection) {
        delete m_connection;
        m_connection = nullptr;
        doemit = true;
    }

    if (!m_autoReconnect && m_connectedOnce)
        finalize();

    /* we want m_connection and finalize being called before the signal
     * thus isConnected will return false in slot
     * and startConnection can be called in slot
     */
    if (doemit)
        emit q->disconnected();
}

bool FcitxQtConnectionPrivate::isConnected() {
    return m_connection && m_connection->isConnected();
}

void FcitxQtConnectionPrivate::newServiceAppear() {
    if (!isConnected()) {
        cleanUp();

        createConnection();
    }
}
