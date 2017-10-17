/*
* Copyright (C) 2017~2017 by CSSlayer
* wengxt@gmail.com
*
* This library is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation; either version 2.1 of the
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
#ifndef _DBUSADDONS_FCITXQTWATCHER_H_
#define _DBUSADDONS_FCITXQTWATCHER_H_

#include "fcitx5qt4dbusaddons_export.h"

#include <QDBusConnection>
#include <QObject>

namespace fcitx {

class FcitxQtWatcherPrivate;

class FCITX5QT4DBUSADDONS_EXPORT FcitxQtWatcher : public QObject {
    Q_OBJECT
public:
    explicit FcitxQtWatcher(QObject *parent = nullptr);
    explicit FcitxQtWatcher(const QDBusConnection &connection,
                            QObject *parent = nullptr);
    ~FcitxQtWatcher();
    void watch();
    void unwatch();
    void setConnection(const QDBusConnection &connection);
    QDBusConnection connection() const;
    void setWatchPortal(bool portal);
    bool watchPortal() const;
    bool isWatching() const;
    bool availability() const;

    QString serviceName() const;

signals:
    void availabilityChanged(bool);

private slots:
    void imChanged(const QString &service, const QString &oldOwner,
                   const QString &newOwner);

private:
    void setAvailability(bool availability);
    void updateAvailability();

    FcitxQtWatcherPrivate *const d_ptr;
    Q_DECLARE_PRIVATE(FcitxQtWatcher);
};
}

#endif // _DBUSADDONS_FCITXQTWATCHER_H_
