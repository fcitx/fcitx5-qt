//
// Copyright (C) 2017~2017 by CSSlayer
// wengxt@gmail.com
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
#ifndef _WIDGETSADDONS_FCITXQTI18NHELPER_H_
#define _WIDGETSADDONS_FCITXQTI18NHELPER_H_

#include <QString>
#include <fcitx-utils/i18n.h>

namespace fcitx {

inline QString tr2fcitx(const char *message, const char *comment = nullptr) {
    if (comment && comment[0] && message && message[0]) {
        return QString(C_(comment, message));
    } else if (message && message[0]) {
        return QString(_(message));
    } else {
        return QString();
    }
}
} // namespace fcitx

#endif // _WIDGETSADDONS_FCITXQTI18NHELPER_H_
