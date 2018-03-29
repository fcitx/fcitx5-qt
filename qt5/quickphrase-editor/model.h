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
#ifndef _QUICKPHRASE_EDITOR_MODEL_H_
#define _QUICKPHRASE_EDITOR_MODEL_H_

#include <QAbstractTableModel>
#include <QFutureWatcher>
#include <QSet>
#include <QTextStream>

class QFile;
namespace fcitx {

typedef QList<QPair<QString, QString>> QStringPairList;

class QuickPhraseModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit QuickPhraseModel(QObject *parent = 0);
    virtual ~QuickPhraseModel();

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index,
                  int role = Qt::DisplayRole) const override;
    void load(const QString &file, bool append);
    void loadData(QTextStream &stream);
    void addItem(const QString &macro, const QString &word);
    void deleteItem(int row);
    void deleteAllItem();
    QFutureWatcher<bool> *save(const QString &file);
    void saveData(QTextStream &dev);
    bool needSave();

signals:
    void needSaveChanged(bool needSave);

private slots:
    void loadFinished();
    void saveFinished();

private:
    QStringPairList parse(const QString &file);
    bool saveData(const QString &file, const fcitx::QStringPairList &list);
    void setNeedSave(bool needSave);
    bool needSave_;
    QStringPairList list_;
    QFutureWatcher<QStringPairList> *futureWatcher_;
};
} // namespace fcitx

#endif // _QUICKPHRASE_EDITOR_MODEL_H_
