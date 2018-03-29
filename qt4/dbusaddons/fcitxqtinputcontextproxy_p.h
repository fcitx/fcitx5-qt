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

#ifndef _DBUSADDONS_FCITXQTINPUTCONTEXTPROXY_P_H_
#define _DBUSADDONS_FCITXQTINPUTCONTEXTPROXY_P_H_

#include "fcitxqtinputcontextproxy.h"
#include "fcitxqtinputcontextproxyimpl.h"
#include "fcitxqtinputmethodproxy.h"
#include "fcitxqtwatcher.h"
#include <QDBusServiceWatcher>

namespace fcitx {

class FcitxQtInputContextProxyPrivate {
public:
    FcitxQtInputContextProxyPrivate(FcitxQtWatcher *watcher,
                                    FcitxQtInputContextProxy *q)
        : q_ptr(q), fcitxWatcher_(watcher), watcher_(q) {
        registerFcitxQtDBusTypes();
        QObject::connect(fcitxWatcher_, SIGNAL(availabilityChanged(bool)), q,
                         SLOT(availabilityChanged()));
        watcher_.setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
        QObject::connect(&watcher_, SIGNAL(serviceUnregistered(QString)), q,
                         SLOT(serviceUnregistered()));
        availabilityChanged();
    }

    ~FcitxQtInputContextProxyPrivate() {
        if (isValid()) {
            icproxy_->DestroyIC();
        }
    }

    bool isValid() const { return (icproxy_ && icproxy_->isValid()); }

    void serviceUnregistered() {
        cleanUp();
        availabilityChanged();
    }

    void availabilityChanged() {
        QTimer::singleShot(100, q_ptr, SLOT(recheck()));
    }

    void recheck() {
        if (!isValid() && fcitxWatcher_->availability()) {
            createInputContext();
        }
        if (!fcitxWatcher_->availability()) {
            cleanUp();
        }
    }

    void cleanUp() {
        auto services = watcher_.watchedServices();
        for (const auto &service : services) {
            watcher_.removeWatchedService(service);
        }

        delete improxy_;
        improxy_ = nullptr;
        delete icproxy_;
        icproxy_ = nullptr;
        delete createInputContextWatcher_;
        createInputContextWatcher_ = nullptr;
    }

    void createInputContext() {
        Q_Q(FcitxQtInputContextProxy);
        if (!fcitxWatcher_->availability()) {
            return;
        }

        cleanUp();

        auto service = fcitxWatcher_->serviceName();
        auto connection = fcitxWatcher_->connection();

        auto owner = connection.interface()->serviceOwner(service);
        if (!owner.isValid()) {
            return;
        }

        watcher_.setConnection(connection);
        watcher_.setWatchedServices(QStringList() << owner);
        // Avoid race, query again.
        if (!connection.interface()->isServiceRegistered(owner)) {
            cleanUp();
            return;
        }

        QFileInfo info(QCoreApplication::applicationFilePath());
        portal_ = true;
        improxy_ =
            new FcitxQtInputMethodProxy(owner, "/inputmethod", connection, q);
        FcitxQtStringKeyValueList list;
        FcitxQtStringKeyValue arg;
        arg.setKey("program");
        arg.setValue(info.fileName());
        list << arg;
        if (!display_.isEmpty()) {
            FcitxQtStringKeyValue arg2;
            arg2.setKey("display");
            arg2.setValue(display_);
            list << arg2;
        }

        auto result = improxy_->CreateInputContext(list);
        createInputContextWatcher_ = new QDBusPendingCallWatcher(result);
        QObject::connect(createInputContextWatcher_,
                         SIGNAL(finished(QDBusPendingCallWatcher *)), q,
                         SLOT(createInputContextFinished()));
    }

    void createInputContextFinished() {
        Q_Q(FcitxQtInputContextProxy);
        if (createInputContextWatcher_->isError()) {
            cleanUp();
            return;
        }

        QDBusPendingReply<QDBusObjectPath, QByteArray> reply(
            *createInputContextWatcher_);
        icproxy_ = new FcitxQtInputContextProxyImpl(improxy_->service(),
                                                    reply.value().path(),
                                                    improxy_->connection(), q);
        QObject::connect(icproxy_, SIGNAL(CommitString(QString)), q,
                         SIGNAL(commitString(QString)));
        QObject::connect(icproxy_, SIGNAL(CurrentIM(QString, QString, QString)),
                         q, SIGNAL(currentIM(QString, QString, QString)));
        QObject::connect(icproxy_, SIGNAL(ForwardKey(uint, uint, bool)), q,
                         SIGNAL(forwardKey(uint, uint, bool)));
        QObject::connect(
            icproxy_,
            SIGNAL(UpdateFormattedPreedit(FcitxQtFormattedPreeditList, int)), q,
            SIGNAL(updateFormattedPreedit(FcitxQtFormattedPreeditList, int)));
        QObject::connect(icproxy_, SIGNAL(DeleteSurroundingText(int, uint)), q,
                         SIGNAL(deleteSurroundingText(int, uint)));

        delete createInputContextWatcher_;
        createInputContextWatcher_ = nullptr;
        emit q->inputContextCreated(reply.argumentAt<1>());
    }

    FcitxQtInputContextProxy *q_ptr;
    Q_DECLARE_PUBLIC(FcitxQtInputContextProxy);

    FcitxQtWatcher *fcitxWatcher_;
    QDBusServiceWatcher watcher_;
    FcitxQtInputMethodProxy *improxy_ = nullptr;
    FcitxQtInputContextProxyImpl *icproxy_ = nullptr;
    QDBusPendingCallWatcher *createInputContextWatcher_ = nullptr;
    QString display_;
    bool portal_ = false;
};
} // namespace fcitx

#endif // _DBUSADDONS_FCITXQTINPUTCONTEXTPROXY_P_H_
