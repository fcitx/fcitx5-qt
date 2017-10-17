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
#ifndef _DBUSADDONS_FCITXQTINPUTCONTEXTPROXY_H_
#define _DBUSADDONS_FCITXQTINPUTCONTEXTPROXY_H_

#include "fcitx5qt4dbusaddons_export.h"

#include <QObject>
#include <QDBusServiceWatcher>
#include <QDBusConnection>
#include <QDBusPendingReply>
#include "fcitxqtdbustypes.h"

class QDBusPendingCallWatcher;

namespace fcitx {

class FcitxQtWatcher;
class FcitxQtInputContextProxyPrivate;

class FCITX5QT4DBUSADDONS_EXPORT FcitxQtInputContextProxy : public QObject {
    Q_OBJECT
public:
    FcitxQtInputContextProxy(FcitxQtWatcher *watcher, QObject *parent);
    ~FcitxQtInputContextProxy();

    bool isValid() const;
    void setDisplay(const QString &display);
    const QString &display() const;

public slots:
    QDBusPendingReply<> focusIn();
    QDBusPendingReply<> focusOut();
    QDBusPendingReply<bool> processKeyEvent(uint keyval, uint keycode, uint state, bool type, uint time);
    QDBusPendingReply<> reset();
    QDBusPendingReply<> setCapability(qulonglong caps);
    QDBusPendingReply<> setCursorRect(int x, int y, int w, int h);
    QDBusPendingReply<> setSurroundingText(const QString &text, uint cursor, uint anchor);
    QDBusPendingReply<> setSurroundingTextPosition(uint cursor, uint anchor);

signals:
    void commitString(const QString &str);
    void currentIM(const QString &name, const QString &uniqueName, const QString &langCode);
    void deleteSurroundingText(int offset, uint nchar);
    void forwardKey(uint keyval, uint state, bool isRelease);
    void updateFormattedPreedit(const FcitxQtFormattedPreeditList &str, int cursorpos);
    void inputContextCreated(const QByteArray &uuid);

private:
    Q_PRIVATE_SLOT(d_func(), void availabilityChanged());
    Q_PRIVATE_SLOT(d_func(), void recheck());
    Q_PRIVATE_SLOT(d_func(), void cleanUp());
    Q_PRIVATE_SLOT(d_func(), void serviceUnregistered());
    Q_PRIVATE_SLOT(d_func(), void createInputContextFinished());

    FcitxQtInputContextProxyPrivate *const d_ptr;
    Q_DECLARE_PRIVATE(FcitxQtInputContextProxy);
};

}

#endif // _DBUSADDONS_FCITXQTINPUTCONTEXTPROXY_H_
