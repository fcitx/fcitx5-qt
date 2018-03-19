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

#ifndef MAIN_H
#define MAIN_H

#include <QStringList>
#include <qpa/qplatforminputcontextplugin_p.h>

#include "qfcitxplatforminputcontext.h"

namespace fcitx {

class QFcitxPlatformInputContextPlugin : public QPlatformInputContextPlugin {
    Q_OBJECT
public:
    Q_PLUGIN_METADATA(IID QPlatformInputContextFactoryInterface_iid FILE
                      "fcitx.json")
    QStringList keys() const;
    QFcitxPlatformInputContext *create(const QString &system,
                                       const QStringList &paramList);
};
} // namespace fcitx

#endif // MAIN_H
