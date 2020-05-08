/*
 * SPDX-FileCopyrightText: 2012~2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "fcitxqtinputcontextproxy_p.h"
#include <QCoreApplication>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QFileInfo>
#include <QTimer>

namespace fcitx {

FcitxQtInputContextProxy::FcitxQtInputContextProxy(FcitxQtWatcher *watcher,
                                                   QObject *parent)
    : QObject(parent),
      d_ptr(new FcitxQtInputContextProxyPrivate(watcher, this)) {}

FcitxQtInputContextProxy::~FcitxQtInputContextProxy() { delete d_ptr; }

void FcitxQtInputContextProxy::setDisplay(const QString &display) {
    Q_D(FcitxQtInputContextProxy);
    d->display_ = display;
}

const QString &FcitxQtInputContextProxy::display() const {
    Q_D(const FcitxQtInputContextProxy);
    return d->display_;
}

bool FcitxQtInputContextProxy::isValid() const {
    Q_D(const FcitxQtInputContextProxy);
    return d->isValid();
}

QDBusPendingReply<> FcitxQtInputContextProxy::focusIn() {
    Q_D(FcitxQtInputContextProxy);
    return d->icproxy_->FocusIn();
}

QDBusPendingReply<> FcitxQtInputContextProxy::focusOut() {
    Q_D(FcitxQtInputContextProxy);
    return d->icproxy_->FocusOut();
}

QDBusPendingReply<bool>
FcitxQtInputContextProxy::processKeyEvent(uint keyval, uint keycode, uint state,
                                          bool type, uint time) {
    Q_D(FcitxQtInputContextProxy);
    return d->icproxy_->ProcessKeyEvent(keyval, keycode, state, type, time);
}

QDBusPendingReply<> FcitxQtInputContextProxy::reset() {
    Q_D(FcitxQtInputContextProxy);
    return d->icproxy_->Reset();
}

QDBusPendingReply<> FcitxQtInputContextProxy::setCapability(qulonglong caps) {
    Q_D(FcitxQtInputContextProxy);
    return d->icproxy_->SetCapability(caps);
}

QDBusPendingReply<> FcitxQtInputContextProxy::setCursorRect(int x, int y, int w,
                                                            int h) {
    Q_D(FcitxQtInputContextProxy);
    return d->icproxy_->SetCursorRect(x, y, w, h);
}

QDBusPendingReply<>
FcitxQtInputContextProxy::setSurroundingText(const QString &text, uint cursor,
                                             uint anchor) {
    Q_D(FcitxQtInputContextProxy);
    return d->icproxy_->SetSurroundingText(text, cursor, anchor);
}

QDBusPendingReply<>
FcitxQtInputContextProxy::setSurroundingTextPosition(uint cursor, uint anchor) {
    Q_D(FcitxQtInputContextProxy);
    return d->icproxy_->SetSurroundingTextPosition(cursor, anchor);
}

} // namespace fcitx
