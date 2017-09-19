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
#ifndef _DBUSADDONS_FCITXQTCONNECTION_P_H_
#define _DBUSADDONS_FCITXQTCONNECTION_P_H_

#include "fcitxqtconnection.h"

class QDBusConnection;
class QDBusServiceWatcher;

namespace fcitx {

class FcitxQtConnectionPrivate : public QObject {
    Q_OBJECT
public:
    FcitxQtConnectionPrivate(FcitxQtConnection *conn);
    virtual ~FcitxQtConnectionPrivate();
    FcitxQtConnection *const q_ptr;
    Q_DECLARE_PUBLIC(FcitxQtConnection);

private Q_SLOTS:
    void imChanged(const QString &service, const QString &oldowner,
                   const QString &newowner);
    void dbusDisconnected();
    void cleanUp();
    void newServiceAppear();

private:
    bool isConnected();
    void createConnection();
    void initialize();
    void finalize();

    QString m_serviceName;
    QDBusConnection *m_connection;
    QDBusServiceWatcher *m_serviceWatcher;
    QString m_socketFile;
    bool m_autoReconnect;
    bool m_connectedOnce;
    bool m_initialized;
};
}

#endif // _DBUSADDONS_FCITXQTCONNECTION_P_H_
