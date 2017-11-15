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

#include <QApplication>
#include <QDBusConnection>
#include <QDateTime>
#include <QKeyEvent>
#include <QPalette>
#include <QTextCharFormat>
#include <QTextCodec>
#include <QWidget>
#include <QX11Info>

#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include "qtkey.h"

#include "fcitxqtinputcontextproxy.h"
#include "fcitxqtwatcher.h"
#include "qfcitxinputcontext.h"

#include <fcitx-utils/key.h>
#include <fcitx-utils/textformatflags.h>
#include <fcitx-utils/utf8.h>

#include <X11/Xlib.h>
#include <memory>
#undef KeyPress
#undef KeyRelease
#undef FocusIn
#undef FocusOut

namespace fcitx {

void setFocusGroupForX11(const QByteArray &uuid) {
    if (uuid.size() != 16) {
        return;
    }

    Display *xdisplay = QX11Info::display();
    if (!xdisplay) {
        return;
    }

    Atom atom = XInternAtom(xdisplay, "_FCITX_SERVER", False);
    if (!atom) {
        return;
    }
    Window window = XGetSelectionOwner(xdisplay, atom);
    if (!window) {
        return;
    }
    XEvent ev;

    memset(&ev, 0, sizeof(ev));
    ev.xclient.type = ClientMessage;
    ev.xclient.window = window;
    ev.xclient.message_type = atom;
    ev.xclient.format = 8;
    memcpy(ev.xclient.data.b, uuid.constData(), 16);

    XSendEvent(xdisplay, window, False, NoEventMask, &ev);
    XSync(xdisplay, False);
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

struct xkb_context *_xkb_context_new_helper() {
    struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (context) {
        xkb_context_set_log_level(context, XKB_LOG_LEVEL_CRITICAL);
    }

    return context;
}

QFcitxInputContext::QFcitxInputContext()
    : m_watcher(new FcitxQtWatcher(QDBusConnection::sessionBus(), this)),
      m_cursorPos(0), m_useSurroundingText(false),
      m_syncMode(get_boolean_env("FCITX_QT_USE_SYNC", false)), m_destroy(false),
      m_xkbContext(_xkb_context_new_helper()),
      m_xkbComposeTable(m_xkbContext ? xkb_compose_table_new_from_locale(
                                           m_xkbContext.data(), get_locale(),
                                           XKB_COMPOSE_COMPILE_NO_FLAGS)
                                     : 0),
      m_xkbComposeState(m_xkbComposeTable
                            ? xkb_compose_state_new(m_xkbComposeTable.data(),
                                                    XKB_COMPOSE_STATE_NO_FLAGS)
                            : 0) {
    registerFcitxQtDBusTypes();
    m_watcher->setWatchPortal(true);
    m_watcher->watch();
}

QFcitxInputContext::~QFcitxInputContext() {
    m_destroy = true;
    m_watcher->unwatch();
    cleanUp();
    delete m_watcher;
}

void QFcitxInputContext::cleanUp() {
    m_icMap.clear();

    if (!m_destroy) {
        commitPreedit();
    }
}

void QFcitxInputContext::mouseHandler(int cursorPosition, QMouseEvent *event) {
    if (event->type() == QEvent::MouseButtonPress &&
        (cursorPosition <= 0 || cursorPosition >= m_preedit.length())) {
        // qDebug() << action << cursorPosition;
        commitPreedit();
    }
}

void QFcitxInputContext::commitPreedit(QPointer<QWidget> input) {
    if (!input)
        return;
    if (m_commitPreedit.length() <= 0)
        return;
    QInputMethodEvent e;
    e.setCommitString(m_commitPreedit);
    QCoreApplication::sendEvent(input, &e);
    m_commitPreedit.clear();

    m_preeditList.clear();
}

bool checkUtf8(const QByteArray &byteArray) {
    QTextCodec::ConverterState state;
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    const QString text =
        codec->toUnicode(byteArray.constData(), byteArray.size(), &state);
    return state.invalidChars == 0;
}

void QFcitxInputContext::reset() {
    commitPreedit();
    if (FcitxQtInputContextProxy *proxy = validIC()) {
        proxy->reset();
    }
    if (m_xkbComposeState) {
        xkb_compose_state_reset(m_xkbComposeState.data());
    }
}

void QFcitxInputContext::update() {
    QWidget *window = qApp->focusWidget();
    FcitxQtInputContextProxy *proxy = validICByWindow(window);
    if (!proxy)
        return;

    FcitxQtICData &data = *static_cast<FcitxQtICData *>(
        proxy->property("icData").value<void *>());

    QWidget *input = qApp->focusWidget();
    if (!input)
        return;

    cursorRectChanged();

    if (true) {
        Qt::InputMethodHints hints = input->inputMethodHints();

#define CHECK_HINTS(_HINTS, _CAPABILITY)                                       \
    if (hints & _HINTS)                                                        \
        addCapability(data, fcitx::CapabilityFlag::_CAPABILITY);               \
    else                                                                       \
        removeCapability(data, fcitx::CapabilityFlag::_CAPABILITY);

        CHECK_HINTS(Qt::ImhHiddenText, Password)
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
        CHECK_HINTS(Qt::ImhUrlCharactersOnly, Url)
    }

    bool setSurrounding = false;
    do {
        if (!m_useSurroundingText)
            break;
        if (data.capability.test(fcitx::CapabilityFlag::Password) ||
            data.capability.test(fcitx::CapabilityFlag::Sensitive))
            break;
        QVariant var = input->inputMethodQuery(Qt::ImSurroundingText);
        QVariant var1 = input->inputMethodQuery(Qt::ImCursorPosition);
        QVariant var2 = input->inputMethodQuery(Qt::ImAnchorPosition);
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
            data.surroundingText = QString::null;
            removeCapability(data, fcitx::CapabilityFlag::SurroundingText);
        }
    } while (0);
}

void QFcitxInputContext::setFocusWidget(QWidget *object) {
    Q_UNUSED(object);
    FcitxQtInputContextProxy *proxy = validICByWindow(m_lastWindow);
    if (proxy) {
        proxy->focusOut();
    }

    QWidget *window = qApp->focusWidget();
    m_lastWindow = window;
    if (!window) {
        return;
    }
    proxy = validICByWindow(window);
    if (proxy)
        proxy->focusIn();
    else {
        createICData(window);
    }
    QInputContext::setFocusWidget(object);
}

void QFcitxInputContext::widgetDestroyed(QWidget *w) {
    QInputContext::widgetDestroyed(w);

    m_icMap.erase(w);
}

void QFcitxInputContext::windowDestroyed(QObject *object) {
    /* access QWindow is not possible here, so we use our own map to do so */
    m_icMap.erase(reinterpret_cast<QWidget *>(object));
    // qDebug() << "Window Destroyed and we destroy IC correctly, horray!";
}

void QFcitxInputContext::cursorRectChanged() {
    QWidget *inputWindow = qApp->focusWidget();
    if (!inputWindow)
        return;
    FcitxQtInputContextProxy *proxy = validICByWindow(inputWindow);
    if (!proxy)
        return;

    FcitxQtICData &data = *static_cast<FcitxQtICData *>(
        proxy->property("icData").value<void *>());

    QRect r = inputWindow->inputMethodQuery(Qt::ImMicroFocus).toRect();

    auto point = inputWindow->mapToGlobal(r.topLeft());
    QRect newRect(point, r.size());

    if (data.rect != newRect) {
        data.rect = newRect;
        proxy->setCursorRect(newRect.x(), newRect.y(), newRect.width(),
                             newRect.height());
    }
}

void QFcitxInputContext::createInputContextFinished(const QByteArray &uuid) {
    auto proxy = qobject_cast<FcitxQtInputContextProxy *>(sender());
    if (!proxy) {
        return;
    }
    auto w =
        reinterpret_cast<QWidget *>(proxy->property("wid").value<void *>());
    FcitxQtICData *data =
        static_cast<FcitxQtICData *>(proxy->property("icData").value<void *>());
    data->rect = QRect();

    if (proxy->isValid()) {
        QWidget *window = qApp->focusWidget();
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
    m_useSurroundingText =
        get_boolean_env("FCITX_QT_ENABLE_SURROUNDING_TEXT", true);
    if (m_useSurroundingText)
        flag |= fcitx::CapabilityFlag::SurroundingText;

    addCapability(*data, flag, true);
}

void QFcitxInputContext::updateCapability(const FcitxQtICData &data) {
    if (!data.proxy || !data.proxy->isValid())
        return;

    QDBusPendingReply<void> result = data.proxy->setCapability(data.capability);
}

void QFcitxInputContext::commitString(const QString &str) {
    m_cursorPos = 0;
    m_preeditList.clear();
    m_commitPreedit.clear();
    QWidget *input = qApp->focusWidget();
    if (!input)
        return;

    QInputMethodEvent event;
    event.setCommitString(str);
    QCoreApplication::sendEvent(input, &event);
}

void QFcitxInputContext::updateFormattedPreedit(
    const FcitxQtFormattedPreeditList &preeditList, int cursorPos) {
    QWidget *input = qApp->focusWidget();
    if (!input)
        return;
    if (cursorPos == m_cursorPos && preeditList == m_preeditList)
        return;
    m_preeditList = preeditList;
    m_cursorPos = cursorPos;
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
            palette = QApplication::palette();
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
    m_preedit = str;
    m_commitPreedit = commitStr;
    QInputMethodEvent event(str, attrList);
    QCoreApplication::sendEvent(input, &event);
    update();
}

void QFcitxInputContext::deleteSurroundingText(int offset, uint _nchar) {
    QWidget *input = qApp->focusWidget();
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

void QFcitxInputContext::forwardKey(uint keyval, uint state, bool type) {
    QWidget *input = qApp->focusWidget();
    if (input != nullptr) {
        key_filtered = true;
        QKeyEvent *keyevent = createKeyEvent(keyval, state, type);
        QCoreApplication::sendEvent(input, keyevent);
        delete keyevent;
        key_filtered = false;
    }
}

void QFcitxInputContext::createICData(QWidget *w) {
    auto iter = m_icMap.find(w);
    if (iter == m_icMap.end()) {
        auto result =
            m_icMap.emplace(std::piecewise_construct, std::forward_as_tuple(w),
                            std::forward_as_tuple(m_watcher));
        iter = result.first;
        auto &data = iter->second;

        data.proxy->setDisplay("x11:");
        data.proxy->setProperty("wid",
                                qVariantFromValue(static_cast<void *>(w)));
        data.proxy->setProperty("icData",
                                qVariantFromValue(static_cast<void *>(&data)));
        connect(data.proxy, SIGNAL(inputContextCreated(QByteArray)), this,
                SLOT(createInputContextFinished(QByteArray)));
        connect(data.proxy, SIGNAL(commitString(QString)), this,
                SLOT(commitString(QString)));
        connect(data.proxy, SIGNAL(forwardKey(uint, uint, bool)), this,
                SLOT(forwardKey(uint, uint, bool)));
        connect(data.proxy, SIGNAL(updateFormattedPreedit(
                                FcitxQtFormattedPreeditList, int)),
                this,
                SLOT(updateFormattedPreedit(FcitxQtFormattedPreeditList, int)));
        connect(data.proxy, SIGNAL(deleteSurroundingText(int, uint)), this,
                SLOT(deleteSurroundingText(int, uint)));
    }
}

QKeyEvent *QFcitxInputContext::createKeyEvent(uint keyval, uint _state,
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
                      key, qstate, text, false, count);

    return keyevent;
}

bool QFcitxInputContext::filterEvent(const QEvent *event) {
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

        QWidget *input = qApp->focusWidget();

        if (!input) {
            break;
        }

        FcitxQtInputContextProxy *proxy = validICByWindow(input);

        if (!proxy) {
            if (filterEventFallback(keyval, keycode, state, isRelease)) {
                return true;
            } else {
                break;
            }
        }

        proxy->focusIn();
        update();

        auto reply =
            proxy->processKeyEvent(keyval, keycode, state, isRelease,
                                   QDateTime::currentDateTime().toTime_t());

        if (Q_UNLIKELY(m_syncMode)) {
            reply.waitForFinished();

            if (reply.isError() || !reply.value()) {
                if (filterEventFallback(keyval, keycode, state, isRelease)) {
                    return true;
                } else {
                    break;
                }
            } else {
                update();
                return true;
            }
        } else {
            ProcessKeyWatcher *watcher = new ProcessKeyWatcher(
                *keyEvent, qApp->focusWidget(), reply, proxy);
            connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher *)), this,
                    SLOT(processKeyEventFinished(QDBusPendingCallWatcher *)));
            return true;
        }
    } while (0);
    return QInputContext::filterEvent(event);
}

void QFcitxInputContext::processKeyEventFinished(QDBusPendingCallWatcher *w) {
    ProcessKeyWatcher *watcher = static_cast<ProcessKeyWatcher *>(w);
    QDBusPendingReply<bool> result(*watcher);
    bool filtered = false;

    QWidget *window = watcher->window();
    // if window is already destroyed, we can only throw this event away.
    if (!window) {
        delete watcher;
        return;
    }

    QKeyEvent &keyEvent = watcher->keyEvent();

    // use same variable name as in QXcbKeyboard::handleKeyEvent
    QEvent::Type type = keyEvent.type();
    quint32 code = keyEvent.nativeScanCode();
    quint32 sym = keyEvent.nativeVirtualKey();
    quint32 state = keyEvent.nativeModifiers();
    QString string = keyEvent.text();

    if (result.isError() || !result.value()) {
        filtered =
            filterEventFallback(sym, code, state, type == QEvent::KeyRelease);
    } else {
        filtered = true;
    }

    if (!watcher->isError()) {
        update();
    }

    if (!filtered) {
        key_filtered = true;
        QCoreApplication::sendEvent(window, &keyEvent);
        key_filtered = false;
    }

    delete watcher;
}

bool QFcitxInputContext::filterEventFallback(uint keyval, uint keycode,
                                             uint state, bool isRelease) {
    Q_UNUSED(keycode);
    if (processCompose(keyval, state, isRelease)) {
        return true;
    }
    return false;
}

FcitxQtInputContextProxy *QFcitxInputContext::validIC() {
    if (m_icMap.empty()) {
        return nullptr;
    }
    QWidget *window = qApp->focusWidget();
    return validICByWindow(window);
}

FcitxQtInputContextProxy *QFcitxInputContext::validICByWindow(QWidget *w) {
    if (!w) {
        return nullptr;
    }

    if (m_icMap.empty()) {
        return nullptr;
    }
    auto iter = m_icMap.find(w);
    if (iter == m_icMap.end())
        return nullptr;
    auto &data = iter->second;
    if (!data.proxy || !data.proxy->isValid()) {
        return nullptr;
    }
    return data.proxy;
}

bool QFcitxInputContext::processCompose(uint keyval, uint state,
                                        bool isRelease) {
    Q_UNUSED(state);

    if (!m_xkbComposeTable || isRelease)
        return false;

    struct xkb_compose_state *xkbComposeState = m_xkbComposeState.data();

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

QString QFcitxInputContext::identifierName() { return "fcitx5"; }

QString QFcitxInputContext::language() { return ""; }
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
