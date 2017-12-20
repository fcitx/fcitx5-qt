//
// Copyright (C) 2012~2017 by CSSlayer
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
#ifndef _QUICKPHRASE_EDITOR_MAIN_H_
#define _QUICKPHRASE_EDITOR_MAIN_H_

#include "fcitxqtconfiguiplugin.h"

class QuickPhraseEditorPlugin : public fcitx::FcitxQtConfigUIPlugin {
    Q_OBJECT
public:
    Q_PLUGIN_METADATA(IID FcitxQtConfigUIFactoryInterface_iid FILE
                      "quickphrase-editor.json")
    explicit QuickPhraseEditorPlugin(QObject *parent = 0);
    virtual QString name();
    virtual QStringList files();
    virtual QString domain();
    virtual fcitx::FcitxQtConfigUIWidget *create(const QString &key);
};

#endif // _QUICKPHRASE_EDITOR_MAIN_H_
