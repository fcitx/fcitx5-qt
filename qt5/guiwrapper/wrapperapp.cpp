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

#include <QDebug>

#include "fcitxqtconfiguifactory.h"
#include "mainwindow.h"
#include "wrapperapp.h"
#include <QCommandLineParser>
#include <QWindow>
#include <fcitx-utils/i18n.h>
#include <fcitx-utils/standardpath.h>

namespace fcitx {

WrapperApp::WrapperApp(int &argc, char **argv)
    : QApplication(argc, argv), m_factory(new FcitxQtConfigUIFactory(this)),
      m_mainWindow(0) {
    FcitxQtConfigUIWidget *widget = 0;

    setApplicationName(QLatin1String("fcitx5-qt5-gui-wrapper"));
    setApplicationVersion(QLatin1String(FCITX5_QT_VERSION));

    QCommandLineParser parser;
    parser.setApplicationDescription(_("A launcher for Fcitx Gui plugin."));
    parser.addHelpOption();
    parser.addOptions({
        {{"w", "winid"}, _("Parent window ID"), _("winid")},
        {{"t", "test"}, _("Test if config exists")},
    });
    parser.addPositionalArgument(_("path"), _("Config path"));
    parser.process(*this);

    auto args = parser.positionalArguments();
    if (args.empty()) {
        qWarning("Missing path argument.");
        QMetaObject::invokeMethod(this, "errorExit", Qt::QueuedConnection);
        return;
    }

    if (parser.isSet("test")) {
        if (m_factory->test(args[0])) {
            QMetaObject::invokeMethod(this, "quit", Qt::QueuedConnection);
        } else {
            QMetaObject::invokeMethod(this, "errorExit", Qt::QueuedConnection);
        }
    } else {
        WId winid = 0;
        bool ok;
        if (parser.isSet("winid")) {
            winid = parser.value("winid").toLong(&ok, 0);
        }
        widget = m_factory->create(args[0]);
        if (!widget) {
            qWarning("Could not find plugin for file.");
            QMetaObject::invokeMethod(this, "errorExit", Qt::QueuedConnection);
            return;
        }
        m_mainWindow = new MainWindow(widget);
        if (ok && winid) {
            m_mainWindow->setParentWindow(winid);
        }
        m_mainWindow->exec();
        QMetaObject::invokeMethod(this, "quit", Qt::QueuedConnection);
    }
}

WrapperApp::~WrapperApp() {
    if (m_mainWindow) {
        delete m_mainWindow;
    }
}

void WrapperApp::errorExit() { exit(1); }
}
