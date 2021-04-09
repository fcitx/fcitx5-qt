/*
 * SPDX-FileCopyrightText: 2012~2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef QFCITXPLATFORMINPUTCONTEXT_H
#define QFCITXPLATFORMINPUTCONTEXT_H

#include "fcitxcandidatewindow.h"
#include "fcitxqtinputcontextproxy.h"
#include "fcitxqtwatcher.h"
#include <QDBusConnection>
#include <QDBusServiceWatcher>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QPointer>
#include <QRect>
#include <QWindow>
#include <memory>
#include <qpa/qplatforminputcontext.h>
#include <unordered_map>
#include <xkbcommon/xkbcommon-compose.h>

namespace fcitx {

class FcitxQtConnection;
class FcitxTheme;

struct FcitxQtICData {
    FcitxQtICData(FcitxQtWatcher *watcher, QWindow *window)
        : proxy(new FcitxQtInputContextProxy(watcher, watcher)),
          watcher_(watcher), window_(window) {
        proxy->setProperty("icData",
                           QVariant::fromValue(static_cast<void *>(this)));
        QObject::connect(window, &QWindow::visibilityChanged, proxy,
                         [this](bool visible) {
                             if (!visible) {
                                 resetCandidateWindow();
                             }
                         });
        QObject::connect(watcher, &FcitxQtWatcher::availabilityChanged, proxy,
                         [this](bool avail) {
                             if (!avail) {
                                 resetCandidateWindow();
                             }
                         });
    }
    FcitxQtICData(const FcitxQtICData &that) = delete;
    ~FcitxQtICData() {
        delete proxy;
        resetCandidateWindow();
    }

    FcitxCandidateWindow *candidateWindow(FcitxTheme *theme) {
        if (!candidateWindow_) {
            candidateWindow_ = new FcitxCandidateWindow(window(), theme);
            QObject::connect(
                candidateWindow_, &FcitxCandidateWindow::candidateSelected,
                proxy,
                [proxy = proxy](int index) { proxy->selectCandidate(index); });
            QObject::connect(candidateWindow_,
                             &FcitxCandidateWindow::prevClicked, proxy,
                             [proxy = proxy]() { proxy->prevPage(); });
            QObject::connect(candidateWindow_,
                             &FcitxCandidateWindow::nextClicked, proxy,
                             [proxy = proxy]() { proxy->nextPage(); });
        }
        return candidateWindow_;
    }

    QWindow *window() { return window_.data(); }
    auto *watcher() { return watcher_; }

    void resetCandidateWindow() {
        if (auto *w = candidateWindow_.data()) {
            candidateWindow_ = nullptr;
            w->deleteLater();
            return;
        }
    }

    quint64 capability = 0;
    FcitxQtInputContextProxy *proxy;
    QRect rect;
    // Last key event forwarded.
    std::unique_ptr<QKeyEvent> event;
    QString surroundingText;
    int surroundingAnchor = -1;
    int surroundingCursor = -1;

private:
    FcitxQtWatcher *watcher_;
    QPointer<QWindow> window_;
    QPointer<FcitxCandidateWindow> candidateWindow_;
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

public Q_SLOTS:
    void cursorRectChanged();
    void commitString(const QString &str);
    void updateFormattedPreedit(const FcitxQtFormattedPreeditList &preeditList,
                                int cursorPos);
    void deleteSurroundingText(int offset, unsigned int nchar);
    void forwardKey(unsigned int keyval, unsigned int state, bool type);
    void createInputContextFinished(const QByteArray &uuid);
    void cleanUp();
    void windowDestroyed(QObject *object);
    void updateCurrentIM(const QString &name, const QString &uniqueName,
                         const QString &langCode);
    void updateClientSideUI(const FcitxQtFormattedPreeditList &preedit,
                            int cursorpos,
                            const FcitxQtFormattedPreeditList &auxUp,
                            const FcitxQtFormattedPreeditList &auxDown,
                            const FcitxQtStringKeyValueList &candidates,
                            int candidateIndex, int layoutHint, bool hasPrev,
                            bool hasNext);
private Q_SLOTS:
    void processKeyEventFinished(QDBusPendingCallWatcher *);

private:
    bool processCompose(unsigned int keyval, unsigned int state,
                        bool isRelaese);
    QKeyEvent *createKeyEvent(unsigned int keyval, unsigned int state,
                              bool isRelaese, const QKeyEvent *event);
    void forwardEvent(QWindow *window, const QKeyEvent &event);

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
    bool filterEventFallback(unsigned int keyval, unsigned int keycode,
                             unsigned int state, bool isRelaese);

    Q_INVOKABLE void updateCursorRect(QPointer<QWindow> window);

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
    FcitxTheme *theme_ = nullptr;
};
} // namespace fcitx

#endif // QFCITXPLATFORMINPUTCONTEXT_H
