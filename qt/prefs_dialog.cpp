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

#include "config.h"

#include <QtCore/QSettings>
#include <QtGui/QFileDialog>

#include "prefs_dialog.h"
#include "settings.h"


const QString PrefsDialog::kINTF_ALIGN_HISTORY = "Interface/AlignHistory";
const QString PrefsDialog::kINTF_DIC_PATH = "Interface/DicPath";


PrefsDialog::PrefsDialog(QWidget *iParent)
    : QDialog(iParent)
{
    setupUi(this);

    // Interface settings
    QSettings qs(ORGANIZATION, PACKAGE_NAME);
    checkBoxIntfAlignHistory->setChecked(qs.value(kINTF_ALIGN_HISTORY).toBool());
    lineEditIntfDicPath->setText(qs.value(kINTF_DIC_PATH, "").toString());

    // Duplicate settings
    checkBoxDuplRefuseInvalid->setChecked(Settings::Instance().getBool("duplicate-reject-invalid"));
    spinBoxDuplSoloPlayers->setValue(Settings::Instance().getInt("duplicate-solo-players"));
    spinBoxDuplSoloValue->setValue(Settings::Instance().getInt("duplicate-solo-value"));

    // Freegame settings
    checkBoxFreeRefuseInvalid->setChecked(Settings::Instance().getBool("freegame-reject-invalid"));

    // Training settings
    // XXX: Hide them until there is something to show
    groupBoxTraining->hide();

    // Resize the dialog so that it gets its minimal size
    resize(10, 10);
}


void PrefsDialog::accept()
{
    updateSettings();
    QDialog::accept();
}


void PrefsDialog::updateSettings()
{
    bool shouldEmitUpdate = false;

    // Interface settings
    QSettings qs(ORGANIZATION, PACKAGE_NAME);
    if (qs.value(kINTF_ALIGN_HISTORY).toBool() != checkBoxIntfAlignHistory->isChecked())
    {
        // We need to redraw the history widget
        shouldEmitUpdate = true;
        qs.setValue(kINTF_ALIGN_HISTORY, checkBoxIntfAlignHistory->isChecked());
    }
    qs.setValue(kINTF_DIC_PATH, lineEditIntfDicPath->text());

    // Duplicate settings
    Settings::Instance().setBool("duplicate-reject-invalid",
                                 checkBoxDuplRefuseInvalid->isChecked());
    Settings::Instance().setInt("duplicate-solo-players",
                                spinBoxDuplSoloPlayers->value());
    Settings::Instance().setInt("duplicate-solo-value",
                                spinBoxDuplSoloValue->value());

    // Freegame settings
    Settings::Instance().setBool("freegame-reject-invalid",
                                 checkBoxFreeRefuseInvalid->isChecked());

    // Training settings


    if (shouldEmitUpdate)
        emit gameUpdated();
}


void PrefsDialog::on_pushButtonIntfDicBrowse_clicked()
{
    QString fileName =
        QFileDialog::getOpenFileName(this, _q("Choose a dictionary"), "", "*.dawg");
    if (fileName != "")
        lineEditIntfDicPath->setText(fileName);
}
