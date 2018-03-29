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

#ifndef _DBUSADDONS_FCITXQTWATCHER_P_H_
#define _DBUSADDONS_FCITXQTWATCHER_P_H_

#include "fcitxqtwatcher.h"
#include <QDBusServiceWatcher>

#define FCITX_MAIN_SERVICE_NAME "org.fcitx.Fcitx5"
#define FCITX_PORTAL_SERVICE_NAME "org.freedesktop.portal.Fcitx"

namespace fcitx {

class FcitxQtWatcherPrivate {
public:
    FcitxQtWatcherPrivate(FcitxQtWatcher *q) : serviceWatcher_(q) {}

    QDBusServiceWatcher serviceWatcher_;
    bool watchPortal_ = false;
    bool availability_ = false;
    bool mainPresent_ = false;
    bool portalPresent_ = false;
    bool watched_ = false;
};
} // namespace fcitx

#endif // _DBUSADDONS_FCITXQTWATCHER_P_H_
