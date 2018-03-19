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
        : q_ptr(q), m_fcitxWatcher(watcher), m_watcher(q) {
        registerFcitxQtDBusTypes();
        QObject::connect(m_fcitxWatcher, &FcitxQtWatcher::availabilityChanged,
                         q, [this]() { availabilityChanged(); });
        m_watcher.setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
        QObject::connect(&m_watcher, &QDBusServiceWatcher::serviceUnregistered,
                         q, [this]() {
                             cleanUp();
                             availabilityChanged();
                         });
        availabilityChanged();
    }

    ~FcitxQtInputContextProxyPrivate() {
        if (isValid()) {
            m_icproxy->DestroyIC();
        }
    }

    bool isValid() const { return (m_icproxy && m_icproxy->isValid()); }

    void availabilityChanged() {
        QTimer::singleShot(100, q_ptr, [this]() { recheck(); });
    }

    void recheck() {
        if (!isValid() && m_fcitxWatcher->availability()) {
            createInputContext();
        }
        if (!m_fcitxWatcher->availability()) {
            cleanUp();
        }
    }

    void cleanUp() {
        auto services = m_watcher.watchedServices();
        for (const auto &service : services) {
            m_watcher.removeWatchedService(service);
        }

        delete m_improxy;
        m_improxy = nullptr;
        delete m_icproxy;
        m_icproxy = nullptr;
        delete m_createInputContextWatcher;
        m_createInputContextWatcher = nullptr;
    }

    void createInputContext() {
        Q_Q(FcitxQtInputContextProxy);
        if (!m_fcitxWatcher->availability()) {
            return;
        }

        cleanUp();

        auto service = m_fcitxWatcher->serviceName();
        auto connection = m_fcitxWatcher->connection();

        auto owner = connection.interface()->serviceOwner(service);
        if (!owner.isValid()) {
            return;
        }

        m_watcher.setConnection(connection);
        m_watcher.setWatchedServices(QStringList() << owner);
        // Avoid race, query again.
        if (!connection.interface()->isServiceRegistered(owner)) {
            cleanUp();
            return;
        }

        QFileInfo info(QCoreApplication::applicationFilePath());
        m_portal = true;
        m_improxy =
            new FcitxQtInputMethodProxy(owner, "/inputmethod", connection, q);
        FcitxQtStringKeyValueList list;
        FcitxQtStringKeyValue arg;
        arg.setKey("program");
        arg.setValue(info.fileName());
        list << arg;
        if (!m_display.isEmpty()) {
            FcitxQtStringKeyValue arg2;
            arg2.setKey("display");
            arg2.setValue(m_display);
            list << arg2;
        }

        auto result = m_improxy->CreateInputContext(list);
        m_createInputContextWatcher = new QDBusPendingCallWatcher(result);
        QObject::connect(m_createInputContextWatcher,
                         &QDBusPendingCallWatcher::finished, q,
                         [this]() { createInputContextFinished(); });
    }

    void createInputContextFinished() {
        Q_Q(FcitxQtInputContextProxy);
        if (m_createInputContextWatcher->isError()) {
            cleanUp();
            return;
        }

        QDBusPendingReply<QDBusObjectPath, QByteArray> reply(
            *m_createInputContextWatcher);
        m_icproxy = new FcitxQtInputContextProxyImpl(
            m_improxy->service(), reply.value().path(), m_improxy->connection(),
            q);
        QObject::connect(m_icproxy, &FcitxQtInputContextProxyImpl::CommitString,
                         q, &FcitxQtInputContextProxy::commitString);
        QObject::connect(m_icproxy, &FcitxQtInputContextProxyImpl::CurrentIM, q,
                         &FcitxQtInputContextProxy::currentIM);
        QObject::connect(m_icproxy,
                         &FcitxQtInputContextProxyImpl::DeleteSurroundingText,
                         q, &FcitxQtInputContextProxy::deleteSurroundingText);
        QObject::connect(m_icproxy, &FcitxQtInputContextProxyImpl::ForwardKey,
                         q, &FcitxQtInputContextProxy::forwardKey);
        QObject::connect(m_icproxy,
                         &FcitxQtInputContextProxyImpl::UpdateFormattedPreedit,
                         q, &FcitxQtInputContextProxy::updateFormattedPreedit);

        delete m_createInputContextWatcher;
        m_createInputContextWatcher = nullptr;
        emit q->inputContextCreated(reply.argumentAt<1>());
    }

    FcitxQtInputContextProxy *q_ptr;
    Q_DECLARE_PUBLIC(FcitxQtInputContextProxy);

    FcitxQtWatcher *m_fcitxWatcher;
    QDBusServiceWatcher m_watcher;
    FcitxQtInputMethodProxy *m_improxy = nullptr;
    FcitxQtInputContextProxyImpl *m_icproxy = nullptr;
    QDBusPendingCallWatcher *m_createInputContextWatcher = nullptr;
    QString m_display;
    bool m_portal = false;
    qulonglong m_capability = 0;
};
} // namespace fcitx

#endif // _DBUSADDONS_FCITXQTINPUTCONTEXTPROXY_P_H_
