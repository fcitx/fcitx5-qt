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

#ifndef QFCITXINPUTCONTEXT_H
#define QFCITXINPUTCONTEXT_H

#include "fcitxqtinputcontextproxy.h"
#include "fcitxqtwatcher.h"
#include <QApplication>
#include <QDBusConnection>
#include <QDBusServiceWatcher>
#include <QInputContext>
#include <QKeyEvent>
#include <QPointer>
#include <QRect>
#include <QWidget>
#include <fcitx-utils/capabilityflags.h>
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
    ProcessKeyWatcher(const QKeyEvent &event, QWidget *window,
                      const QDBusPendingCall &call, QObject *parent = 0)
        : QDBusPendingCallWatcher(call, parent),
          m_event(QKeyEvent::createExtendedKeyEvent(
              event.type(), event.key(), event.modifiers(),
              event.nativeScanCode(), event.nativeVirtualKey(),
              event.nativeModifiers(), event.text(), event.isAutoRepeat(),
              event.count())),
          m_window(window) {}

    virtual ~ProcessKeyWatcher() { delete m_event; }

    QKeyEvent &keyEvent() { return *m_event; }

    QWidget *window() { return m_window.data(); }

private:
    QKeyEvent *m_event;
    QPointer<QWidget> m_window;
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

class FcitxQtInputMethodProxy;

class QFcitxInputContext : public QInputContext {
    Q_OBJECT
public:
    QFcitxInputContext();
    virtual ~QFcitxInputContext();

    QString identifierName() override;
    QString language() override;
    void reset() override;
    bool isComposing() const override { return false; };
    void update() override;
    void setFocusWidget(QWidget *w) override;

    void widgetDestroyed(QWidget *w) override;

    bool filterEvent(const QEvent *event) override;
    void mouseHandler(int x, QMouseEvent *event) override;

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
    void commitPreedit(QPointer<QWidget> input = qApp->focusWidget());
    void createICData(QWidget *w);
    FcitxQtInputContextProxy *validIC();
    FcitxQtInputContextProxy *validICByWindow(QWidget *window);
    bool filterEventFallback(uint keyval, uint keycode, uint state,
                             bool isRelaese);

    FcitxQtWatcher *m_watcher;
    QString m_preedit;
    QString m_commitPreedit;
    FcitxQtFormattedPreeditList m_preeditList;
    int m_cursorPos;
    bool m_useSurroundingText;
    bool m_syncMode;
    QString m_lastSurroundingText;
    int m_lastSurroundingAnchor = 0;
    int m_lastSurroundingCursor = 0;
    std::unordered_map<QWidget *, FcitxQtICData> m_icMap;
    QPointer<QWidget> m_lastWindow;
    bool m_destroy;
    QScopedPointer<struct xkb_context, XkbContextDeleter> m_xkbContext;
    QScopedPointer<struct xkb_compose_table, XkbComposeTableDeleter>
        m_xkbComposeTable;
    QScopedPointer<struct xkb_compose_state, XkbComposeStateDeleter>
        m_xkbComposeState;
private slots:
    void processKeyEventFinished(QDBusPendingCallWatcher *);
};
}

#endif // QFCITXINPUTCONTEXT_H
