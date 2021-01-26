/*
 * SPDX-FileCopyrightText: 2012~2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "model.h"
#include "editor.h"
#include "filelistmodel.h"
#include <QApplication>
#include <QFile>
#include <QFutureWatcher>
#include <QtConcurrentRun>
#include <fcitx-utils/i18n.h>
#include <fcitx-utils/standardpath.h>
#include <fcntl.h>

namespace fcitx {

typedef QPair<QString, QString> ItemType;

QuickPhraseModel::QuickPhraseModel(QObject *parent)
    : QAbstractTableModel(parent), needSave_(false), futureWatcher_(0) {}

QuickPhraseModel::~QuickPhraseModel() {}

bool QuickPhraseModel::needSave() { return needSave_; }

QVariant QuickPhraseModel::headerData(int section, Qt::Orientation orientation,
                                      int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        if (section == 0)
            return _("Keyword");
        else if (section == 1)
            return _("Phrase");
    }
    return QVariant();
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
            } else if (index.column() == 1) {
                return list_[index.row()].second;
            }
        }
    } while (0);
    return QVariant();
}

void QuickPhraseModel::addItem(const QString &macro, const QString &word) {
    beginInsertRows(QModelIndex(), list_.size(), list_.size());
    list_.append(QPair<QString, QString>(macro, word));
    endInsertRows();
    setNeedSave(true);
}

void QuickPhraseModel::deleteItem(int row) {
    if (row >= list_.count())
        return;
    QPair<QString, QString> item = list_.at(row);
    QString key = item.first;
    beginRemoveRows(QModelIndex(), row, row);
    list_.removeAt(row);
    endRemoveRows();
    setNeedSave(true);
}

void QuickPhraseModel::deleteAllItem() {
    if (list_.count())
        setNeedSave(true);
    beginResetModel();
    list_.clear();
    endResetModel();
}

Qt::ItemFlags QuickPhraseModel::flags(const QModelIndex &index) const {
    if (!index.isValid())
        return {};

    return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool QuickPhraseModel::setData(const QModelIndex &index, const QVariant &value,
                               int role) {
    if (role != Qt::EditRole)
        return false;

    if (index.column() == 0) {
        list_[index.row()].first = value.toString();

        Q_EMIT dataChanged(index, index);
        setNeedSave(true);
        return true;
    } else if (index.column() == 1) {
        list_[index.row()].second = value.toString();

        Q_EMIT dataChanged(index, index);
        setNeedSave(true);
        return true;
    } else
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
    } else
        setNeedSave(true);
    futureWatcher_ = new QFutureWatcher<QStringPairList>(this);
    futureWatcher_->setFuture(QtConcurrent::run<QStringPairList>(
        this, &QuickPhraseModel::parse, file));
    connect(futureWatcher_, &QFutureWatcherBase::finished, this,
            &QuickPhraseModel::loadFinished);
}

QStringPairList QuickPhraseModel::parse(const QString &file) {
    QByteArray fileNameArray = file.toLocal8Bit();
    QStringPairList list;

    do {
        auto fp = fcitx::StandardPath::global().open(
            fcitx::StandardPath::Type::PkgData, fileNameArray.constData(),
            O_RDONLY);
        if (fp.fd() < 0)
            break;

        QFile file;
        if (!file.open(fp.fd(), QFile::ReadOnly)) {
            break;
        }
        QByteArray line;
        while (!(line = file.readLine()).isNull()) {
            QString s = QString::fromUtf8(line);
            s = s.simplified();
            if (s.isEmpty())
                continue;
            QString key = s.section(" ", 0, 0, QString::SectionSkipEmpty);
            QString value = s.section(" ", 1, -1, QString::SectionSkipEmpty);
            if (key.isEmpty() || value.isEmpty())
                continue;
            list.append(QPair<QString, QString>(key, value));
        }

        file.close();
    } while (0);

    return list;
}

void QuickPhraseModel::loadFinished() {
    list_.append(futureWatcher_->future().result());
    endResetModel();
    futureWatcher_->deleteLater();
    futureWatcher_ = 0;
}

QFutureWatcher<bool> *QuickPhraseModel::save(const QString &file) {
    QFutureWatcher<bool> *futureWatcher = new QFutureWatcher<bool>(this);
    futureWatcher->setFuture(QtConcurrent::run<bool>(
        this, &QuickPhraseModel::saveData, file, list_));
    connect(futureWatcher, &QFutureWatcherBase::finished, this,
            &QuickPhraseModel::saveFinished);
    return futureWatcher;
}

void QuickPhraseModel::saveData(QTextStream &dev) {
    for (int i = 0; i < list_.size(); i++) {
        dev << list_[i].first << "\t" << list_[i].second << "\n";
    }
}

void QuickPhraseModel::loadData(QTextStream &stream) {
    beginResetModel();
    list_.clear();
    setNeedSave(true);
    QString s;
    while (!(s = stream.readLine()).isNull()) {
        s = s.simplified();
        if (s.isEmpty())
            continue;
        QString key = s.section(" ", 0, 0, QString::SectionSkipEmpty);
        QString value = s.section(" ", 1, -1, QString::SectionSkipEmpty);
        if (key.isEmpty() || value.isEmpty())
            continue;
        list_.append(QPair<QString, QString>(key, value));
    }
    endResetModel();
}

bool QuickPhraseModel::saveData(const QString &file,
                                const QStringPairList &list) {
    QByteArray filenameArray = file.toLocal8Bit();
    fs::makePath(stringutils::joinPath(
        StandardPath::global().userDirectory(StandardPath::Type::PkgData),
        QUICK_PHRASE_CONFIG_DIR));
    return StandardPath::global().safeSave(
        StandardPath::Type::PkgData, filenameArray.constData(),
        [&list](int fd) {
            QFile tempFile;
            if (!tempFile.open(fd, QIODevice::WriteOnly)) {
                return false;
            }
            for (int i = 0; i < list.size(); i++) {
                tempFile.write(list[i].first.toUtf8());
                tempFile.write("\t");
                tempFile.write(list[i].second.toUtf8());
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
