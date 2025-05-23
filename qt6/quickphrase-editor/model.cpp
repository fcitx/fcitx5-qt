/*
 * SPDX-FileCopyrightText: 2012~2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "model.h"
#include "filelistmodel.h"
#include <QAbstractItemModel>
#include <QApplication>
#include <QFile>
#include <QFutureWatcher>
#include <QObject>
#include <QString>
#include <QTextStream>
#include <QFuture>
#include <Qt>
#include <QtConcurrentRun>
#include <fcitx-utils/fs.h>
#include <fcitx-utils/i18n.h>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/standardpaths.h>
#include <fcitx-utils/stringutils.h>
#include <fcitx-utils/utf8.h>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace fcitx {

namespace {

std::optional<std::pair<std::string, std::string>>
parseLine(const std::string &strBuf) {
    auto text = stringutils::trimView(strBuf);
    if (text.empty()) {
        return std::nullopt;
    }
    if (!utf8::validate(text)) {
        return std::nullopt;
    }

    auto pos = text.find_first_of(FCITX_WHITESPACE);
    if (pos == std::string::npos) {
        return std::nullopt;
    }

    auto word = text.find_first_not_of(FCITX_WHITESPACE, pos);
    if (word == std::string::npos) {
        return std::nullopt;
    }

    std::string key(text.begin(), text.begin() + pos);
    auto wordString =
        stringutils::unescapeForValue(std::string_view(text).substr(word));
    if (!wordString) {
        return std::nullopt;
    }

    return std::make_pair(key, *wordString);
}

QString escapeValue(const QString &v) {
    return QString::fromStdString(stringutils::escapeForValue(v.toStdString()));
}

} // namespace

using ItemType = std::pair<QString, QString>;

QuickPhraseModel::QuickPhraseModel(QObject *parent)
    : QAbstractTableModel(parent), needSave_(false), futureWatcher_(0) {}

QuickPhraseModel::~QuickPhraseModel() {}

bool QuickPhraseModel::needSave() const { return needSave_; }

QVariant QuickPhraseModel::headerData(int section, Qt::Orientation orientation,
                                      int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        if (section == 0) {
            return _("Keyword");
        }
        if (section == 1) {
            return _("Phrase");
        }
    }
    return {};
}

int QuickPhraseModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    return list_.count();
}

int QuickPhraseModel::columnCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    return 2;
}

QVariant QuickPhraseModel::data(const QModelIndex &index, int role) const {
    do {
        if ((role == Qt::DisplayRole || role == Qt::EditRole) &&
            index.row() < list_.count()) {
            if (index.column() == 0) {
                return list_[index.row()].first;
            }
            if (index.column() == 1) {
                return list_[index.row()].second;
            }
        }
    } while (0);
    return QVariant();
}

void QuickPhraseModel::addItem(const QString &macro, const QString &word) {
    beginInsertRows(QModelIndex(), list_.size(), list_.size());
    list_.append({macro, word});
    endInsertRows();
    setNeedSave(true);
}

void QuickPhraseModel::deleteItem(int row) {
    if (row >= list_.count()) {
        return;
    }
    auto item = list_.at(row);
    QString key = item.first;
    beginRemoveRows(QModelIndex(), row, row);
    list_.removeAt(row);
    endRemoveRows();
    setNeedSave(true);
}

void QuickPhraseModel::deleteAllItem() {
    if (list_.count()) {
        setNeedSave(true);
    }
    beginResetModel();
    list_.clear();
    endResetModel();
}

Qt::ItemFlags QuickPhraseModel::flags(const QModelIndex &index) const {
    if (!index.isValid()) {
        return {};
    }

    return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool QuickPhraseModel::setData(const QModelIndex &index, const QVariant &value,
                               int role) {
    if (role != Qt::EditRole) {
        return false;
    }

    if (index.column() == 0) {
        list_[index.row()].first = value.toString();

        Q_EMIT dataChanged(index, index);
        setNeedSave(true);
        return true;
    }
    if (index.column() == 1) {
        list_[index.row()].second = value.toString();

        Q_EMIT dataChanged(index, index);
        setNeedSave(true);
        return true;
    }
    return false;
}

void QuickPhraseModel::load(const QString &file, bool append) {
    if (futureWatcher_) {
        return;
    }

    beginResetModel();
    if (!append) {
        list_.clear();
        setNeedSave(false);
    } else {
        setNeedSave(true);
    }
    futureWatcher_ = new QFutureWatcher<QStringPairList>(this);
    futureWatcher_->setFuture(
        QtConcurrent::run([this, file]() { return parse(file); }));
    connect(futureWatcher_, &QFutureWatcherBase::finished, this,
            &QuickPhraseModel::loadFinished);
}

QStringPairList QuickPhraseModel::parse(const QString &file) {
    QStringPairList list;

    do {
        auto fp = fcitx::StandardPaths::global().open(
            fcitx::StandardPathsType::PkgData, file.toStdString());
        if (!fp.isValid()) {
            break;
        }

        QFile file;
        if (!file.open(fp.fd(), QFile::ReadOnly)) {
            break;
        }
        QByteArray line;
        while (!(line = file.readLine()).isNull()) {
            auto l = line.toStdString();
            auto parsed = parseLine(l);
            if (!parsed) {
                continue;
            }
            auto [key, value] = *parsed;
            if (key.empty() || value.empty()) {
                continue;
            }
            list_.append(
                {QString::fromStdString(key), QString::fromStdString(value)});
        }

        file.close();
    } while (false);

    return list;
}

void QuickPhraseModel::loadFinished() {
    list_.append(futureWatcher_->future().result());
    endResetModel();
    futureWatcher_->deleteLater();
    futureWatcher_ = 0;
}

QFutureWatcher<bool> *QuickPhraseModel::save(const QString &file) {
    auto *futureWatcher = new QFutureWatcher<bool>(this);
    futureWatcher->setFuture(QtConcurrent::run(
        [this, file, list = list_]() { return saveData(file, list); }));
    connect(futureWatcher, &QFutureWatcherBase::finished, this,
            &QuickPhraseModel::saveFinished);
    return futureWatcher;
}

void QuickPhraseModel::saveDataToStream(QTextStream &dev) {
    for (const auto &item : list_) {
        dev << item.first << "\t" << escapeValue(item.second) << "\n";
    }
}

void QuickPhraseModel::loadData(QTextStream &stream) {
    beginResetModel();
    list_.clear();
    setNeedSave(true);
    QString s;
    while (!(s = stream.readLine()).isNull()) {
        auto line = s.toStdString();
        auto parsed = parseLine(line);
        if (!parsed) {
            continue;
        }
        auto [key, value] = *parsed;
        if (key.empty() || value.empty()) {
            continue;
        }
        list_.append(
            {QString::fromStdString(key), QString::fromStdString(value)});
    }
    endResetModel();
}

bool QuickPhraseModel::saveData(const QString &file,
                                const QStringPairList &list) {
    fs::makePath(
        StandardPaths::global().userDirectory(StandardPathsType::PkgData) /
        QUICK_PHRASE_CONFIG_DIR);
    return StandardPaths::global().safeSave(
        StandardPathsType::PkgData, file.toStdString(), [&list](int fd) {
            QFile tempFile;
            if (!tempFile.open(fd, QIODevice::WriteOnly)) {
                return false;
            }
            for (const auto &item : list) {
                tempFile.write(item.first.toUtf8());
                tempFile.write("\t");
                tempFile.write(escapeValue(item.second).toUtf8());
                tempFile.write("\n");
            }
            tempFile.close();
            return true;
        });
}

void QuickPhraseModel::saveFinished() {
    QFutureWatcher<bool> *watcher =
        static_cast<QFutureWatcher<bool> *>(sender());
    QFuture<bool> future = watcher->future();
    if (future.result()) {
        setNeedSave(false);
    }
    watcher->deleteLater();
}

void QuickPhraseModel::setNeedSave(bool needSave) {
    if (needSave_ != needSave) {
        needSave_ = needSave;
        Q_EMIT needSaveChanged(needSave_);
    }
}
} // namespace fcitx
