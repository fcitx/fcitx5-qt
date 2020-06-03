/*
 * SPDX-FileCopyrightText: 2013~2017 CSSlayer <wengxt@gmail.com>
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
#ifndef _WIDGETSADDONS_FCITXQTKEYSEQUENCEWIDGET_H_
#define _WIDGETSADDONS_FCITXQTKEYSEQUENCEWIDGET_H_

/* this is forked from kdelibs/kdeui/kkeysequencewidget.h */

/*
    Original Copyright header
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2001, 2002 Ellis Whitehead <ellis@kde.org>
    SPDX-FileCopyrightText: 2007 Andreas Hartmetz <ahartmetz@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <QList>
#include <QPushButton>
#include <fcitx-utils/key.h>

#include "fcitx5qt5widgetsaddons_export.h"

namespace fcitx {

enum FcitxQtModifierSide { MS_Unknown = 0, MS_Left = 1, MS_Right = 2 };

class FcitxQtKeySequenceWidgetPrivate;

class FCITX5QT5WIDGETSADDONS_EXPORT FcitxQtKeySequenceWidget : public QWidget {
    Q_OBJECT

    Q_PROPERTY(bool multiKeyShortcutsAllowed READ multiKeyShortcutsAllowed WRITE
                   setMultiKeyShortcutsAllowed)

    Q_PROPERTY(bool modifierlessAllowed READ isModifierlessAllowed WRITE
                   setModifierlessAllowed)

    Q_PROPERTY(bool modifierOnlyAllowed READ isModifierOnlyAllowed WRITE
                   setModifierOnlyAllowed)

public:
    /**
     * Constructor.
     */
    explicit FcitxQtKeySequenceWidget(QWidget *parent = 0);

    /**
     * Destructs the widget.
     */
    virtual ~FcitxQtKeySequenceWidget();

    void setMultiKeyShortcutsAllowed(bool);
    bool multiKeyShortcutsAllowed() const;

    void setModifierlessAllowed(bool allow);
    bool isModifierlessAllowed();

    void setModifierOnlyAllowed(bool allow);
    bool isModifierOnlyAllowed();

    void setClearButtonShown(bool show);

    const QList<Key> &keySequence() const;

Q_SIGNALS:
    void keySequenceChanged(const QList<Key> &seq);

public Q_SLOTS:
    void captureKeySequence();
    void setKeySequence(const QList<Key> &seq);
    void clearKeySequence();

private:
    friend class FcitxQtKeySequenceWidgetPrivate;
    FcitxQtKeySequenceWidgetPrivate *const d;

    Q_DISABLE_COPY(FcitxQtKeySequenceWidget)
};
} // namespace fcitx

#endif // _WIDGETSADDONS_FCITXQTKEYSEQUENCEWIDGET_H_
