/*
 * Copyright (C) 2012~2017 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
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

#ifndef QFCITXPLATFORMINPUTCONTEXT_H
#define QFCITXPLATFORMINPUTCONTEXT_H

#include "fcitxqtinputcontextproxy.h"
#include <QDBusConnection>
#include <QDBusServiceWatcher>
#include <QFileSystemWatcher>
#include <QKeyEvent>
#include <QPointer>
#include <QRect>
#include <QWindow>
#include <fcitx-utils/capabilityflags.h>
#include <qpa/qplatforminputcontext.h>
#include <unordered_map>
#include <xkbcommon/xkbcommon-compose.h>

class FcitxQtConnection;
class QFileSystemWatcher;

struct FcitxQtICData {
    FcitxQtICData()
        : proxy(nullptr), surroundingAnchor(-1), surroundingCursor(-1) {}
    FcitxQtICData(const FcitxQtICData &that) = delete;
    ~FcitxQtICData() {
        if (proxy) {
            if (proxy->isValid()) {
                proxy->DestroyIC();
            }
            delete proxy;
        }
    }
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
          m_event(event.type(), event.key(), event.modifiers(),
                  event.nativeScanCode(), event.nativeVirtualKey(),
                  event.nativeModifiers(), event.text(), event.isAutoRepeat(),
                  event.count()),
          m_window(window) {}

    virtual ~ProcessKeyWatcher() {}

    const QKeyEvent &keyEvent() { return m_event; }

    QWindow *window() { return m_window.data(); }

private:
    QKeyEvent m_event;
    QPointer<QWindow> m_window;
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

class QFcitxPlatformInputContext : public QPlatformInputContext {
    Q_OBJECT
public:
    QFcitxPlatformInputContext();
    virtual ~QFcitxPlatformInputContext();

    virtual bool filterEvent(const QEvent *event) Q_DECL_OVERRIDE;
    virtual bool isValid() const Q_DECL_OVERRIDE;
    virtual void invokeAction(QInputMethod::Action,
                              int cursorPosition) Q_DECL_OVERRIDE;
    virtual void reset() Q_DECL_OVERRIDE;
    virtual void commit() Q_DECL_OVERRIDE;
    virtual void update(Qt::InputMethodQueries quries) Q_DECL_OVERRIDE;
    virtual void setFocusObject(QObject *object) Q_DECL_OVERRIDE;
    virtual QLocale locale() const Q_DECL_OVERRIDE;

public Q_SLOTS:
    void cursorRectChanged();
    void commitString(const QString &str);
    void updateFormattedPreedit(const FcitxQtFormattedPreeditList &preeditList,
                                int cursorPos);
    void deleteSurroundingText(int offset, uint nchar);
    void forwardKey(uint keyval, uint state, int type);
    void createInputContextFinished(QDBusPendingCallWatcher *watcher);
    void connected();
    void cleanUp();
    void windowDestroyed(QObject *object);
    void updateCurrentIM(const QString &name, const QString &uniqueName,
                         const QString &langCode);

private:
    void createInputContext(QWindow *w);
    bool processCompose(uint keyval, uint state, bool isRelaese);
    QKeyEvent *createKeyEvent(uint keyval, uint state, bool isRelaese);

    void addCapacity(FcitxQtICData &data, fcitx::CapabilityFlags capability,
                     bool forceUpdate = false) {
        auto newcaps = data.capability | capability;
        if (data.capability != newcaps || forceUpdate) {
            data.capability = newcaps;
            updateCapacity(data);
        }
    }

    void removeCapacity(FcitxQtICData &data, fcitx::CapabilityFlags capability,
                        bool forceUpdate = false) {
        auto newcaps = data.capability & (~capability);
        if (data.capability != newcaps || forceUpdate) {
            data.capability = newcaps;
            updateCapacity(data);
        }
    }

    void updateCapacity(const FcitxQtICData &data);
    void commitPreedit();
    void createICData(QWindow *w);
    FcitxQtInputContextProxy *validIC();
    FcitxQtInputContextProxy *validICByWindow(QWindow *window);
    bool filterEventFallback(uint keyval, uint keycode, uint state,
                             bool isRelaese);

    FcitxQtConnection *m_connection;
    FcitxQtInputMethodProxy *m_improxy;
    QString m_preedit;
    QString m_commitPreedit;
    FcitxQtFormattedPreeditList m_preeditList;
    int m_cursorPos;
    bool m_useSurroundingText;
    bool m_syncMode;
    QString m_lastSurroundingText;
    int m_lastSurroundingAnchor;
    int m_lastSurroundingCursor;
    std::unordered_map<QWindow *, FcitxQtICData> m_icMap;
    QPointer<QWindow> m_lastWindow;
    bool m_destroy;
    QScopedPointer<struct xkb_context, XkbContextDeleter> m_xkbContext;
    QScopedPointer<struct xkb_compose_table, XkbComposeTableDeleter>
        m_xkbComposeTable;
    QScopedPointer<struct xkb_compose_state, XkbComposeStateDeleter>
        m_xkbComposeState;
    QLocale m_locale;
private slots:
    void processKeyEventFinished(QDBusPendingCallWatcher *);
};

#endif // QFCITXPLATFORMINPUTCONTEXT_H
