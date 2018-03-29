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

#ifndef QFCITXPLATFORMINPUTCONTEXT_H
#define QFCITXPLATFORMINPUTCONTEXT_H

#include "fcitxqtinputcontextproxy.h"
#include "fcitxqtwatcher.h"
#include <QDBusConnection>
#include <QDBusServiceWatcher>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QPointer>
#include <QRect>
#include <QWindow>
#include <fcitx-utils/capabilityflags.h>
#include <qpa/qplatforminputcontext.h>
#include <unordered_map>
#include <xkbcommon/xkbcommon-compose.h>

namespace fcitx {

class FcitxQtConnection;

struct FcitxQtICData {
    FcitxQtICData(FcitxQtWatcher *watcher)
        : proxy(new FcitxQtInputContextProxy(watcher, watcher)),
          surroundingAnchor(-1), surroundingCursor(-1) {}
    FcitxQtICData(const FcitxQtICData &that) = delete;
    ~FcitxQtICData() { delete proxy; }
    fcitx::CapabilityFlags capability;
    FcitxQtInputContextProxy *proxy;
    QRect rect;
    QString surroundingText;
    int surroundingAnchor;
    int surroundingCursor;
};

class ProcessKeyWatcher : public QDBusPendingCallWatcher {
    Q_OBJECT
public:
    ProcessKeyWatcher(const QKeyEvent &event, QWindow *window,
                      const QDBusPendingCall &call, QObject *parent = 0)
        : QDBusPendingCallWatcher(call, parent),
          event_(event.type(), event.key(), event.modifiers(),
                 event.nativeScanCode(), event.nativeVirtualKey(),
                 event.nativeModifiers(), event.text(), event.isAutoRepeat(),
                 event.count()),
          window_(window) {}

    virtual ~ProcessKeyWatcher() {}

    const QKeyEvent &keyEvent() { return event_; }

    QWindow *window() { return window_.data(); }

private:
    QKeyEvent event_;
    QPointer<QWindow> window_;
};

struct XkbContextDeleter {
    static inline void cleanup(struct xkb_context *pointer) {
        if (pointer)
            xkb_context_unref(pointer);
    }
};

struct XkbComposeTableDeleter {
    static inline void cleanup(struct xkb_compose_table *pointer) {
        if (pointer)
            xkb_compose_table_unref(pointer);
    }
};

struct XkbComposeStateDeleter {
    static inline void cleanup(struct xkb_compose_state *pointer) {
        if (pointer)
            xkb_compose_state_unref(pointer);
    }
};

class QFcitxPlatformInputContext : public QPlatformInputContext {
    Q_OBJECT
public:
    QFcitxPlatformInputContext();
    virtual ~QFcitxPlatformInputContext();

    bool filterEvent(const QEvent *event) override;
    bool isValid() const override;
    void invokeAction(QInputMethod::Action, int cursorPosition) override;
    void reset() override;
    void commit() override;
    void update(Qt::InputMethodQueries quries) override;
    void setFocusObject(QObject *object) override;
    QLocale locale() const override;

public Q_SLOTS:
    void cursorRectChanged();
    void commitString(const QString &str);
    void updateFormattedPreedit(const FcitxQtFormattedPreeditList &preeditList,
                                int cursorPos);
    void deleteSurroundingText(int offset, uint nchar);
    void forwardKey(uint keyval, uint state, bool type);
    void createInputContextFinished(const QByteArray &uuid);
    void cleanUp();
    void windowDestroyed(QObject *object);
    void updateCurrentIM(const QString &name, const QString &uniqueName,
                         const QString &langCode);

private:
    bool processCompose(uint keyval, uint state, bool isRelaese);
    QKeyEvent *createKeyEvent(uint keyval, uint state, bool isRelaese);

    void addCapability(FcitxQtICData &data, fcitx::CapabilityFlags capability,
                       bool forceUpdate = false) {
        auto newcaps = data.capability | capability;
        if (data.capability != newcaps || forceUpdate) {
            data.capability = newcaps;
            updateCapability(data);
        }
    }

    void removeCapability(FcitxQtICData &data,
                          fcitx::CapabilityFlags capability,
                          bool forceUpdate = false) {
        auto newcaps = data.capability & (~capability);
        if (data.capability != newcaps || forceUpdate) {
            data.capability = newcaps;
            updateCapability(data);
        }
    }

    void updateCapability(const FcitxQtICData &data);
    void commitPreedit(QPointer<QObject> input = qApp->focusObject());
    void createICData(QWindow *w);
    FcitxQtInputContextProxy *validIC();
    FcitxQtInputContextProxy *validICByWindow(QWindow *window);
    bool filterEventFallback(uint keyval, uint keycode, uint state,
                             bool isRelaese);

    FcitxQtWatcher *watcher_;
    QString preedit_;
    QString commitPreedit_;
    FcitxQtFormattedPreeditList preeditList_;
    int cursorPos_;
    bool useSurroundingText_;
    bool syncMode_;
    std::unordered_map<QWindow *, FcitxQtICData> icMap_;
    QPointer<QWindow> lastWindow_;
    QPointer<QObject> lastObject_;
    bool destroy_;
    QScopedPointer<struct xkb_context, XkbContextDeleter> xkbContext_;
    QScopedPointer<struct xkb_compose_table, XkbComposeTableDeleter>
        xkbComposeTable_;
    QScopedPointer<struct xkb_compose_state, XkbComposeStateDeleter>
        xkbComposeState_;
    QLocale locale_;
private slots:
    void processKeyEventFinished(QDBusPendingCallWatcher *);
};
} // namespace fcitx

#endif // QFCITXPLATFORMINPUTCONTEXT_H
