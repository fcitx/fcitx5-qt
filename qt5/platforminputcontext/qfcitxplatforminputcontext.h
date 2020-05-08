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
#include <qpa/qplatforminputcontext.h>
#include <unordered_map>
#include <xkbcommon/xkbcommon-compose.h>

namespace fcitx {

class FcitxQtConnection;

// This need to keep sync with fcitx5.
enum FcitxCapabilityFlag : uint64_t {
    FcitxCapabilityFlag_Preedit = (1 << 1),
    FcitxCapabilityFlag_Password = (1 << 3),
    FcitxCapabilityFlag_FormattedPreedit = (1 << 4),
    FcitxCapabilityFlag_ClientUnfocusCommit = (1 << 5),
    FcitxCapabilityFlag_SurroundingText = (1 << 6),
    FcitxCapabilityFlag_Email = (1 << 7),
    FcitxCapabilityFlag_Digit = (1 << 8),
    FcitxCapabilityFlag_Uppercase = (1 << 9),
    FcitxCapabilityFlag_Lowercase = (1 << 10),
    FcitxCapabilityFlag_NoAutoUpperCase = (1 << 11),
    FcitxCapabilityFlag_Url = (1 << 12),
    FcitxCapabilityFlag_Dialable = (1 << 13),
    FcitxCapabilityFlag_Number = (1 << 14),
    FcitxCapabilityFlag_NoSpellCheck = (1 << 17),
    FcitxCapabilityFlag_Alpha = (1 << 21),
    FcitxCapabilityFlag_GetIMInfoOnFocus = (1 << 23),
    FcitxCapabilityFlag_RelativeRect = (1 << 24),

    FcitxCapabilityFlag_Multiline = (1ull << 35),
    FcitxCapabilityFlag_Sensitive = (1ull << 36),
};

enum FcitxTextFormatFlag : int {
    FcitxTextFormatFlag_Underline = (1 << 3), /**< underline is a flag */
    FcitxTextFormatFlag_HighLight = (1 << 4), /**< highlight the preedit */
    FcitxTextFormatFlag_DontCommit = (1 << 5),
    FcitxTextFormatFlag_Bold = (1 << 6),
    FcitxTextFormatFlag_Strike = (1 << 7),
    FcitxTextFormatFlag_Italic = (1 << 8),
    FcitxTextFormatFlag_None = 0,
};

struct FcitxQtICData {
    FcitxQtICData(FcitxQtWatcher *watcher)
        : proxy(new FcitxQtInputContextProxy(watcher, watcher)),
          surroundingAnchor(-1), surroundingCursor(-1) {}
    FcitxQtICData(const FcitxQtICData &that) = delete;
    ~FcitxQtICData() { delete proxy; }
    quint64 capability;
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

    bool isValid() const override;
    void setFocusObject(QObject *object) override;
    void invokeAction(QInputMethod::Action, int cursorPosition) override;
    void reset() override;
    void commit() override;
    void update(Qt::InputMethodQueries quries) override;
    bool filterEvent(const QEvent *event) override;
    QLocale locale() const override;
    bool hasCapability(Capability capability) const override;

public slots:
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
private slots:
    void processKeyEventFinished(QDBusPendingCallWatcher *);

private:
    bool processCompose(uint keyval, uint state, bool isRelaese);
    QKeyEvent *createKeyEvent(uint keyval, uint state, bool isRelaese);

    void addCapability(FcitxQtICData &data, quint64 capability,
                       bool forceUpdate = false) {
        auto newcaps = data.capability | capability;
        if (data.capability != newcaps || forceUpdate) {
            data.capability = newcaps;
            updateCapability(data);
        }
    }

    void removeCapability(FcitxQtICData &data, quint64 capability,
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
};
} // namespace fcitx

#endif // QFCITXPLATFORMINPUTCONTEXT_H
