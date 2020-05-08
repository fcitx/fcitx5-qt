/*
 * SPDX-FileCopyrightText: 2012~2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
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
        QObject::connect(fcitxWatcher_, &FcitxQtWatcher::availabilityChanged, q,
                         [this]() { availabilityChanged(); });
        watcher_.setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
        QObject::connect(&watcher_, &QDBusServiceWatcher::serviceUnregistered,
                         q, [this]() {
                             cleanUp();
                             availabilityChanged();
                         });
        availabilityChanged();
    }

    ~FcitxQtInputContextProxyPrivate() {
        if (isValid()) {
            icproxy_->DestroyIC();
        }
    }

    bool isValid() const { return (icproxy_ && icproxy_->isValid()); }

    void availabilityChanged() {
        QTimer::singleShot(100, q_ptr, [this]() { recheck(); });
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
        improxy_ = new FcitxQtInputMethodProxy(
            owner, "/org/freedesktop/portal/inputmethod", connection, q);
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
                         &QDBusPendingCallWatcher::finished, q,
                         [this]() { createInputContextFinished(); });
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
        QObject::connect(icproxy_, &FcitxQtInputContextProxyImpl::CommitString,
                         q, &FcitxQtInputContextProxy::commitString);
        QObject::connect(icproxy_, &FcitxQtInputContextProxyImpl::CurrentIM, q,
                         &FcitxQtInputContextProxy::currentIM);
        QObject::connect(icproxy_,
                         &FcitxQtInputContextProxyImpl::DeleteSurroundingText,
                         q, &FcitxQtInputContextProxy::deleteSurroundingText);
        QObject::connect(icproxy_, &FcitxQtInputContextProxyImpl::ForwardKey, q,
                         &FcitxQtInputContextProxy::forwardKey);
        QObject::connect(icproxy_,
                         &FcitxQtInputContextProxyImpl::UpdateFormattedPreedit,
                         q, &FcitxQtInputContextProxy::updateFormattedPreedit);

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
