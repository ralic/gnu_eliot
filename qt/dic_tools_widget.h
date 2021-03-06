/*****************************************************************************
 * Eliot
 * Copyright (C) 2008-2012 Olivier Teulière
 * Authors: Olivier Teulière <ipkiss @@ gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *****************************************************************************/

#ifndef DIC_TOOLS_WIDGET_H_
#define DIC_TOOLS_WIDGET_H_

#include <string>
#include <QWidget>
#include <QPalette>

#include "ui/dic_tools_widget.ui.h"
#include "logging.h"

class QStandardItemModel;
class QString;
class Dictionary;
class CustomPopup;

class DicToolsWidget: public QWidget, private Ui::DicToolsWidget
{
    Q_OBJECT;
    DEFINE_LOGGER();

public:
    explicit DicToolsWidget(QWidget *parent = 0);

public slots:
    void setDic(const Dictionary *iDic);
    void setPlus1Rack(const QString &iRack);

signals:
    void requestDefinition(QString iWord);

private:
    /// Dictionary in which the searches are done
    const Dictionary *m_dic;

    /// Model of the tree view for the "plus 1" search
    QStandardItemModel *m_plus1Model;
    /// Model of the tree view for the "regexp" search
    QStandardItemModel *m_regexpModel;
    /// Model of the tree view for the dictionary letters
    QStandardItemModel *m_dicInfoModel;

    /// Palette to write text in red
    QPalette redPalette;
    /// Palette to write text in yellow
    QPalette darkYellowPalette;
    /// Palette to write text in green
    QPalette greenPalette;

    /// Popup menu for words definition
    CustomPopup *m_customPopupPlus1;

    /// Popup menu for words definition
    CustomPopup *m_customPopupRegexp;

private slots:

    void enableSearchPlus1();
    void enableSearchRegexp();

    void saveRegexpResults();

    /// Force synchronizing the model with the "check" results
    void refreshCheck();
    /// Force synchronizing the model with the "plus 1" results
    void refreshPlus1();
    /// Force synchronizing the model with the "regexp" results
    void refreshRegexp();
    /// Force synchronizing the model with the dictionary letters
    void refreshDicInfo();
    /// Save the words of the dictionary to a file
    void exportWordsList();
    /// Build the popup menu
    void populateMenuPlus1(QMenu &iMenu, const QPoint &iPoint);
    void populateMenuRegexp(QMenu &iMenu, const QPoint &iPoint);
};

#endif

