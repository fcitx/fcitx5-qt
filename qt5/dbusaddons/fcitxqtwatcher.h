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

#ifndef _DBUSADDONS_FCITXQTWATCHER_H_
#define _DBUSADDONS_FCITXQTWATCHER_H_

#include "fcitx5qt5dbusaddons_export.h"

#include <QDBusConnection>
#include <QObject>

namespace fcitx {

class FcitxQtWatcherPrivate;

class FCITX5QT5DBUSADDONS_EXPORT FcitxQtWatcher : public QObject {
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
