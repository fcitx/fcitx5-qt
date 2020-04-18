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
#ifndef _QUICKPHRASE_EDITOR_EDITOR_H_
#define _QUICKPHRASE_EDITOR_EDITOR_H_

#include "fcitxqtconfiguiwidget.h"
#include "model.h"
#include "ui_editor.h"
#include <QDir>
#include <QMainWindow>
#include <QMutex>

class QAbstractItemModel;

namespace fcitx {

class FileListModel;

class ListEditor final : public FcitxQtConfigUIWidget, Ui::Editor {
    Q_OBJECT
public:
    explicit ListEditor(QWidget *parent = 0);

    void load() override;
    void save() override;
    QString title() override;
    bool asyncSave() override;

    void loadFileList();

public slots:
    void batchEditAccepted();
    void removeFileTriggered();
    void addFileTriggered();
    void refreshListTriggered();
    void changeFile(int);

private slots:
    void addWord();
    void batchEditWord();
    void deleteWord();
    void deleteAllWord();
    void itemFocusChanged();
    void addWordAccepted();
    void importData();
    void exportData();
    void importFileSelected();
    void exportFileSelected();

private:
    void load(const QString &file);
    void save(const QString &file);
    QString currentFile();
    QString currentName();
    QuickPhraseModel *model_;
    FileListModel *fileListModel_;
    QMenu *operationMenu_;
    QString lastFile_;
};
} // namespace fcitx

#endif // _QUICKPHRASE_EDITOR_EDITOR_H_
