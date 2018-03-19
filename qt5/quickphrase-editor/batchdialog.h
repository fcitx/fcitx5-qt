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
#ifndef _QUICKPHRASE_EDITOR_BATCHDIALOG_H_
#define _QUICKPHRASE_EDITOR_BATCHDIALOG_H_

#include "ui_batchdialog.h"
#include <QDialog>

namespace fcitx {
class BatchDialog : public QDialog, public Ui::BatchDialog {
    Q_OBJECT
public:
    explicit BatchDialog(QWidget *parent = 0);
    virtual ~BatchDialog();

    QString text() const;
    void setText(const QString &s);
};
} // namespace fcitx

#endif // _QUICKPHRASE_EDITOR_BATCHDIALOG_H_
