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
