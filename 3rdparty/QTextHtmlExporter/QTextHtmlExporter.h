/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QTEXTHTMLEXPORTER_H
#define QTEXTHTMLEXPORTER_H

#include <QtGui>

namespace QLGT
{
    class QTextHtmlExporter
    {
    public:
        QTextHtmlExporter(const QTextDocument *_doc);

        enum ExportMode {
            ExportEntireDocument,
            ExportFragment
        };

        QString toHtml(const QByteArray &encoding, ExportMode mode = ExportEntireDocument);
        QString toHtml(const QTextFrame& frame);
        QString toHtml(const QTextTableCell& cell);

    private:
        enum StyleMode { EmitStyleTag, OmitStyleTag };
        enum FrameType { TextFrame, TableFrame, RootFrame };

        void emitFrame(QTextFrame::Iterator frameIt);
        void emitTextFrame(const QTextFrame *frame);
        void emitBlock(const QTextBlock &block);
        void emitTable(const QTextTable *table);
        void emitFragment(const QTextFragment &fragment);

        void emitBlockAttributes(const QTextBlock &block);
        bool emitCharFormatStyle(const QTextCharFormat &format);
        void emitTextLength(const char *attribute, const QTextLength &length);
        void emitAlignment(Qt::Alignment alignment);
        void emitFloatStyle(QTextFrameFormat::Position pos, StyleMode mode = EmitStyleTag);
        void emitMargins(const QString &top, const QString &bottom, const QString &left, const QString &right);
        void emitAttribute(const char *attribute, const QString &value);
        void emitFrameStyle(const QTextFrameFormat &format, FrameType frameType);
        void emitBorderStyle(QTextFrameFormat::BorderStyle style);
        void emitPageBreakPolicy(QTextFormat::PageBreakFlags policy);

        void emitFontFamily(const QString &family);

        void emitBackgroundAttribute(const QTextFormat &format);
        QString findUrlForImage(const QTextDocument *doc, qint64 cacheKey, bool isPixmap);

        QString html;
        QTextCharFormat defaultCharFormat;
        const QTextDocument *doc;
        bool fragmentMarkers;
    };
}
#endif //QTEXTHTMLEXPORTER_H

