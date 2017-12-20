/*
 *   Copyright (C) 2012~2017 by CSSlayer
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

#include "mainwindow.h"
#include "fcitxqtconfiguifactory.h"
#include "fcitxqtcontrollerproxy.h"
#include "fcitxqtwatcher.h"
#include <QDebug>
#include <QLocale>
#include <QPushButton>
#include <QWindow>
#include <fcitx-utils/i18n.h>

namespace fcitx {

MainWindow::MainWindow(FcitxQtConfigUIWidget *pluginWidget, QWidget *parent)
    : QDialog(parent), m_watcher(new FcitxQtWatcher(this)),
      m_pluginWidget(pluginWidget), m_proxy(0) {
    setupUi(this);
    m_watcher->setConnection(QDBusConnection::sessionBus());
    verticalLayout->insertWidget(0, m_pluginWidget);
    buttonBox->button(QDialogButtonBox::Ok)->setText(_("&Ok"));
    buttonBox->button(QDialogButtonBox::Apply)->setText(_("&Apply"));
    buttonBox->button(QDialogButtonBox::Reset)->setText(_("&Reset"));
    buttonBox->button(QDialogButtonBox::Close)->setText(_("&Close"));
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
    buttonBox->button(QDialogButtonBox::Reset)->setEnabled(false);
    setWindowIcon(QIcon::fromTheme(m_pluginWidget->icon()));
    setWindowTitle(m_pluginWidget->title());

    connect(m_pluginWidget, &FcitxQtConfigUIWidget::changed, this,
            &MainWindow::changed);
    if (m_pluginWidget->asyncSave()) {
        connect(m_pluginWidget, &FcitxQtConfigUIWidget::saveFinished, this,
                &MainWindow::saveFinished);
    }
    connect(buttonBox, &QDialogButtonBox::clicked, this, &MainWindow::clicked);
    connect(m_watcher, &FcitxQtWatcher::availabilityChanged, this,
            &MainWindow::availabilityChanged);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    m_watcher->watch();
}

void MainWindow::availabilityChanged(bool avail) {
    if (!avail) {
        return;
    }
    if (m_proxy) {
        delete m_proxy;
    }
    m_proxy = new FcitxQtControllerProxy(m_watcher->serviceName(),
                                         QLatin1String("/controller"),
                                         m_watcher->connection(), this);
}

void MainWindow::clicked(QAbstractButton *button) {
    QDialogButtonBox::StandardButton standardButton =
        buttonBox->standardButton(button);
    if (standardButton == QDialogButtonBox::Apply ||
        standardButton == QDialogButtonBox::Ok) {
        if (m_pluginWidget->asyncSave())
            m_pluginWidget->setEnabled(false);
        m_pluginWidget->save();
        if (!m_pluginWidget->asyncSave())
            saveFinished();
    } else if (standardButton == QDialogButtonBox::Close) {
        qApp->quit();
    } else if (standardButton == QDialogButtonBox::Reset) {
        m_pluginWidget->load();
    }
}

void MainWindow::saveFinished() {
    if (m_pluginWidget->asyncSave())
        m_pluginWidget->setEnabled(true);
    if (m_proxy) {
        m_proxy->ReloadAddonConfig(m_pluginWidget->addon());
    }
}

void MainWindow::changed(bool changed) {
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(changed);
    buttonBox->button(QDialogButtonBox::Apply)->setEnabled(changed);
    buttonBox->button(QDialogButtonBox::Reset)->setEnabled(changed);
}

MainWindow::~MainWindow() {}

void MainWindow::setParentWindow(WId id) { wid_ = id; }

void MainWindow::showEvent(QShowEvent *event) {
    if (!wid_) {
        return;
    }
    setAttribute(Qt::WA_NativeWindow, true);
    QWindow *subWindow = windowHandle();
    Q_ASSERT(subWindow);

    QWindow *mainWindow = QWindow::fromWinId(wid_);
    wid_ = 0;
    if (!mainWindow) {
        // foreign windows not supported on all platforms
        return;
    }
    connect(this, &QObject::destroyed, mainWindow, &QObject::deleteLater);
    subWindow->setTransientParent(mainWindow);

    QDialog::showEvent(event);
}
}
