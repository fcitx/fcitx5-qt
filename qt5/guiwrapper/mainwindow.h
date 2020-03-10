/*
 *   Copyright (C) 2012~2012 by CSSlayer
 *   wengxt@gmail.com
 *   Copyright (C) 2017~2017 by xzhao
 *   i@xuzhao.net
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
 */

#ifndef FCITXQT5_GUIWRAPPER_MAINWINDOW_H
#define FCITXQT5_GUIWRAPPER_MAINWINDOW_H

#include <QDialog>

#include "fcitxqtconfiguiwidget.h"
#include "ui_mainwindow.h"
#include <QDBusPendingCallWatcher>

namespace fcitx {

class FcitxQtControllerProxy;
class FcitxQtWatcher;
class MainWindow : public QDialog, public Ui::MainWindow {
    Q_OBJECT
public:
    explicit MainWindow(const QString &path,
                        FcitxQtConfigUIWidget *pluginWidget,
                        QWidget *parent = 0);

    void setParentWindow(WId id);
public slots:
    void changed(bool changed);
    void clicked(QAbstractButton *button);
    void availabilityChanged(bool avail);
    void saveSubConfig(const QString &path);

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void saveFinished();
    void saveFinishedPhase2(QDBusPendingCallWatcher *watcher);

private:
    QString path_;
    FcitxQtWatcher *watcher_;
    FcitxQtConfigUIWidget *pluginWidget_;
    FcitxQtControllerProxy *proxy_;
    WId wid_ = 0;
    bool closeAfterSave_ = false;
};
} // namespace fcitx

#endif // FCITXQT5_GUIWRAPPER_MAINWINDOW_H
