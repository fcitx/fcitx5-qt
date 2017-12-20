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
#ifndef _QUICKPHRASE_EDITOR_FILELISTMODEL_H_
#define _QUICKPHRASE_EDITOR_FILELISTMODEL_H_

#include <QAbstractListModel>
#include <QStringList>

#define QUICK_PHRASE_CONFIG_DIR "data/quickphrase.d"
#define QUICK_PHRASE_CONFIG_FILE "data/QuickPhrase.mb"

namespace fcitx {

class FileListModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit FileListModel(QObject *parent = 0);
    virtual ~FileListModel();

    virtual QVariant data(const QModelIndex &index,
                          int role = Qt::DisplayRole) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;

    void loadFileList();
    int findFile(const QString &lastFileName);

private:
    QStringList m_fileList;
};
}

#endif // _QUICKPHRASE_EDITOR_FILELISTMODEL_H_
