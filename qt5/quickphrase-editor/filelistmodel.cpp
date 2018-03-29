//
// Copyright (C) 2013~2017 by CSSlayer
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

#include "filelistmodel.h"
#include <fcitx-utils/i18n.h>
#include <fcitx-utils/standardpath.h>
#include <fcntl.h>

fcitx::FileListModel::FileListModel(QObject *parent)
    : QAbstractListModel(parent) {
    loadFileList();
}

fcitx::FileListModel::~FileListModel() {}

int fcitx::FileListModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : fileList_.size();
}

QVariant fcitx::FileListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= fileList_.size())
        return QVariant();

    switch (role) {
    case Qt::DisplayRole:
        if (fileList_[index.row()] == QUICK_PHRASE_CONFIG_FILE) {
            return _("Default");
        } else {
            // remove "data/quickphrase.d/"
            const size_t length = strlen(QUICK_PHRASE_CONFIG_DIR);
            return fileList_[index.row()].mid(length + 1,
                                              fileList_[index.row()].size() -
                                                  length - strlen(".mb") - 1);
        }
    case Qt::UserRole:
        return fileList_[index.row()];
    default:
        break;
    }
    return QVariant();
}

void fcitx::FileListModel::loadFileList() {
    beginResetModel();
    fileList_.clear();
    fileList_.append(QUICK_PHRASE_CONFIG_FILE);
    auto files = StandardPath::global().multiOpen(
        StandardPath::Type::PkgData, QUICK_PHRASE_CONFIG_DIR, O_RDONLY,
        filter::Suffix(".mb"));

    for (auto &file : files) {
        fileList_.append(QString::fromLocal8Bit(
            stringutils::joinPath(QUICK_PHRASE_CONFIG_DIR, file.first).data()));
    }

    endResetModel();
}

int fcitx::FileListModel::findFile(const QString &lastFileName) {
    int idx = fileList_.indexOf(lastFileName);
    if (idx < 0) {
        return 0;
    }
    return idx;
}
