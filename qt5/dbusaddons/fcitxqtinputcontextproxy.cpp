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

#include "fcitxqtinputcontextproxy_p.h"
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusConnectionInterface>
#include <QFileInfo>
#include <QCoreApplication>
#include <QTimer>

namespace fcitx {

FcitxQtInputContextProxy::FcitxQtInputContextProxy(FcitxQtWatcher *watcher, QObject *parent) : QObject(parent),
d_ptr(new FcitxQtInputContextProxyPrivate(watcher, this))
{
}

FcitxQtInputContextProxy::~FcitxQtInputContextProxy()
{
    delete d_ptr;
}

void FcitxQtInputContextProxy::setDisplay(const QString& display)
{
    Q_D(FcitxQtInputContextProxy);
    d->m_display = display;
}

const QString &FcitxQtInputContextProxy::display() const
{
    Q_D(const FcitxQtInputContextProxy);
    return d->m_display;
}

bool FcitxQtInputContextProxy::isValid() const
{
    Q_D(const FcitxQtInputContextProxy);
    return d->isValid();
}

QDBusPendingReply<> FcitxQtInputContextProxy::focusIn()
{
    Q_D(FcitxQtInputContextProxy);
    return d->m_icproxy->FocusIn();
}

QDBusPendingReply<> FcitxQtInputContextProxy::focusOut()
{
    Q_D(FcitxQtInputContextProxy);
    return d->m_icproxy->FocusOut();
}

QDBusPendingReply<bool> FcitxQtInputContextProxy::processKeyEvent(uint keyval, uint keycode, uint state, bool type, uint time)
{
    Q_D(FcitxQtInputContextProxy);
    return d->m_icproxy->ProcessKeyEvent(keyval, keycode, state, type, time);
}

QDBusPendingReply<> FcitxQtInputContextProxy::reset()
{
    Q_D(FcitxQtInputContextProxy);
    return d->m_icproxy->Reset();
}

QDBusPendingReply<> FcitxQtInputContextProxy::setCapability(qulonglong caps)
{
    Q_D(FcitxQtInputContextProxy);
    return d->m_icproxy->SetCapability(caps);
}

QDBusPendingReply<> FcitxQtInputContextProxy::setCursorRect(int x, int y, int w, int h)
{
    Q_D(FcitxQtInputContextProxy);
    return d->m_icproxy->SetCursorRect(x, y, w, h);
}

QDBusPendingReply<> FcitxQtInputContextProxy::setSurroundingText(const QString &text, uint cursor, uint anchor)
{
    Q_D(FcitxQtInputContextProxy);
    return d->m_icproxy->SetSurroundingText(text, cursor, anchor);
}

QDBusPendingReply<> FcitxQtInputContextProxy::setSurroundingTextPosition(uint cursor, uint anchor)
{
    Q_D(FcitxQtInputContextProxy);
    return d->m_icproxy->SetSurroundingTextPosition(cursor, anchor);
}

}
