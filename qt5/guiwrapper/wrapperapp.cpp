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

#include <libintl.h>
#include <locale.h>

#include <QDebug>

#include "fcitxqtconfiguifactory.h"
#include "mainwindow.h"
#include "wrapperapp.h"
#include <fcitx-utils/standardpath.h>

namespace fcitx {

WrapperApp::WrapperApp(int &argc, char **argv)
    : QApplication(argc, argv), m_factory(new FcitxQtConfigUIFactory(this)),
      m_mainWindow(0) {
    auto localedir = fcitx::StandardPath::fcitxPath("localedir");
    setlocale(LC_ALL, "");
    bindtextdomain("fcitx", localedir.c_str());
    bind_textdomain_codeset("fcitx", "UTF-8");
    textdomain("fcitx");

    FcitxQtConfigUIWidget *widget = 0;

    if (argc == 3 && strcmp(argv[1], "--test") == 0) {
        if (m_factory->test(QString::fromLocal8Bit(argv[2]))) {
            QMetaObject::invokeMethod(this, "quit", Qt::QueuedConnection);
        } else {
            QMetaObject::invokeMethod(this, "errorExit", Qt::QueuedConnection);
        }
    } else {
        if (argc == 2) {
            widget = m_factory->create(QString::fromLocal8Bit(argv[1]));
        }
        if (!widget) {
            qWarning("Could not find plugin for file.");
            QMetaObject::invokeMethod(this, "errorExit", Qt::QueuedConnection);
        } else {
            m_mainWindow = new MainWindow(widget);
            m_mainWindow->show();
        }
    }
}

WrapperApp::~WrapperApp() {
    if (m_mainWindow) {
        delete m_mainWindow;
    }
}

void WrapperApp::errorExit() { exit(1); }
}
