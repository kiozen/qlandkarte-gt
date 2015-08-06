/**********************************************************************************************
    Copyright (C) 2012 Oliver Eichler oliver.eichler@gmx.de

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

#ifndef CDIARYEDIT_H
#define CDIARYEDIT_H

#include <QWidget>
#include "ui_IDiaryEdit.h"

class CDiary;
class CTrack;

class CDiaryEdit : public QWidget, private Ui::IDiaryEdit
{
    Q_OBJECT;
    public:
        CDiaryEdit(CDiary& diary, QWidget * parent);
        virtual ~CDiaryEdit();

        void collectData();
        void setTabTitle();

    public slots:
        void slotReload();
        void slotReload(bool fromDB);

    private slots:
        void slotSave();
        void slotPrintPreview();
        void setWindowModified();
        void setWindowModified(bool yes);
        void slotClipboardDataChanged();
        void slotTextBold();
        void slotTextUnderline();
        void slotTextItalic();
        void slotTextColor();
        void slotTextStyle(int styleIndex);

        void slotCurrentCharFormatChanged(const QTextCharFormat &format);
        void slotCursorPositionChanged();

        void slotIntReload();

    protected:
        void resizeEvent(QResizeEvent * e);
        void closeEvent(QCloseEvent * event);

    private:
        friend class CDiaryEditLock;

        void draw(QTextDocument& doc);
        void mergeFormatOnWordOrSelection(const QTextCharFormat &format);
        void fontChanged(const QFont &f);
        void colorChanged(const QColor &c);
        void getTrackProfile(CTrack * trk, QImage& image);

        enum eTblCol{eSym, eInfo, eComment, eMax};

        int isInternalEdit;

        CDiary& diary;

        QAction * actionUndo;
        QAction * actionRedo;
        QAction * actionCut;
        QAction * actionCopy;
        QAction * actionPaste;

        QAction * actionTextBold;
        QAction * actionTextUnderline;
        QAction * actionTextItalic;
        QAction * actionTextColor;

};
#endif                           //CDIARYEDIT_H
