//
// Copyright (C) 2011~2017 by CSSlayer
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

#include <QDBusConnection>
#include <QDebug>
#include <QGuiApplication>
#include <QInputMethod>
#include <QKeyEvent>
#include <QPalette>
#include <QTextCharFormat>
#include <QTextCodec>
#include <QWindow>
#include <QX11Info>
#include <qpa/qplatformcursor.h>
#include <qpa/qplatformscreen.h>
#include <qpa/qwindowsysteminterface.h>

#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include "qtkey.h"

#include "fcitxqtinputcontextproxy.h"
#include "qfcitxplatforminputcontext.h"

#include <fcitx-utils/key.h>
#include <fcitx-utils/textformatflags.h>
#include <fcitx-utils/utf8.h>

#include <memory>
#include <xcb/xcb.h>

namespace fcitx {

template <typename T>
using XCBReply = std::unique_ptr<T, decltype(&std::free)>;

template <typename T>
XCBReply<T> makeXCBReply(T *ptr) noexcept {
    return {ptr, &std::free};
}

void setFocusGroupForX11(const QByteArray &uuid) {
    if (uuid.size() != 16) {
        return;
    }

    if (!QX11Info::isPlatformX11()) {
        return;
    }
    auto connection = QX11Info::connection();
    xcb_atom_t result = XCB_ATOM_NONE;
    {
        char atomName[] = "_FCITX_SERVER";
        xcb_intern_atom_cookie_t cookie =
            xcb_intern_atom(connection, false, strlen(atomName), atomName);
        auto reply =
            makeXCBReply(xcb_intern_atom_reply(connection, cookie, nullptr));
        if (reply) {
            result = reply->atom;
        }

        if (result == XCB_ATOM_NONE) {
            return;
        }
    }

    xcb_window_t owner = XCB_WINDOW_NONE;
    {
        auto cookie = xcb_get_selection_owner(connection, result);
        auto reply = makeXCBReply(
            xcb_get_selection_owner_reply(connection, cookie, nullptr));
        if (reply) {
            owner = reply->owner;
        }
    }
    if (owner == XCB_WINDOW_NONE) {
        return;
    }

    xcb_client_message_event_t ev;

    memset(&ev, 0, sizeof(ev));
    ev.response_type = XCB_CLIENT_MESSAGE;
    ev.window = owner;
    ev.type = result;
    ev.format = 8;
    memcpy(ev.data.data8, uuid.constData(), 16);

    xcb_send_event(connection, false, owner, XCB_EVENT_MASK_NO_EVENT,
                   reinterpret_cast<char *>(&ev));
}

static bool key_filtered = false;

static bool get_boolean_env(const char *name, bool defval) {
    const char *value = getenv(name);

    if (value == nullptr)
        return defval;

    if (strcmp(value, "") == 0 || strcmp(value, "0") == 0 ||
        strcmp(value, "false") == 0 || strcmp(value, "False") == 0 ||
        strcmp(value, "FALSE") == 0)
        return false;

    return true;
}

static inline const char *get_locale() {
    const char *locale = getenv("LC_ALL");
    if (!locale)
        locale = getenv("LC_CTYPE");
    if (!locale)
        locale = getenv("LANG");
    if (!locale)
        locale = "C";

    return locale;
}

static bool objectAcceptsInputMethod() {
    bool enabled = false;
    QObject *object = qApp->focusObject();
    if (object) {
        QInputMethodQueryEvent query(Qt::ImEnabled);
        QGuiApplication::sendEvent(object, &query);
        enabled = query.value(Qt::ImEnabled).toBool();
    }

    return enabled;
}

struct xkb_context *_xkb_context_new_helper() {
    struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (context) {
        xkb_context_set_log_level(context, XKB_LOG_LEVEL_CRITICAL);
    }

    return context;
}

QFcitxPlatformInputContext::QFcitxPlatformInputContext()
    : watcher_(new FcitxQtWatcher(
          QDBusConnection::connectToBus(QDBusConnection::SessionBus, "fcitx"),
          this)),
      cursorPos_(0), useSurroundingText_(false),
      syncMode_(get_boolean_env("FCITX_QT_USE_SYNC", false)), destroy_(false),
      xkbContext_(_xkb_context_new_helper()),
      xkbComposeTable_(xkbContext_ ? xkb_compose_table_new_from_locale(
                                         xkbContext_.data(), get_locale(),
                                         XKB_COMPOSE_COMPILE_NO_FLAGS)
                                   : 0),
      xkbComposeState_(xkbComposeTable_
                           ? xkb_compose_state_new(xkbComposeTable_.data(),
                                                   XKB_COMPOSE_STATE_NO_FLAGS)
                           : 0) {
    registerFcitxQtDBusTypes();
    watcher_->setWatchPortal(true);
    watcher_->watch();
}

QFcitxPlatformInputContext::~QFcitxPlatformInputContext() {
    destroy_ = true;
    watcher_->unwatch();
    cleanUp();
    delete watcher_;
}

void QFcitxPlatformInputContext::cleanUp() {
    icMap_.clear();

    if (!destroy_) {
        commitPreedit();
    }
}

bool QFcitxPlatformInputContext::isValid() const { return true; }

void QFcitxPlatformInputContext::invokeAction(QInputMethod::Action action,
                                              int cursorPosition) {
    if (action == QInputMethod::Click &&
        (cursorPosition <= 0 || cursorPosition >= preedit_.length())) {
        // qDebug() << action << cursorPosition;
        commitPreedit();
    }
}

void QFcitxPlatformInputContext::commitPreedit(QPointer<QObject> input) {
    if (!input)
        return;
    if (commitPreedit_.length() <= 0)
        return;
    QInputMethodEvent e;
    e.setCommitString(commitPreedit_);
    QCoreApplication::sendEvent(input, &e);
    commitPreedit_.clear();
    preeditList_.clear();
}

bool checkUtf8(const QByteArray &byteArray) {
    QTextCodec::ConverterState state;
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    const QString text =
        codec->toUnicode(byteArray.constData(), byteArray.size(), &state);
    return state.invalidChars == 0;
}

void QFcitxPlatformInputContext::reset() {
    commitPreedit();
    if (FcitxQtInputContextProxy *proxy = validIC()) {
        proxy->reset();
    }
    if (xkbComposeState_) {
        xkb_compose_state_reset(xkbComposeState_.data());
    }
    QPlatformInputContext::reset();
}

void QFcitxPlatformInputContext::update(Qt::InputMethodQueries queries) {
    QWindow *window = qApp->focusWindow();
    FcitxQtInputContextProxy *proxy = validICByWindow(window);
    if (!proxy)
        return;

    FcitxQtICData &data = *static_cast<FcitxQtICData *>(
        proxy->property("icData").value<void *>());

    QObject *input = qApp->focusObject();
    if (!input)
        return;

    QInputMethodQueryEvent query(queries);
    QGuiApplication::sendEvent(input, &query);

    if (queries & Qt::ImCursorRectangle) {
        cursorRectChanged();
    }

    if (queries & Qt::ImHints) {
        Qt::InputMethodHints hints =
            Qt::InputMethodHints(query.value(Qt::ImHints).toUInt());

#define CHECK_HINTS(_HINTS, _CAPABILITY)                                       \
    if (hints & _HINTS)                                                        \
        addCapability(data, fcitx::CapabilityFlag::_CAPABILITY);               \
    else                                                                       \
        removeCapability(data, fcitx::CapabilityFlag::_CAPABILITY);

        CHECK_HINTS(Qt::ImhHiddenText, Password)
        CHECK_HINTS(Qt::ImhSensitiveData, Sensitive)
        CHECK_HINTS(Qt::ImhNoAutoUppercase, NoAutoUpperCase)
        CHECK_HINTS(Qt::ImhPreferNumbers, Number)
        CHECK_HINTS(Qt::ImhPreferUppercase, Uppercase)
        CHECK_HINTS(Qt::ImhPreferLowercase, Lowercase)
        CHECK_HINTS(Qt::ImhNoPredictiveText, NoSpellCheck)
        CHECK_HINTS(Qt::ImhDigitsOnly, Digit)
        CHECK_HINTS(Qt::ImhFormattedNumbersOnly, Number)
        CHECK_HINTS(Qt::ImhUppercaseOnly, Uppercase)
        CHECK_HINTS(Qt::ImhLowercaseOnly, Lowercase)
        CHECK_HINTS(Qt::ImhDialableCharactersOnly, Dialable)
        CHECK_HINTS(Qt::ImhEmailCharactersOnly, Email)
        CHECK_HINTS(Qt::ImhPreferLatin, Alpha)
        CHECK_HINTS(Qt::ImhUrlCharactersOnly, Url)
        CHECK_HINTS(Qt::ImhMultiLine, Multiline)
    }

    bool setSurrounding = false;
    do {
        if (!useSurroundingText_)
            break;
        if (!((queries & Qt::ImSurroundingText) &&
              (queries & Qt::ImCursorPosition)))
            break;
        if (data.capability.test(fcitx::CapabilityFlag::Password) ||
            data.capability.test(fcitx::CapabilityFlag::Sensitive))
            break;
        QVariant var = query.value(Qt::ImSurroundingText);
        QVariant var1 = query.value(Qt::ImCursorPosition);
        QVariant var2 = query.value(Qt::ImAnchorPosition);
        if (!var.isValid() || !var1.isValid())
            break;
        QString text = var.toString();
/* we don't want to waste too much memory here */
#define SURROUNDING_THRESHOLD 4096
        if (text.length() < SURROUNDING_THRESHOLD) {
            if (checkUtf8(text.toUtf8())) {
                addCapability(data, fcitx::CapabilityFlag::SurroundingText);

                int cursor = var1.toInt();
                int anchor;
                if (var2.isValid())
                    anchor = var2.toInt();
                else
                    anchor = cursor;

                // adjust it to real character size
                QVector<uint> tempUCS4 = text.leftRef(cursor).toUcs4();
                cursor = tempUCS4.size();
                tempUCS4 = text.leftRef(anchor).toUcs4();
                anchor = tempUCS4.size();
                if (data.surroundingText != text) {
                    data.surroundingText = text;
                    proxy->setSurroundingText(text, cursor, anchor);
                } else {
                    if (data.surroundingAnchor != anchor ||
                        data.surroundingCursor != cursor)
                        proxy->setSurroundingTextPosition(cursor, anchor);
                }
                data.surroundingCursor = cursor;
                data.surroundingAnchor = anchor;
                setSurrounding = true;
            }
        }
        if (!setSurrounding) {
            data.surroundingAnchor = -1;
            data.surroundingCursor = -1;
            data.surroundingText = QString();
            removeCapability(data, fcitx::CapabilityFlag::SurroundingText);
        }
    } while (0);
}

void QFcitxPlatformInputContext::commit() { QPlatformInputContext::commit(); }

void QFcitxPlatformInputContext::setFocusObject(QObject *object) {
    Q_UNUSED(object);
    FcitxQtInputContextProxy *proxy = validICByWindow(lastWindow_);
    commitPreedit(lastObject_);
    if (proxy) {
        proxy->focusOut();
    }

    QWindow *window = qApp->focusWindow();
    lastWindow_ = window;
    lastObject_ = object;
    if (!window) {
        return;
    }
    proxy = validICByWindow(window);
    if (proxy)
        proxy->focusIn();
    else {
        createICData(window);
    }
}

void QFcitxPlatformInputContext::windowDestroyed(QObject *object) {
    /* access QWindow is not possible here, so we use our own map to do so */
    icMap_.erase(static_cast<QWindow *>(object));
    // qDebug() << "Window Destroyed and we destroy IC correctly, horray!";
}

void QFcitxPlatformInputContext::cursorRectChanged() {
    QWindow *inputWindow = qApp->focusWindow();
    if (!inputWindow)
        return;
    FcitxQtInputContextProxy *proxy = validICByWindow(inputWindow);
    if (!proxy)
        return;

    FcitxQtICData &data = *static_cast<FcitxQtICData *>(
        proxy->property("icData").value<void *>());

    QRect r = qApp->inputMethod()->cursorRectangle().toRect();
    if (!r.isValid())
        return;

    // not sure if this is necessary but anyway, qt's screen used to be buggy.
    if (!inputWindow->screen()) {
        return;
    }

    qreal scale = inputWindow->devicePixelRatio();
    auto screenGeometry = inputWindow->screen()->geometry();
    auto point = inputWindow->mapToGlobal(r.topLeft());
    auto native =
        (point - screenGeometry.topLeft()) * scale + screenGeometry.topLeft();
    QRect newRect(native, r.size() * scale);

    if (data.rect != newRect) {
        data.rect = newRect;
        proxy->setCursorRect(newRect.x(), newRect.y(), newRect.width(),
                             newRect.height());
    }
}

void QFcitxPlatformInputContext::createInputContextFinished(
    const QByteArray &uuid) {
    auto proxy = qobject_cast<FcitxQtInputContextProxy *>(sender());
    if (!proxy) {
        return;
    }
    auto w = static_cast<QWindow *>(proxy->property("wid").value<void *>());
    FcitxQtICData *data =
        static_cast<FcitxQtICData *>(proxy->property("icData").value<void *>());
    data->rect = QRect();

    if (proxy->isValid()) {
        QWindow *window = qApp->focusWindow();
        if (window && window == w) {
            proxy->focusIn();
            cursorRectChanged();
        }
        setFocusGroupForX11(uuid);
    }

    fcitx::CapabilityFlags flag;
    flag |= fcitx::CapabilityFlag::Preedit;
    flag |= fcitx::CapabilityFlag::FormattedPreedit;
    flag |= fcitx::CapabilityFlag::ClientUnfocusCommit;
    flag |= fcitx::CapabilityFlag::GetIMInfoOnFocus;
    useSurroundingText_ =
        get_boolean_env("FCITX_QT_ENABLE_SURROUNDING_TEXT", true);
    if (useSurroundingText_) {
        flag |= fcitx::CapabilityFlag::SurroundingText;
    }

    if (qApp && qApp->platformName() == "wayland") {
        flag |= fcitx::CapabilityFlag::RelativeRect;
    }

    addCapability(*data, flag, true);
}

void QFcitxPlatformInputContext::updateCapability(const FcitxQtICData &data) {
    if (!data.proxy || !data.proxy->isValid())
        return;

    QDBusPendingReply<void> result = data.proxy->setCapability(data.capability);
}

void QFcitxPlatformInputContext::commitString(const QString &str) {
    qDebug() << "COMMIT" << str;
    cursorPos_ = 0;
    preeditList_.clear();
    commitPreedit_.clear();
    QObject *input = qApp->focusObject();
    if (!input)
        return;

    QInputMethodEvent event;
    event.setCommitString(str);
    QCoreApplication::sendEvent(input, &event);
}

void QFcitxPlatformInputContext::updateFormattedPreedit(
    const FcitxQtFormattedPreeditList &preeditList, int cursorPos) {
    QObject *input = qApp->focusObject();
    if (!input)
        return;
    if (cursorPos == cursorPos_ && preeditList == preeditList_)
        return;
    preeditList_ = preeditList;
    cursorPos_ = cursorPos;
    QString str, commitStr;
    int pos = 0;
    QList<QInputMethodEvent::Attribute> attrList;
    Q_FOREACH (const FcitxQtFormattedPreedit &preedit, preeditList) {
        str += preedit.string();
        if (!(fcitx::TextFormatFlags(preedit.format()) &
              fcitx::TextFormatFlag::DontCommit))
            commitStr += preedit.string();
        QTextCharFormat format;
        if (fcitx::TextFormatFlags(preedit.format()) &
            fcitx::TextFormatFlag::Underline) {
            format.setUnderlineStyle(QTextCharFormat::DashUnderline);
        }
        if (fcitx::TextFormatFlags(preedit.format()) &
            fcitx::TextFormatFlag::Strike) {
            format.setFontStrikeOut(true);
        }
        if (fcitx::TextFormatFlags(preedit.format()) &
            fcitx::TextFormatFlag::Bold) {
            format.setFontWeight(QFont::Bold);
        }
        if (fcitx::TextFormatFlags(preedit.format()) &
            fcitx::TextFormatFlag::Italic) {
            format.setFontItalic(true);
        }
        if (fcitx::TextFormatFlags(preedit.format()) &
            fcitx::TextFormatFlag::HighLight) {
            QBrush brush;
            QPalette palette;
            palette = QGuiApplication::palette();
            format.setBackground(QBrush(
                QColor(palette.color(QPalette::Active, QPalette::Highlight))));
            format.setForeground(QBrush(QColor(
                palette.color(QPalette::Active, QPalette::HighlightedText))));
        }
        attrList.append(
            QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat, pos,
                                         preedit.string().length(), format));
        pos += preedit.string().length();
    }

    QByteArray array = str.toUtf8();
    array.truncate(cursorPos);
    cursorPos = QString::fromUtf8(array).length();

    attrList.append(QInputMethodEvent::Attribute(QInputMethodEvent::Cursor,
                                                 cursorPos, 1, 0));
    preedit_ = str;
    commitPreedit_ = commitStr;
    QInputMethodEvent event(str, attrList);
    QCoreApplication::sendEvent(input, &event);
    update(Qt::ImCursorRectangle);
}

void QFcitxPlatformInputContext::deleteSurroundingText(int offset,
                                                       uint _nchar) {
    QObject *input = qApp->focusObject();
    if (!input)
        return;

    QInputMethodEvent event;

    FcitxQtInputContextProxy *proxy =
        qobject_cast<FcitxQtInputContextProxy *>(sender());
    if (!proxy) {
        return;
    }

    FcitxQtICData *data =
        static_cast<FcitxQtICData *>(proxy->property("icData").value<void *>());
    QVector<uint> ucsText = data->surroundingText.toUcs4();

    int cursor = data->surroundingCursor;
    // make nchar signed so we are safer
    int nchar = _nchar;
    // Qt's reconvert semantics is different from gtk's. It doesn't count the
    // current
    // selection. Discard selection from nchar.
    if (data->surroundingAnchor < data->surroundingCursor) {
        nchar -= data->surroundingCursor - data->surroundingAnchor;
        offset += data->surroundingCursor - data->surroundingAnchor;
        cursor = data->surroundingAnchor;
    } else if (data->surroundingAnchor > data->surroundingCursor) {
        nchar -= data->surroundingAnchor - data->surroundingCursor;
        cursor = data->surroundingCursor;
    }

    // validates
    if (nchar >= 0 && cursor + offset >= 0 &&
        cursor + offset + nchar < ucsText.size()) {
        // order matters
        QVector<uint> replacedChars = ucsText.mid(cursor + offset, nchar);
        nchar = QString::fromUcs4(replacedChars.data(), replacedChars.size())
                    .size();

        int start, len;
        if (offset >= 0) {
            start = cursor;
            len = offset;
        } else {
            start = cursor;
            len = -offset;
        }

        QVector<uint> prefixedChars = ucsText.mid(start, len);
        offset = QString::fromUcs4(prefixedChars.data(), prefixedChars.size())
                     .size() *
                 (offset >= 0 ? 1 : -1);
        event.setCommitString("", offset, nchar);
        QCoreApplication::sendEvent(input, &event);
    }
}

void QFcitxPlatformInputContext::forwardKey(uint keyval, uint state,
                                            bool type) {
    QObject *input = qApp->focusObject();
    if (input != nullptr) {
        key_filtered = true;
        QKeyEvent *keyevent = createKeyEvent(keyval, state, type);

        QCoreApplication::sendEvent(input, keyevent);
        delete keyevent;
        key_filtered = false;
    }
}

void QFcitxPlatformInputContext::updateCurrentIM(const QString &name,
                                                 const QString &uniqueName,
                                                 const QString &langCode) {
    Q_UNUSED(name);
    Q_UNUSED(uniqueName);
    QLocale newLocale(langCode);
    if (locale_ != newLocale) {
        locale_ = newLocale;
        emitLocaleChanged();
    }
}

QLocale QFcitxPlatformInputContext::locale() const { return locale_; }

bool QFcitxPlatformInputContext::hasCapability(Capability) const {
    return true;
}

void QFcitxPlatformInputContext::createICData(QWindow *w) {
    auto iter = icMap_.find(w);
    if (iter == icMap_.end()) {
        auto result =
            icMap_.emplace(std::piecewise_construct, std::forward_as_tuple(w),
                           std::forward_as_tuple(watcher_));
        connect(w, &QObject::destroyed, this,
                &QFcitxPlatformInputContext::windowDestroyed);
        iter = result.first;
        auto &data = iter->second;

        if (QGuiApplication::platformName() == QLatin1String("xcb")) {
            data.proxy->setDisplay("x11:");
        } else if (QGuiApplication::platformName() ==
                   QLatin1String("wayland")) {
            data.proxy->setDisplay("wayland:");
        }
        data.proxy->setProperty("wid",
                                qVariantFromValue(static_cast<void *>(w)));
        data.proxy->setProperty("icData",
                                qVariantFromValue(static_cast<void *>(&data)));
        connect(data.proxy, &FcitxQtInputContextProxy::inputContextCreated,
                this, &QFcitxPlatformInputContext::createInputContextFinished);
        connect(data.proxy, &FcitxQtInputContextProxy::commitString, this,
                &QFcitxPlatformInputContext::commitString);
        connect(data.proxy, &FcitxQtInputContextProxy::forwardKey, this,
                &QFcitxPlatformInputContext::forwardKey);
        connect(data.proxy, &FcitxQtInputContextProxy::updateFormattedPreedit,
                this, &QFcitxPlatformInputContext::updateFormattedPreedit);
        connect(data.proxy, &FcitxQtInputContextProxy::deleteSurroundingText,
                this, &QFcitxPlatformInputContext::deleteSurroundingText);
        connect(data.proxy, &FcitxQtInputContextProxy::currentIM, this,
                &QFcitxPlatformInputContext::updateCurrentIM);
    }
}

QKeyEvent *QFcitxPlatformInputContext::createKeyEvent(uint keyval, uint _state,
                                                      bool isRelease) {
    Qt::KeyboardModifiers qstate = Qt::NoModifier;

    fcitx::KeyStates state(_state);

    int count = 1;
    if (state & fcitx::KeyState::Alt) {
        qstate |= Qt::AltModifier;
        count++;
    }

    if (state & fcitx::KeyState::Shift) {
        qstate |= Qt::ShiftModifier;
        count++;
    }

    if (state & fcitx::KeyState::Ctrl) {
        qstate |= Qt::ControlModifier;
        count++;
    }

    auto unicode = xkb_keysym_to_utf32(keyval);
    QString text;
    if (unicode) {
        text = QString::fromUcs4(&unicode, 1);
    }

    int key = keysymToQtKey(keyval, text);

    QKeyEvent *keyevent =
        new QKeyEvent(isRelease ? (QEvent::KeyRelease) : (QEvent::KeyPress),
                      key, qstate, 0, keyval, _state, text, false, count);

    return keyevent;
}

bool QFcitxPlatformInputContext::filterEvent(const QEvent *event) {
    do {
        if (event->type() != QEvent::KeyPress &&
            event->type() != QEvent::KeyRelease) {
            break;
        }

        const QKeyEvent *keyEvent = static_cast<const QKeyEvent *>(event);
        quint32 keyval = keyEvent->nativeVirtualKey();
        quint32 keycode = keyEvent->nativeScanCode();
        quint32 state = keyEvent->nativeModifiers();
        bool isRelease = keyEvent->type() == QEvent::KeyRelease;

        if (key_filtered) {
            break;
        }

        if (!inputMethodAccepted() && !objectAcceptsInputMethod())
            break;

        QObject *input = qApp->focusObject();

        if (!input) {
            break;
        }

        FcitxQtInputContextProxy *proxy = validICByWindow(qApp->focusWindow());

        if (!proxy) {
            if (filterEventFallback(keyval, keycode, state, isRelease)) {
                return true;
            } else {
                break;
            }
        }

        proxy->focusIn();
        update(Qt::ImHints);

        auto reply =
            proxy->processKeyEvent(keyval, keycode, state, isRelease,
                                   QDateTime::currentDateTime().toTime_t());

        if (Q_UNLIKELY(syncMode_)) {
            reply.waitForFinished();

            if (reply.isError() || !reply.value()) {
                if (filterEventFallback(keyval, keycode, state, isRelease)) {
                    return true;
                } else {
                    break;
                }
            } else {
                update(Qt::ImCursorRectangle);
                return true;
            }
        } else {
            ProcessKeyWatcher *watcher = new ProcessKeyWatcher(
                *keyEvent, qApp->focusWindow(), reply, proxy);
            connect(watcher, &QDBusPendingCallWatcher::finished, this,
                    &QFcitxPlatformInputContext::processKeyEventFinished);
            return true;
        }
    } while (0);
    return QPlatformInputContext::filterEvent(event);
}

void QFcitxPlatformInputContext::processKeyEventFinished(
    QDBusPendingCallWatcher *w) {
    ProcessKeyWatcher *watcher = static_cast<ProcessKeyWatcher *>(w);
    QDBusPendingReply<bool> result(*watcher);
    bool filtered = false;

    QWindow *window = watcher->window();
    // if window is already destroyed, we can only throw this event away.
    if (!window) {
        delete watcher;
        return;
    }

    const QKeyEvent &keyEvent = watcher->keyEvent();

    // use same variable name as in QXcbKeyboard::handleKeyEvent
    QEvent::Type type = keyEvent.type();
    int qtcode = keyEvent.key();
    Qt::KeyboardModifiers modifiers = keyEvent.modifiers();
    quint32 code = keyEvent.nativeScanCode();
    quint32 sym = keyEvent.nativeVirtualKey();
    quint32 state = keyEvent.nativeModifiers();
    QString string = keyEvent.text();
    bool isAutoRepeat = keyEvent.isAutoRepeat();
    ulong time = keyEvent.timestamp();

    if (result.isError() || !result.value()) {
        filtered =
            filterEventFallback(sym, code, state, type == QEvent::KeyRelease);
    } else {
        filtered = true;
    }

    if (!watcher->isError()) {
        update(Qt::ImCursorRectangle);
    }

    if (!filtered) {
        // copied from QXcbKeyboard::handleKeyEvent()
        if (type == QEvent::KeyPress && qtcode == Qt::Key_Menu) {
            QPoint globalPos, pos;
            if (window->screen()) {
                globalPos = window->screen()->handle()->cursor()->pos();
                pos = window->mapFromGlobal(globalPos);
            }
            QWindowSystemInterface::handleContextMenuEvent(
                window, false, pos, globalPos, modifiers);
        }
        QWindowSystemInterface::handleExtendedKeyEvent(
            window, time, type, qtcode, modifiers, code, sym, state, string,
            isAutoRepeat);
    }

    delete watcher;
}

bool QFcitxPlatformInputContext::filterEventFallback(uint keyval, uint keycode,
                                                     uint state,
                                                     bool isRelease) {
    Q_UNUSED(keycode);
    if (processCompose(keyval, state, isRelease)) {
        return true;
    }
    return false;
}

FcitxQtInputContextProxy *QFcitxPlatformInputContext::validIC() {
    if (icMap_.empty()) {
        return nullptr;
    }
    QWindow *window = qApp->focusWindow();
    return validICByWindow(window);
}

FcitxQtInputContextProxy *
QFcitxPlatformInputContext::validICByWindow(QWindow *w) {
    if (!w) {
        return nullptr;
    }

    if (icMap_.empty()) {
        return nullptr;
    }
    auto iter = icMap_.find(w);
    if (iter == icMap_.end())
        return nullptr;
    auto &data = iter->second;
    if (!data.proxy || !data.proxy->isValid()) {
        return nullptr;
    }
    return data.proxy;
}

bool QFcitxPlatformInputContext::processCompose(uint keyval, uint state,
                                                bool isRelease) {
    Q_UNUSED(state);

    if (!xkbComposeTable_ || isRelease)
        return false;

    struct xkb_compose_state *xkbComposeState = xkbComposeState_.data();

    enum xkb_compose_feed_result result =
        xkb_compose_state_feed(xkbComposeState, keyval);
    if (result == XKB_COMPOSE_FEED_IGNORED) {
        return false;
    }

    enum xkb_compose_status status =
        xkb_compose_state_get_status(xkbComposeState);
    if (status == XKB_COMPOSE_NOTHING) {
        return 0;
    } else if (status == XKB_COMPOSE_COMPOSED) {
        char buffer[] = {'\0', '\0', '\0', '\0', '\0', '\0', '\0'};
        int length =
            xkb_compose_state_get_utf8(xkbComposeState, buffer, sizeof(buffer));
        xkb_compose_state_reset(xkbComposeState);
        if (length != 0) {
            commitString(QString::fromUtf8(buffer));
        }

    } else if (status == XKB_COMPOSE_CANCELLED) {
        xkb_compose_state_reset(xkbComposeState);
    }

    return true;
}
} // namespace fcitx
