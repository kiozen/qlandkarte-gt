/* -*-mode:c; c-basic-offset:4; -*- */
/**********************************************************************************************
    Copyright (C) 2010 Albrecht Dre <albrecht.dress@arcor.de>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

**********************************************************************************************/

#ifndef __C_HELP_BUTTON__
#define __C_HELP_BUTTON__

#include <QToolButton>
#include <QString>

class CHelpButton : public QToolButton
{
    Q_OBJECT;
    public:
        CHelpButton(QWidget * parent=0);
        virtual ~CHelpButton() {};

        void setHelp(const QString & title, const QString & contents);

    private slots:
        void slotClicked();

    private:
        QString m_title;
        QString m_contents;
};

// Note: the help dialogue is just an internal helper for CHelpButton.
// DO NOT USE IT.
#include "ui_IHelpDlg.h"

class CHelpDialog : public QDialog, private Ui::IHelpDialog
{
    Q_OBJECT;
    public:
        CHelpDialog(QWidget * parent=0);
        virtual ~CHelpDialog() {};

        void setContents(const QString & contents) { helpText->setText(contents); };
};
#endif                           // __C_HELP_BUTTON__
