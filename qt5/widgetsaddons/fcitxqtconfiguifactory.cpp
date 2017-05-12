/***************************************************************************
 *   Copyright (C) 2012~2012 by CSSlayer                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include "fcitxqtconfiguifactory.h"
#include "fcitxqtconfiguifactory_p.h"
#include "fcitxqtconfiguiplugin.h"

#include <QDebug>
#include <QDir>
#include <QLibrary>
#include <QPluginLoader>
#include <QStandardPaths>
#include <fcitx-utils/standardpath.h>
#include <libintl.h>

FcitxQtConfigUIFactoryPrivate::FcitxQtConfigUIFactoryPrivate(
    FcitxQtConfigUIFactory *factory)
    : QObject(factory), q_ptr(factory) {}

FcitxQtConfigUIFactoryPrivate::~FcitxQtConfigUIFactoryPrivate() {}

FcitxQtConfigUIFactory::FcitxQtConfigUIFactory(QObject *parent)
    : QObject(parent), d_ptr(new FcitxQtConfigUIFactoryPrivate(this)) {
    Q_D(FcitxQtConfigUIFactory);
    d->scan();
}

FcitxQtConfigUIFactory::~FcitxQtConfigUIFactory() {}

FcitxQtConfigUIWidget *FcitxQtConfigUIFactory::create(const QString &file) {
    Q_D(FcitxQtConfigUIFactory);

    if (!d->plugins.contains(file))
        return 0;

    return d->plugins[file]->create(file);
}

bool FcitxQtConfigUIFactory::test(const QString &file) {
    Q_D(FcitxQtConfigUIFactory);

    return d->plugins.contains(file);
}

void FcitxQtConfigUIFactoryPrivate::scan() {
    fcitx::StandardPath::global().scanFiles(
        fcitx::StandardPath::Type::Addon, "qt5",
        [this](const std::string &path, const std::string &dirPath, bool user) {
            do {
                if (user) {
                    break;
                }

                QDir dir(QString::fromLocal8Bit(dirPath.c_str()));
                QFileInfo fi(
                    dir.filePath(QString::fromLocal8Bit(path.c_str())));

                QString filePath = fi.filePath(); // file name with path
                QString fileName = fi.fileName(); // just file name

                if (!QLibrary::isLibrary(filePath)) {
                    break;
                }

                QPluginLoader *loader = new QPluginLoader(filePath, this);
                // qDebug() << loader->load();
                // qDebug() << loader->errorString();
                FcitxQtConfigUIFactoryInterface *plugin =
                    qobject_cast<FcitxQtConfigUIFactoryInterface *>(
                        loader->instance());
                if (plugin) {
                    QStringList list = plugin->files();
                    Q_FOREACH (const QString &s, list) { plugins[s] = plugin; }
                }
            } while (0);
            return true;
        });
}
