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

#include "CDiaryEdit.h"
#include "CGeoDB.h"
#include "CDiary.h"
#include "CDiaryDB.h"
#include "CMainWindow.h"
#include "CTabWidget.h"
#include "CWpt.h"
#include "CWptDB.h"
#include "CTrack.h"
#include "CTrackDB.h"
#include "QTextHtmlExporter.h"
#include "CMainWindow.h"
#include "CCanvas.h"
#include "CPlot.h"
#include "IUnit.h"
#include "CSettings.h"

#include <QtGui>
#include <QMessageBox>
#include <QColorDialog>
#include <QPrintDialog>
#include <QPrinter>

class CDiaryEditLock
{
    public:
        CDiaryEditLock(CDiaryEdit * d) : d(d){d->isInternalEdit += 1;}
        ~CDiaryEditLock(){d->isInternalEdit -= 1;}
    private:
        CDiaryEdit * d;
};

CDiaryEdit::CDiaryEdit(CDiary& diary, QWidget * parent)
: QWidget(parent)
, isInternalEdit(0)
, diary(diary)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setupUi(this);

    connect(textEdit, SIGNAL(currentCharFormatChanged(const QTextCharFormat &)), this, SLOT(slotCurrentCharFormatChanged(const QTextCharFormat &)));
    connect(textEdit, SIGNAL(cursorPositionChanged()), this, SLOT(slotCursorPositionChanged()));

    toolSave->setIcon(QIcon(":/icons/save.png"));
    connect(toolSave, SIGNAL(clicked(bool)), this, SLOT(slotSave()));

    toolReload->setIcon(QIcon(":/icons/refresh.png"));
    connect(toolReload, SIGNAL(clicked(bool)), this, SLOT(slotReload()));

    toolPrint->setIcon(QIcon(":/icons/iconPrint22x22.png"));
    connect(toolPrint, SIGNAL(clicked(bool)), this, SLOT(slotPrintPreview()));

    connect(textEdit, SIGNAL(textChanged()), this, SLOT(setWindowModified()));
    connect(textEdit->document(), SIGNAL(modificationChanged(bool)), this, SLOT(setWindowModified(bool)));

    toolExit->setIcon(QIcon(":/icons/iconExit16x16.png"));
    connect(toolExit, SIGNAL(clicked(bool)), this, SLOT(close()));

    actionTextBold = new QAction(QIcon(":/icons/textbold.png"), tr("&Bold"), this);
    actionTextBold->setShortcut(Qt::CTRL + Qt::Key_B);
    QFont bold;
    bold.setBold(true);
    actionTextBold->setFont(bold);
    connect(actionTextBold, SIGNAL(triggered()), this, SLOT(slotTextBold()));
    actionTextBold->setCheckable(true);
    toolBold->setDefaultAction(actionTextBold);

    actionTextItalic = new QAction(QIcon(":/icons/textitalic.png"), tr("&Italic"), this);
    actionTextItalic->setShortcut(Qt::CTRL + Qt::Key_I);
    QFont italic;
    italic.setItalic(true);
    actionTextItalic->setFont(italic);
    connect(actionTextItalic, SIGNAL(triggered()), this, SLOT(slotTextItalic()));
    actionTextItalic->setCheckable(true);
    toolItalic->setDefaultAction(actionTextItalic);

    actionTextUnderline = new QAction(QIcon(":/icons/textunder.png"), tr("&Underline"), this);
    actionTextUnderline->setShortcut(Qt::CTRL + Qt::Key_U);
    QFont underline;
    underline.setUnderline(true);
    actionTextUnderline->setFont(underline);
    connect(actionTextUnderline, SIGNAL(triggered()), this, SLOT(slotTextUnderline()));
    actionTextUnderline->setCheckable(true);
    toolUnder->setDefaultAction(actionTextUnderline);

    QPixmap pix(24, 24);
    pix.fill(Qt::black);
    actionTextColor = new QAction(pix, tr("&Color..."), this);
    connect(actionTextColor, SIGNAL(triggered()), this, SLOT(slotTextColor()));
    toolColor->setDefaultAction(actionTextColor);

    comboStyle->addItem("standard");
    comboStyle->addItem("Bullet List (Disc)");
    comboStyle->addItem("Bullet List (Circle)");
    comboStyle->addItem("Bullet List (Square)");
    comboStyle->addItem("Ordered List (Decimal)");
    comboStyle->addItem("Ordered List (Alpha lower)");
    comboStyle->addItem("Ordered List (Alpha upper)");
    connect(comboStyle, SIGNAL(activated(int)), this, SLOT(slotTextStyle(int)));

    QAction *a;
    a = actionUndo = new QAction(QIcon(":/icons/editundo.png"), tr("&Undo"), this);
    a->setShortcut(QKeySequence::Undo);
    toolUndo->setDefaultAction(a);

    a = actionRedo = new QAction(QIcon(":/icons/editredo.png"), tr("&Redo"), this);
    a->setShortcut(QKeySequence::Redo);
    toolRedo->setDefaultAction(a);

    a = actionCut = new QAction(QIcon(":/icons/editcut.png"), tr("Cu&t"), this);
    a->setShortcut(QKeySequence::Cut);
    toolCut->setDefaultAction(a);

    a = actionCopy = new QAction(QIcon(":/icons/editcopy.png"), tr("&Copy"), this);
    a->setShortcut(QKeySequence::Copy);
    toolCopy->setDefaultAction(a);

    a = actionPaste = new QAction(QIcon(":/icons/editpaste.png"), tr("&Paste"), this);
    a->setShortcut(QKeySequence::Paste);
    toolPaste->setDefaultAction(a);

    actionPaste->setEnabled(!QApplication::clipboard()->text().isEmpty());
    actionUndo->setEnabled(textEdit->document()->isUndoAvailable());
    actionRedo->setEnabled(textEdit->document()->isRedoAvailable());

    connect(textEdit->document(), SIGNAL(undoAvailable(bool)), actionUndo, SLOT(setEnabled(bool)));
    connect(textEdit->document(), SIGNAL(redoAvailable(bool)), actionRedo, SLOT(setEnabled(bool)));

    connect(actionUndo, SIGNAL(triggered()), textEdit, SLOT(undo()));
    connect(actionRedo, SIGNAL(triggered()), textEdit, SLOT(redo()));

    actionCut->setEnabled(false);
    actionCopy->setEnabled(false);

    connect(actionCut, SIGNAL(triggered()), textEdit, SLOT(cut()));
    connect(actionCopy, SIGNAL(triggered()), textEdit, SLOT(copy()));
    connect(actionPaste, SIGNAL(triggered()), textEdit, SLOT(paste()));

    connect(textEdit, SIGNAL(copyAvailable(bool)), actionCut, SLOT(setEnabled(bool)));
    connect(textEdit, SIGNAL(copyAvailable(bool)), actionCopy, SLOT(setEnabled(bool)));

    connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(slotClipboardDataChanged()));

    textEdit->setFocus();
    colorChanged(textEdit->textColor());

    SETTINGS;
    checkGeoCache->setChecked(cfg.value("diary/showGeoCaches", false).toBool());
    connect(checkGeoCache, SIGNAL(clicked()), this, SLOT(slotIntReload()));

    checkProfile->setChecked(cfg.value("diary/showProfiles", true).toBool());
    connect(checkProfile, SIGNAL(clicked()), this, SLOT(slotIntReload()));

    checkAddMap->setChecked(cfg.value("diary/addMapView", true).toBool());

}


CDiaryEdit::~CDiaryEdit()
{
    collectData();

    SETTINGS;
    cfg.setValue("diary/showGeoCaches", checkGeoCache->isChecked());
    cfg.setValue("diary/showProfiles", checkProfile->isChecked());
    cfg.setValue("diary/addMapView", checkAddMap->isChecked());
}


void CDiaryEdit::mergeFormatOnWordOrSelection(const QTextCharFormat &format)
{
    QTextCursor cursor = textEdit->textCursor();
    if (!cursor.hasSelection())
    {
        cursor.select(QTextCursor::WordUnderCursor);
    }
    cursor.mergeCharFormat(format);
    textEdit->mergeCurrentCharFormat(format);
}


void CDiaryEdit::fontChanged(const QFont &f)
{
    actionTextBold->setChecked(f.bold());
    actionTextItalic->setChecked(f.italic());
    actionTextUnderline->setChecked(f.underline());
}


void CDiaryEdit::resizeEvent(QResizeEvent * e)
{
    QWidget::resizeEvent(e);

    CDiaryEditLock lock(this);

    collectData();
    textEdit->clear();
    textEdit->document()->setTextWidth(textEdit->size().width() - 20);
    draw(*textEdit->document());
}


void CDiaryEdit::closeEvent(QCloseEvent * e)
{
    if(diary.isModified())
    {
        QMessageBox::Button res = QMessageBox::warning(this, tr("Diary modified..."), tr("The diary is modified. Do you want to save it?"), QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes);

        if(res == QMessageBox::Yes)
        {
            slotSave();
        }
    }

    QWidget::closeEvent(e);
}


void CDiaryEdit::setWindowModified()
{
    setWindowModified(true);
}


void CDiaryEdit::setWindowModified(bool yes)
{
    if(isInternalEdit || !yes) return;

    emit CDiaryDB::self().emitSigModified(diary.getKey());
    emit CDiaryDB::self().emitSigChanged();

    if(!diary.modified)
    {
        diary.modified = yes;
        setTabTitle();
    }

}


void CDiaryEdit::slotSave()
{
    collectData();

    if(!CGeoDB::self().setProjectDiaryData(diary.keyProjectGeoDB, diary))
    {
        QMessageBox::warning(0, tr("Failed..."), tr("Failed to save diary to database. Probably because it was not created from a database project."), QMessageBox::Abort, QMessageBox::Abort);
        return;
    }

    diary.modified = false;

    setTabTitle();
}


void CDiaryEdit::slotReload()
{
    slotReload(true);
}


void CDiaryEdit::slotReload(bool fromDB)
{
    CDiaryEditLock lock(this);

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    if(fromDB)
    {
        if(CGeoDB::self().getProjectDiaryData(diary.keyProjectGeoDB, diary))
        {
            diary.modified = false;
        }
    }

    setTabTitle();

    textEdit->clear();
    textEdit->document()->setTextWidth(textEdit->size().width() - 20);
    draw(*textEdit->document());

    QApplication::restoreOverrideCursor();
}


void CDiaryEdit::slotPrintPreview()
{
    CDiaryEditLock lock(this);
    collectData();

    QPrinter printer;
    QPrintDialog dialog(&printer, this);
    dialog.setWindowTitle(tr("Print Diary"));
    if (dialog.exec() != QDialog::Accepted)
        return;

    QTextDocument doc;
    QSizeF pageSize = printer.pageRect(QPrinter::DevicePixel).size();
    doc.setPageSize(pageSize);
    draw(doc);

    if(checkAddMap->isChecked())
    {
        QImage img;
        theMainWindow->getCanvas()->print(img, pageSize.toSize() - QSize(10,10));
        doc.rootFrame()->lastCursorPosition().insertImage(img);
    }
    doc.print(&printer);

    textEdit->clear();
    textEdit->document()->setTextWidth(textEdit->size().width() - 20);
    draw(*textEdit->document());
}


void CDiaryEdit::slotClipboardDataChanged()
{
    actionPaste->setEnabled(!QApplication::clipboard()->text().isEmpty());
}


void CDiaryEdit::slotTextBold()
{
    QTextCharFormat fmt;
    fmt.setFontWeight(actionTextBold->isChecked() ? QFont::Bold : QFont::Normal);
    mergeFormatOnWordOrSelection(fmt);
}


void CDiaryEdit::slotTextUnderline()
{
    QTextCharFormat fmt;
    fmt.setFontUnderline(actionTextUnderline->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}


void CDiaryEdit::slotTextItalic()
{
    QTextCharFormat fmt;
    fmt.setFontItalic(actionTextItalic->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}


void CDiaryEdit::slotTextColor()
{
    QColor col = QColorDialog::getColor(textEdit->textColor(), this);
    if (!col.isValid())
    {
        return;
    }
    QTextCharFormat fmt;
    fmt.setForeground(col);
    mergeFormatOnWordOrSelection(fmt);
    colorChanged(col);
}


void CDiaryEdit::slotTextStyle(int styleIndex)
{
    QTextCursor cursor = textEdit->textCursor();

    if (styleIndex != 0)
    {
        QTextListFormat::Style style = QTextListFormat::ListDisc;

        switch (styleIndex)
        {
            default:
            case 1:
                style = QTextListFormat::ListDisc;
                break;
            case 2:
                style = QTextListFormat::ListCircle;
                break;
            case 3:
                style = QTextListFormat::ListSquare;
                break;
            case 4:
                style = QTextListFormat::ListDecimal;
                break;
            case 5:
                style = QTextListFormat::ListLowerAlpha;
                break;
            case 6:
                style = QTextListFormat::ListUpperAlpha;
                break;
        }

        cursor.beginEditBlock();

        QTextBlockFormat blockFmt = cursor.blockFormat();

        QTextListFormat listFmt;

        if (cursor.currentList())
        {
            listFmt = cursor.currentList()->format();
        }
        else
        {
            listFmt.setIndent(blockFmt.indent() + 1);
            blockFmt.setIndent(0);
            cursor.setBlockFormat(blockFmt);
        }

        listFmt.setStyle(style);

        cursor.createList(listFmt);

        cursor.endEditBlock();
    }
    else
    {
        // ####
        QTextBlockFormat bfmt;
        bfmt.setObjectIndex(-1);
        cursor.mergeBlockFormat(bfmt);
    }
}


void CDiaryEdit::slotCurrentCharFormatChanged(const QTextCharFormat &format)
{
    fontChanged(format.font());
    colorChanged(format.foreground().color());
}


void CDiaryEdit::slotIntReload()
{
    slotReload(false);
}


void CDiaryEdit::colorChanged(const QColor &c)
{
    QPixmap pix(16, 16);
    pix.fill(c);
    actionTextColor->setIcon(pix);
}


void CDiaryEdit::slotCursorPositionChanged()
{
    int i;
    if(isInternalEdit) return;

    QTextCursor cursor = textEdit->textCursor();

    if(!diary.diaryFrame.isNull())
    {
        if(diary.diaryFrame->firstCursorPosition() <= cursor && cursor <= diary.diaryFrame->lastCursorPosition())
        {
            return;
        }
    }

    for(i = 1; i <= diary.wpts.count(); i++)
    {
        QTextTable * tbl = diary.tblWpt;
        if(tbl->cellAt(i, eComment).firstCursorPosition() <= cursor && cursor <= tbl->cellAt(i, eComment).lastCursorPosition())
        {
            return;
        }

        if(tbl->cellAt(i, eSym).firstCursorPosition() <= cursor && cursor <= tbl->cellAt(i, eInfo).lastCursorPosition())
        {
            textEdit->setTextCursor(tbl->cellAt(i, eComment).lastCursorPosition());
            return;
        }
    }

    QTextTable * tbl = diary.tblTrk;
    if(tbl)
    {
        for(i = 1; i <= diary.trks.count(); i++)
        {
            if(tbl->cellAt(i, eComment).firstCursorPosition() <= cursor && cursor <= tbl->cellAt(i, eComment).lastCursorPosition())
            {
                return;
            }

            if(tbl->cellAt(i, eSym).firstCursorPosition() <= cursor && cursor <= tbl->cellAt(i, eInfo).lastCursorPosition())
            {
                textEdit->setTextCursor(tbl->cellAt(i, eComment).lastCursorPosition());
                return;
            }
        }

        if(cursor > tbl->cellAt(i-1, eComment).lastCursorPosition())
        {
            textEdit->setTextCursor(tbl->cellAt(i-1, eComment).lastCursorPosition());
            return;
        }
    }

    if(!diary.diaryFrame.isNull())
    {
        textEdit->setTextCursor(diary.diaryFrame->lastCursorPosition());
    }

}


void CDiaryEdit::setTabTitle()
{
    CDiaryEditLock lock(this);
    CTabWidget * tab = theMainWindow->getCanvasTab();
    if(tab)
    {
        int idx = tab->indexOf(this);
        if(diary.modified)
        {
            tab->setTabText(idx, tr("Diary - %1 *").arg(diary.getName()));
        }
        else
        {
            tab->setTabText(idx, tr("Diary - %1").arg(diary.getName()));
        }
    }
}


static bool qSortWptLessName(CWpt * p1, CWpt * p2)
{

    QString name1 = p1->isGeoCache() ? p1->getGeocacheData().name : p1->getName();
    QString name2 = p2->isGeoCache() ? p2->getGeocacheData().name : p2->getName();

    return name1.toUpper() < name2.toUpper();
}


static bool qSortWptLessTime(CWpt * p1, CWpt * p2)
{
    if(p1->getTimestamp() == p2->getTimestamp())
    {
        return qSortWptLessName(p1,p2);
    }

    return p1->getTimestamp() < p2->getTimestamp();
}


static bool qSortTrkLessTime(CTrack * t1, CTrack * t2)
{
    return t1->getStartTimestamp() < t2->getStartTimestamp();
}


#define CHAR_PER_LINE 100
#define ROOT_FRAME_MARGIN 5

void CDiaryEdit::draw(QTextDocument& doc)
{
    CDiaryEditLock lock(this);
    QFontMetrics fm(QFont(font().family(),10));

    bool hasGeoCaches = false;
    int cnt;
    int w = doc.textWidth();
    int pointSize = ((10 * (w - 2 * ROOT_FRAME_MARGIN)) / (CHAR_PER_LINE *  fm.width("X")));

    if(pointSize == 0) return;

    doc.setUndoRedoEnabled(false);

    QFont f = textEdit->font();
    f.setPointSize(pointSize);
    textEdit->setFont(f);

    QTextCharFormat fmtCharHeading1;
    fmtCharHeading1.setFont(f);
    fmtCharHeading1.setFontWeight(QFont::Black);
    fmtCharHeading1.setFontPointSize(f.pointSize() + 8);

    QTextCharFormat fmtCharHeading2;
    fmtCharHeading2.setFont(f);
    fmtCharHeading2.setFontWeight(QFont::Black);
    fmtCharHeading2.setFontPointSize(f.pointSize() + 4);

    QTextCharFormat fmtCharStandard;
    fmtCharStandard.setFont(f);

    QTextCharFormat fmtCharHeader;
    fmtCharHeader.setFont(f);
    fmtCharHeader.setBackground(Qt::darkBlue);
    fmtCharHeader.setFontWeight(QFont::Bold);
    fmtCharHeader.setForeground(Qt::white);

    QTextBlockFormat fmtBlockStandard;
    fmtBlockStandard.setTopMargin(10);
    fmtBlockStandard.setBottomMargin(10);
    fmtBlockStandard.setAlignment(Qt::AlignJustify);

    QTextFrameFormat fmtFrameStandard;
    fmtFrameStandard.setTopMargin(5);
    fmtFrameStandard.setBottomMargin(5);
    fmtFrameStandard.setWidth(w - 2 * ROOT_FRAME_MARGIN);

    QTextFrameFormat fmtFrameRoot;
    fmtFrameRoot.setTopMargin(ROOT_FRAME_MARGIN);
    fmtFrameRoot.setBottomMargin(ROOT_FRAME_MARGIN);
    fmtFrameRoot.setLeftMargin(ROOT_FRAME_MARGIN);
    fmtFrameRoot.setRightMargin(ROOT_FRAME_MARGIN);

    QTextTableFormat fmtTableStandard;
    fmtTableStandard.setBorder(1);
    fmtTableStandard.setBorderBrush(Qt::black);
    fmtTableStandard.setCellPadding(4);
    fmtTableStandard.setCellSpacing(0);
    fmtTableStandard.setHeaderRowCount(1);
    fmtTableStandard.setTopMargin(10);
    fmtTableStandard.setBottomMargin(20);
    fmtTableStandard.setWidth(w - 2 * ROOT_FRAME_MARGIN);

    QVector<QTextLength> constraints;
    constraints << QTextLength(QTextLength::FixedLength, 32);
    constraints << QTextLength(QTextLength::VariableLength, 50);
    constraints << QTextLength(QTextLength::VariableLength, 100);
    fmtTableStandard.setColumnWidthConstraints(constraints);

    doc.rootFrame()->setFrameFormat(fmtFrameRoot);
    QTextCursor cursor = doc.rootFrame()->firstCursorPosition();

    cursor.insertText(diary.getName(), fmtCharHeading1);
    cursor.setCharFormat(fmtCharStandard);
    cursor.setBlockFormat(fmtBlockStandard);

    diary.diaryFrame = cursor.insertFrame(fmtFrameStandard);
    {
        QTextCursor cursor1(diary.diaryFrame);

        cursor1.setCharFormat(fmtCharStandard);
        cursor1.setBlockFormat(fmtBlockStandard);

        if(diary.getComment().isEmpty())
        {
            cursor1.insertText(tr("Add your own text here..."));
        }
        else
        {
            cursor1.insertHtml(diary.getComment());
        }
        cursor.setPosition(cursor1.position()+1);
    }

    if(!diary.getWpts().isEmpty())
    {
        QList<CWpt*>& wpts = diary.getWpts();
        cursor.insertText(tr("Waypoints"),fmtCharHeading2);

        QTextTable * table = cursor.insertTable(wpts.count()+1, eMax, fmtTableStandard);
        diary.tblWpt = table;
        table->cellAt(0,eSym).setFormat(fmtCharHeader);
        table->cellAt(0,eInfo).setFormat(fmtCharHeader);
        table->cellAt(0,eComment).setFormat(fmtCharHeader);

        table->cellAt(0,eInfo).firstCursorPosition().insertText(tr("Info"));
        table->cellAt(0,eComment).firstCursorPosition().insertText(tr("Comment"));

        cnt = 1;
        qSort(wpts.begin(), wpts.end(), qSortWptLessTime);

        foreach(CWpt * wpt, wpts)
        {

            table->cellAt(cnt,eSym).firstCursorPosition().insertImage(wpt->getIcon().toImage().scaledToWidth(16, Qt::SmoothTransformation));
            table->cellAt(cnt,eInfo).firstCursorPosition().insertText(wpt->getName() + "\n" + wpt->getInfo(), fmtCharStandard);

            QTextCursor c = table->cellAt(cnt,eComment).firstCursorPosition();
            c.setCharFormat(fmtCharStandard);
            c.setBlockFormat(fmtBlockStandard);
            c.insertHtml(wpt->getComment());

            if(wpt->isGeoCache())
            {
                hasGeoCaches = true;
            }
            cnt++;
        }

        cursor.setPosition(table->lastPosition() + 1);
    }

    if(!diary.getTrks().isEmpty())
    {
        QList<CTrack*>& trks = diary.getTrks();
        cursor.insertText(tr("Tracks"),fmtCharHeading2);

        QTextTable * table = cursor.insertTable(trks.count()+1, eMax, fmtTableStandard);
        diary.tblTrk = table;
        table->cellAt(0,eSym).setFormat(fmtCharHeader);
        table->cellAt(0,eInfo).setFormat(fmtCharHeader);
        table->cellAt(0,eComment).setFormat(fmtCharHeader);

        table->cellAt(0,eInfo).firstCursorPosition().insertText(tr("Info"));
        table->cellAt(0,eComment).firstCursorPosition().insertText(tr("Comment"));

        cnt = 1;
        qSort(trks.begin(), trks.end(), qSortTrkLessTime);

        QFontMetrics fm(f);
        foreach(CTrack * trk, trks)
        {
            table->cellAt(cnt,eSym).firstCursorPosition().insertImage(trk->getIcon().toImage().scaledToWidth(16, Qt::SmoothTransformation));
            table->cellAt(cnt,eInfo).firstCursorPosition().insertText(trk->getInfo(), fmtCharStandard);
            if(checkProfile->isChecked())
            {
                quint32 w =  doc.textWidth();
                //QImage profile(fm.width("X") * 30,fm.height()*7,QImage::Format_ARGB32);
                QImage profile(w/2.5,(w * 6)/(16 * 2.5),QImage::Format_ARGB32);
                getTrackProfile(trk, profile);
                table->cellAt(cnt,eInfo).lastCursorPosition().insertBlock(fmtBlockStandard);
                table->cellAt(cnt,eInfo).lastCursorPosition().insertImage(profile);
            }

            QTextCursor c = table->cellAt(cnt,eComment).firstCursorPosition();
            c.setCharFormat(fmtCharStandard);
            c.setBlockFormat(fmtBlockStandard);
            c.insertHtml(trk->getComment());

            cnt++;
        }

        cursor.setPosition(table->lastPosition() + 1);
    }

    if(hasGeoCaches && checkGeoCache->isChecked())
    {
        QList<CWpt*>& wpts = diary.getWpts();
        foreach(CWpt * wpt, wpts)
        {
            if(!wpt->isGeoCache())
            {
                continue;
            }

            const CWpt::geocache_t& gc = wpt->getGeocacheData();

            cursor.insertText(gc.name, fmtCharHeading2);
            cursor.setCharFormat(fmtCharStandard);
            cursor.insertBlock(fmtBlockStandard);
            cursor.insertHtml(tr("<b>Owner:</b> %1 <b>Size:</b> %2 <b>Difficulty:</b> %3 <b>Terrain:</b> %4").arg(gc.owner).arg(gc.container).arg(gc.difficulty).arg(gc.terrain));

            cursor.insertBlock(fmtBlockStandard);
            cursor.insertHtml(gc.shortDesc);
            cursor.insertBlock(fmtBlockStandard);
            cursor.insertHtml(gc.longDesc);
            cursor.insertBlock(fmtBlockStandard);

        }
    }
    doc.setUndoRedoEnabled(true);
}


void CDiaryEdit::getTrackProfile(CTrack * track, QImage& image)
{
    CPlot plot(CPlotData::eLinear, CPlot::eNormal, 0);
    plot.hide();
    plot.clear();

    QPolygonF lineElev;
    QList<QPointF> focusElev;
    float basefactor = IUnit::self().basefactor;

    QList<CTrack::pt_t>& trkpts = track->getTrackPoints();
    QList<CTrack::pt_t>::const_iterator trkpt = trkpts.begin();
    while(trkpt != trkpts.end())
    {
        if(trkpt->flags & CTrack::pt_t::eDeleted)
        {
            ++trkpt; continue;
        }

        if(trkpt->ele != WPT_NOFLOAT)
        {
            lineElev << QPointF(trkpt->distance, trkpt->ele * basefactor);
        }

        trkpt++;
    }

    plot.newLine(lineElev,focusElev, "GPS");
    //profile->setXLabel(track->getName());
    plot.setLimits();
    plot.resetZoom();

    QPainter p(&image);
    plot.draw(p,image.size());

}


void CDiaryEdit::collectData()
{
    int cnt;
    if(!diary.diaryFrame.isNull())
    {
        QString comment = QLGT::QTextHtmlExporter(textEdit->document()).toHtml(*diary.diaryFrame);
        diary.setComment(comment.trimmed());
    }

    if(!diary.tblWpt.isNull())
    {
        cnt = 1;
        QList<CWpt*>& wpts = diary.getWpts();
        qSort(wpts.begin(), wpts.end(), qSortWptLessTime);

        foreach(CWpt* wpt, wpts)
        {
            if(cnt < diary.tblWpt->rows())
            {
                wpt->setComment(QLGT::QTextHtmlExporter(textEdit->document()).toHtml(diary.tblWpt->cellAt(cnt, eComment)));
            }
            cnt++;
        }
    }

    if(!diary.tblTrk.isNull())
    {
        cnt = 1;
        QList<CTrack*>& trks = diary.getTrks();
        qSort(trks.begin(), trks.end(), qSortTrkLessTime);

        foreach(CTrack* trk, trks)
        {
            if(cnt < diary.tblTrk->rows())
            {
                trk->setComment(QLGT::QTextHtmlExporter(textEdit->document()).toHtml(diary.tblTrk->cellAt(cnt, eComment)));
            }
            cnt++;
        }
    }
}
