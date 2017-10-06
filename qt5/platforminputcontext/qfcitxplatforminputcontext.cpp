/*
 * Copyright (C) 2011~2017 by CSSlayer
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

#include <QDBusConnection>
#include <QGuiApplication>
#include <QInputMethod>
#include <QKeyEvent>
#include <QPalette>
#include <QTextCharFormat>
#include <QWindow>
#include <QX11Info>
#include <qpa/qplatformcursor.h>
#include <qpa/qplatformscreen.h>
#include <qpa/qwindowsysteminterface.h>

#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include "keyserver_x11.h"

#include "fcitxqtconnection.h"
#include "fcitxqtinputcontextproxy.h"
#include "fcitxqtinputmethodproxy.h"
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
XCBReply<T> makeXCBReply(T *ptr) {
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

struct xkb_context *_xkb_context_new_helper() {
    struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (context) {
        xkb_context_set_log_level(context, XKB_LOG_LEVEL_CRITICAL);
    }

    return context;
}

QFcitxPlatformInputContext::QFcitxPlatformInputContext()
    : m_connection(new FcitxQtConnection(this)), m_improxy(nullptr),
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
    FcitxQtFormattedPreedit::registerMetaType();
    FcitxQtInputContextArgument::registerMetaType();

    connect(m_connection, &FcitxQtConnection::connected, this,
            &QFcitxPlatformInputContext::connected);
    connect(m_connection, &FcitxQtConnection::disconnected, this,
            &QFcitxPlatformInputContext::cleanUp);

    m_connection->startConnection();
}

QFcitxPlatformInputContext::~QFcitxPlatformInputContext() {
    m_destroy = true;
    cleanUp();
}

void QFcitxPlatformInputContext::connected() {
    if (!m_connection->isConnected())
        return;

    // qDebug() << "create Input Context" << m_connection->name();
    if (m_improxy) {
        delete m_improxy;
        m_improxy = nullptr;
    }
    m_improxy = new FcitxQtInputMethodProxy(m_connection->serviceName(),
                                            QLatin1String("/inputmethod"),
                                            *m_connection->connection(), this);

    QWindow *w = qApp->focusWindow();
    if (w)
        createICData(w);
}

void QFcitxPlatformInputContext::cleanUp() {
    m_icMap.clear();

    if (m_improxy) {
        delete m_improxy;
        m_improxy = nullptr;
    }

    if (!m_destroy) {
        commitPreedit();
    }
}

bool QFcitxPlatformInputContext::isValid() const { return true; }

void QFcitxPlatformInputContext::invokeAction(QInputMethod::Action action,
                                              int cursorPosition) {
    if (action == QInputMethod::Click &&
        (cursorPosition <= 0 || cursorPosition >= m_preedit.length())) {
        // qDebug() << action << cursorPosition;
        commitPreedit();
    }
}

void QFcitxPlatformInputContext::commitPreedit(QPointer<QObject> input) {
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

void QFcitxPlatformInputContext::reset() {
    commitPreedit();
    FcitxQtInputContextProxy *proxy = validIC();
    if (proxy)
        proxy->Reset();
    if (m_xkbComposeState) {
        xkb_compose_state_reset(m_xkbComposeState.data());
    }
    QPlatformInputContext::reset();
}

void QFcitxPlatformInputContext::update(Qt::InputMethodQueries queries) {
    QWindow *window = qApp->focusWindow();
    FcitxQtInputContextProxy *proxy = validICByWindow(window);
    if (!proxy)
        return;

    auto &data = m_icMap[window];

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
    }

    bool setSurrounding = false;
    do {
        if (!m_useSurroundingText)
            break;
        if (!((queries & Qt::ImSurroundingText) &&
              (queries & Qt::ImCursorPosition)))
            break;
        if (data.capability.test(fcitx::CapabilityFlag::Password))
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
            if (fcitx::utf8::validate(text.toUtf8())) {
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
                    proxy->SetSurroundingText(text, cursor, anchor);
                } else {
                    if (data.surroundingAnchor != anchor ||
                        data.surroundingCursor != cursor)
                        proxy->SetSurroundingTextPosition(cursor, anchor);
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

void QFcitxPlatformInputContext::commit() { QPlatformInputContext::commit(); }

void QFcitxPlatformInputContext::setFocusObject(QObject *object) {
    Q_UNUSED(object);
    FcitxQtInputContextProxy *proxy = validICByWindow(m_lastWindow);
    commitPreedit(m_lastObject);
    if (proxy) {
        proxy->FocusOut();
    }

    QWindow *window = qApp->focusWindow();
    m_lastWindow = window;
    m_lastObject = object;
    if (!window) {
        return;
    }
    proxy = validICByWindow(window);
    if (proxy)
        proxy->FocusIn();
    else {
        createICData(window);
    }
}

void QFcitxPlatformInputContext::windowDestroyed(QObject *object) {
    /* access QWindow is not possible here, so we use our own map to do so */
    m_icMap.erase(reinterpret_cast<QWindow *>(object));
    // qDebug() << "Window Destroyed and we destroy IC correctly, horray!";
}

void QFcitxPlatformInputContext::cursorRectChanged() {
    QWindow *inputWindow = qApp->focusWindow();
    if (!inputWindow)
        return;
    FcitxQtInputContextProxy *proxy = validICByWindow(inputWindow);
    if (!proxy)
        return;

    auto &data = m_icMap[inputWindow];

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
        proxy->SetCursorRect(newRect.x(), newRect.y(), newRect.width(),
                             newRect.height());
    }
}

void QFcitxPlatformInputContext::createInputContext(QWindow *w) {
    if (!m_connection->isConnected())
        return;

    // qDebug() << "create Input Context" << m_connection->connection()->name();

    if (!m_improxy) {
        m_improxy = new FcitxQtInputMethodProxy(
            m_connection->serviceName(), QLatin1String("/inputmethod"),
            *m_connection->connection(), this);
    }

    if (!m_improxy->isValid())
        return;

    QFileInfo info(QCoreApplication::applicationFilePath());
    FcitxQtInputContextArgumentList args;
    args << FcitxQtInputContextArgument("program", info.fileName());
    auto result = m_improxy->CreateInputContext(args);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(result);
    watcher->setProperty("wid", qVariantFromValue(static_cast<void *>(w)));
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            &QFcitxPlatformInputContext::createInputContextFinished);
}

void QFcitxPlatformInputContext::createInputContextFinished(
    QDBusPendingCallWatcher *watcher) {
    auto w =
        reinterpret_cast<QWindow *>(watcher->property("wid").value<void *>());
    auto iter = m_icMap.find(w);
    if (iter == m_icMap.end()) {
        return;
    }

    auto &data = iter->second;

    QDBusPendingReply<QDBusObjectPath, QByteArray> result = *watcher;

    do {
        if (result.isError()) {
            break;
        }

        if (!m_connection->isConnected())
            break;

        auto objectPath = result.argumentAt<0>();
        if (data.proxy) {
            delete data.proxy;
        }
        data.proxy = new FcitxQtInputContextProxy(
            m_connection->serviceName(), objectPath.path(),
            *m_connection->connection(), this);
        data.proxy->setProperty("icData",
                                qVariantFromValue(static_cast<void *>(&data)));
        connect(data.proxy, &FcitxQtInputContextProxy::CommitString, this,
                &QFcitxPlatformInputContext::commitString);
        connect(data.proxy, &FcitxQtInputContextProxy::ForwardKey, this,
                &QFcitxPlatformInputContext::forwardKey);
        connect(data.proxy, &FcitxQtInputContextProxy::UpdateFormattedPreedit,
                this, &QFcitxPlatformInputContext::updateFormattedPreedit);
        connect(data.proxy, &FcitxQtInputContextProxy::DeleteSurroundingText,
                this, &QFcitxPlatformInputContext::deleteSurroundingText);
        connect(data.proxy, &FcitxQtInputContextProxy::CurrentIM, this,
                &QFcitxPlatformInputContext::updateCurrentIM);

        if (data.proxy->isValid()) {
            QWindow *window = qApp->focusWindow();
            if (window && window == w)
                data.proxy->FocusIn();
        }

        setFocusGroupForX11(result.argumentAt<1>());

        fcitx::CapabilityFlags flag;
        flag |= fcitx::CapabilityFlag::Preedit;
        flag |= fcitx::CapabilityFlag::FormattedPreedit;
        flag |= fcitx::CapabilityFlag::ClientUnfocusCommit;
        flag |= fcitx::CapabilityFlag::GetIMInfoOnFocus;
        m_useSurroundingText =
            get_boolean_env("FCITX_QT_ENABLE_SURROUNDING_TEXT", true);
        if (m_useSurroundingText)
            flag |= fcitx::CapabilityFlag::SurroundingText;

        addCapability(data, flag, true);
    } while (0);
    delete watcher;
}

void QFcitxPlatformInputContext::updateCapability(const FcitxQtICData &data) {
    if (!data.proxy || !data.proxy->isValid())
        return;

    QDBusPendingReply<void> result =
        data.proxy->SetCapability((uint)data.capability);
}

void QFcitxPlatformInputContext::commitString(const QString &str) {
    m_cursorPos = 0;
    m_preeditList.clear();
    m_commitPreedit.clear();
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
    m_preedit = str;
    m_commitPreedit = commitStr;
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
    if (m_locale != newLocale) {
        m_locale = newLocale;
        emitLocaleChanged();
    }
}

QLocale QFcitxPlatformInputContext::locale() const { return m_locale; }

void QFcitxPlatformInputContext::createICData(QWindow *w) {
    auto iter = m_icMap.find(w);
    if (iter == m_icMap.end()) {
        m_icMap.emplace(std::piecewise_construct, std::forward_as_tuple(w),
                        std::forward_as_tuple());
        connect(w, &QObject::destroyed, this,
                &QFcitxPlatformInputContext::windowDestroyed);
    }
    createInputContext(w);
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

    int key;
    symToKeyQt(keyval, key);
    auto c = fcitx::Key::keySymToUnicode(static_cast<fcitx::KeySym>(keyval));

    QKeyEvent *keyevent = new QKeyEvent(
        isRelease ? (QEvent::KeyRelease) : (QEvent::KeyPress), key, qstate, 0,
        keyval, _state, QString::fromUcs4(&c, 1), false, count);

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

        if (!inputMethodAccepted())
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

        proxy->FocusIn();

        auto reply =
            proxy->ProcessKeyEvent(keyval, keycode, state, isRelease,
                                   QDateTime::currentDateTime().toTime_t());

        if (Q_UNLIKELY(m_syncMode)) {
            reply.waitForFinished();

            if (!m_connection->isConnected() || !reply.isFinished() ||
                reply.isError() || !reply.value()) {
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
                *keyEvent, qApp->focusWindow(), reply, this);
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

    if (!result.isError()) {
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
    if (m_icMap.empty()) {
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

bool QFcitxPlatformInputContext::processCompose(uint keyval, uint state,
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
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
