/*
* Copyright (C) 2017~2017 by CSSlayer
* wengxt@gmail.com
*
* This library is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation; either version 2.1 of the
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
#ifndef _DBUSADDONS_FCITXQTWATCHER_P_H_
#define _DBUSADDONS_FCITXQTWATCHER_P_H_

#include "fcitxqtwatcher.h"
#include <QDBusServiceWatcher>

#define FCITX_MAIN_SERVICE_NAME "org.fcitx.Fcitx5"
#define FCITX_PORTAL_SERVICE_NAME "org.freedesktop.portal.Fcitx"

namespace fcitx {

class FcitxQtWatcherPrivate {
public:
    FcitxQtWatcherPrivate(FcitxQtWatcher *q) : m_serviceWatcher(q) {}

    QDBusServiceWatcher m_serviceWatcher;
    bool m_watchPortal = false;
    bool m_availability = false;
    bool m_mainPresent = false;
    bool m_portalPresent = false;
    bool m_watched = false;
};
}

#endif // _DBUSADDONS_FCITXQTWATCHER_P_H_
