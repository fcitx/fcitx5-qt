/*
 * SPDX-FileCopyrightText: 2013~2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "filelistmodel.h"
#include <algorithm>
#include <fcitx-utils/i18n.h>
#include <fcitx-utils/standardpaths.h>
#include <QObject>
#include <QAbstractListModel>
#include <Qt>
#include <filesystem>
#include <iterator>

fcitx::FileListModel::FileListModel(QObject *parent)
    : QAbstractListModel(parent) {}

fcitx::FileListModel::~FileListModel() {}

int fcitx::FileListModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : fileList_.size();
}

QVariant fcitx::FileListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= fileList_.size()) {
        return {};
    }

    switch (role) {
    case Qt::DisplayRole:
        if (fileList_[index.row()] == QUICK_PHRASE_CONFIG_FILE) {
            return _("Default");
        } else {
            return QString::fromStdString(fileList_[index.row()].stem().string());
        }
    case Qt::UserRole:
        return QString::fromStdString(fileList_[index.row()].string());
    default:
        break;
    }
    return {};
}

void fcitx::FileListModel::loadFileList() {
    beginResetModel();
    fileList_.clear();
    fileList_.push_back(QUICK_PHRASE_CONFIG_FILE);
    auto files = StandardPaths::global().locate(StandardPathsType::PkgData,
                                                QUICK_PHRASE_CONFIG_DIR,
                                                pathfilter::extension(".mb"));

    for (auto &file : files) {
        fileList_.push_back(std::filesystem::path(QUICK_PHRASE_CONFIG_DIR) / file.first);
    }

    endResetModel();
}

int fcitx::FileListModel::findFile(const QString &lastFileName) {
    auto iter = std::ranges::find(fileList_, std::filesystem::path(lastFileName.toStdString()));
    if (iter == fileList_.end()) {
        return 0;
    }
    return std::distance(fileList_.begin(), iter);
}
