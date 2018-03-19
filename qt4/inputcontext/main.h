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

#include <QInputContextPlugin>
#include <QStringList>

#include "qfcitxinputcontext.h"

namespace fcitx {

class QFcitxInputContextPlugin : public QInputContextPlugin {
    Q_OBJECT
public:
    QStringList keys() const override;

    QStringList languages(const QString &key) override;

    QString description(const QString &key) override;

    QInputContext *create(const QString &key) override;

    QString displayName(const QString &key) override;
};
} // namespace fcitx

#endif // MAIN_H
