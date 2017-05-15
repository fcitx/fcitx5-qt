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
#ifndef _GUIWRAPPER_WRAPPERAPP_H_
#define _GUIWRAPPER_WRAPPERAPP_H_

#include "mainwindow.h"
#include <QApplication>

class FcitxQtConfigUIFactory;
class WrapperApp : public QApplication {
    Q_OBJECT
public:
    WrapperApp(int &argc, char **argv);
    virtual ~WrapperApp();

private slots:
    void errorExit();

private:
    FcitxQtConfigUIFactory *m_factory;
    MainWindow *m_mainWindow;
};

#endif // _GUIWRAPPER_WRAPPERAPP_H_
