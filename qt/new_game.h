/*****************************************************************************
 * Eliot
 * Copyright (C) 2008 Olivier Teulière
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

#ifndef NEW_GAME_H_
#define NEW_GAME_H_

#include <QDialog>
#include <QtGui/QItemDelegate>

#include <ui/new_game.ui.h>


class Dictionary;
class Game;
class QStandardItemModel;

class NewGame: public QDialog, private Ui::NewGameDialog
{
    Q_OBJECT;

public:
    explicit NewGame(QWidget *iParent = 0);

    /// Possible values for the player type
    static const QString kHUMAN;
    static const QString kAI;

    /**
     * Create and return a game object from the information of the dialog.
     * The Game object is always valid
     */
    Game * createGame(const Dictionary& iDic) const;

private slots:
    void enableOkButton();
    void enableRemoveButton(const QItemSelection&, const QItemSelection&);

    // The following slots are automatically connected
    void on_comboBoxMode_activated(int);
    void on_pushButtonAdd_clicked();
    void on_pushButtonRemove_clicked();

private:
    /// Model of the players
    QStandardItemModel *m_model;
};


/// Delegate used for the edition of the players display
class PlayersDelegate: public QItemDelegate
{
    Q_OBJECT;

public:
    explicit PlayersDelegate(QObject *parent = 0);
    virtual ~PlayersDelegate() {}

    // Implement the needed methods
    virtual QWidget *createEditor(QWidget *parent,
                                  const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const;
    virtual void setEditorData(QWidget *editor,
                               const QModelIndex &index) const;
    virtual void setModelData(QWidget *editor,
                              QAbstractItemModel *model,
                              const QModelIndex &index) const;

    virtual void updateEditorGeometry(QWidget *editor,
                                      const QStyleOptionViewItem &option,
                                      const QModelIndex &index) const;
};


/// Event filter used for the edition of the players display
class PlayersEventFilter: public QObject
{
    Q_OBJECT;

public:
    explicit PlayersEventFilter(QObject *parent = 0);
    virtual ~PlayersEventFilter() {}

signals:
    /// As its name indicates...
    void deletePressed();

protected:
    virtual bool eventFilter(QObject *obj, QEvent *event);
};

#endif
