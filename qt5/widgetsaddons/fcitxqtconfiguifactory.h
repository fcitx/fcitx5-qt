/*
 * Copyright (C) 2017~2017 by CSSlayer
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
#ifndef _WIDGETSADDONS_FCITXQTCONFIGUIFACTORY_H_
#define _WIDGETSADDONS_FCITXQTCONFIGUIFACTORY_H_

#include <QMap>
#include <QObject>
#include <QStringList>

#include "fcitx5qt5widgetsaddons_export.h"
#include "fcitxqtconfiguiplugin.h"
#include "fcitxqtconfiguiwidget.h"

namespace fcitx {

class FcitxQtConfigUIFactoryPrivate;
/**
 * ui plugin factory.
 **/
class FCITX5QT5WIDGETSADDONS_EXPORT FcitxQtConfigUIFactory : public QObject {
    Q_OBJECT
public:
    /**
     * create a plugin factory
     *
     * @param parent object parent
     **/
    explicit FcitxQtConfigUIFactory(QObject *parent = 0);
    virtual ~FcitxQtConfigUIFactory();
    /**
     * create widget based on file name, it might return 0 if there is no match
     *
     * @param file file name need to be configured
     * @return FcitxQtConfigUIWidget*
     **/
    FcitxQtConfigUIWidget *create(const QString &file);
    /**
     * a simplified version of create, but it just test if there is a valid
     *entry or not
     *
     * @param file file name
     * @return bool
     **/
    bool test(const QString &file);

private:
    FcitxQtConfigUIFactoryPrivate *d_ptr;
    Q_DECLARE_PRIVATE(FcitxQtConfigUIFactory);
};
} // namespace fcitx

#endif // _WIDGETSADDONS_FCITXQTCONFIGUIFACTORY_H_
