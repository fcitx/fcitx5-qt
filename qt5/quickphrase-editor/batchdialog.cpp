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

#include "batchdialog.h"
#include "ui_batchdialog.h"
#include <fcitx-utils/i18n.h>

namespace fcitx {
BatchDialog::BatchDialog(QWidget *parent) : QDialog(parent) {
    setupUi(this);
    iconLabel->setPixmap(QIcon::fromTheme("dialog-information").pixmap(22, 22));
}

BatchDialog::~BatchDialog() {}

void BatchDialog::setText(const QString &s) { plainTextEdit->setPlainText(s); }

QString BatchDialog::text() const { return plainTextEdit->toPlainText(); }
} // namespace fcitx
