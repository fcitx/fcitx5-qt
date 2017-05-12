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

#ifndef MAIN_H
#define MAIN_H

#include <QStringList>
#include <qpa/qplatforminputcontextplugin_p.h>

#include "qfcitxplatforminputcontext.h"

class QFcitxPlatformInputContextPlugin : public QPlatformInputContextPlugin {
    Q_OBJECT
public:
    Q_PLUGIN_METADATA(IID QPlatformInputContextFactoryInterface_iid FILE
                      "fcitx.json")
    QStringList keys() const;
    QFcitxPlatformInputContext *create(const QString &system,
                                       const QStringList &paramList);
};

#endif // MAIN_H
