/*
 * Copyright (C) 2012~2017 by CSSlayer
 * wengxt@gmail.com
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
#ifndef _WIDGETSADDONS_FCITXQTCONFIGUIPLUGIN_H_
#define _WIDGETSADDONS_FCITXQTCONFIGUIPLUGIN_H_

#include "fcitx5qt5widgetsaddons_export.h"
#include <QObject>
#include <QString>
#include <QStringList>

namespace fcitx {

class FcitxQtConfigUIWidget;

/**
 * interface for qt config ui
 */
struct FCITX5QT5WIDGETSADDONS_EXPORT FcitxQtConfigUIFactoryInterface {
    /**
     *  return the name for plugin
     */
    virtual QString name() = 0;

    /**
     * create new widget based on key
     *
     * @see FcitxQtConfigUIPlugin::files
     *
     * @return plugin name
     */
    virtual FcitxQtConfigUIWidget *create(const QString &key) = 0;

    /**
     * return a list that this plugin will handle, need to be consist with
     * the file path in config file
     *
     * @return support file list
     */
    virtual QStringList files() = 0;
};

#define FcitxQtConfigUIFactoryInterface_iid                                    \
    "org.fcitx.Fcitx.FcitxQtConfigUIFactoryInterface"
}

Q_DECLARE_INTERFACE(fcitx::FcitxQtConfigUIFactoryInterface,
                    FcitxQtConfigUIFactoryInterface_iid)
namespace fcitx {

/**
 * base class for qt config ui
 */
class FCITX5QT5WIDGETSADDONS_EXPORT FcitxQtConfigUIPlugin
    : public QObject,
      public FcitxQtConfigUIFactoryInterface {
    Q_OBJECT
    Q_INTERFACES(fcitx::FcitxQtConfigUIFactoryInterface)
public:
    explicit FcitxQtConfigUIPlugin(QObject *parent = 0);
    virtual ~FcitxQtConfigUIPlugin();
};

}

#endif // _WIDGETSADDONS_FCITXQTCONFIGUIPLUGIN_H_
