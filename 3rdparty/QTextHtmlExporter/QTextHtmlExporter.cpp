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

#include "QTextHtmlExporter.h"


using namespace QLGT;

static QTextFormat formatDifference(const QTextFormat &from, const QTextFormat &to)
{
    QTextFormat diff = to;

    const QMap<int, QVariant> props = to.properties();
    for (QMap<int, QVariant>::ConstIterator it = props.begin(), end = props.end();
         it != end; ++it)
        if (it.value() == from.property(it.key()))
            diff.clearProperty(it.key());

    return diff;
}


QTextHtmlExporter::QTextHtmlExporter(const QTextDocument *_doc)
    : doc(_doc), fragmentMarkers(false)
{
    const QFont defaultFont = doc->defaultFont();
    defaultCharFormat.setFont(defaultFont);
    // don't export those for the default font since we cannot turn them off with CSS
    defaultCharFormat.clearProperty(QTextFormat::FontUnderline);
    defaultCharFormat.clearProperty(QTextFormat::FontOverline);
    defaultCharFormat.clearProperty(QTextFormat::FontStrikeOut);
    defaultCharFormat.clearProperty(QTextFormat::TextUnderlineStyle);
}

/*!
    Returns the document in HTML format. The conversion may not be
    perfect, especially for complex documents, due to the limitations
    of HTML.
*/
QString QTextHtmlExporter::toHtml(const QByteArray &encoding, ExportMode mode)
{
    html = QLatin1String("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" "
            "\"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
            "<html><head><meta name=\"qrichtext\" content=\"1\" />");
//    html.reserve(doc->docHandle()->length());

    fragmentMarkers = (mode == ExportFragment);

    if (!encoding.isEmpty())
        html += QString::fromLatin1("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=%1\" />").arg(QString::fromLatin1(encoding));

    QString title  = doc->metaInformation(QTextDocument::DocumentTitle);
    if (!title.isEmpty())
        html += QString::fromLatin1("<title>") + title + QString::fromLatin1("</title>");
    html += QLatin1String("<style type=\"text/css\">\n");
    html += QLatin1String("p, li { white-space: pre-wrap; }\n");
    html += QLatin1String("</style>");
    html += QLatin1String("</head><body");

    if (mode == ExportEntireDocument) {
        html += QLatin1String(" style=\"");

        emitFontFamily(defaultCharFormat.fontFamily());

//        if (defaultCharFormat.hasProperty(QTextFormat::FontPointSize)) {
//            html += QLatin1String(" font-size:");
//            html += QString::number(defaultCharFormat.fontPointSize());
//            html += QLatin1String("pt;");
//        }

        html += QLatin1String(" font-weight:");
        html += QString::number(defaultCharFormat.fontWeight() * 8);
        html += QLatin1Char(';');

        html += QLatin1String(" font-style:");
        html += (defaultCharFormat.fontItalic() ? QLatin1String("italic") : QLatin1String("normal"));
        html += QLatin1Char(';');

        // do not set text-decoration on the default font since those values are /always/ propagated
        // and cannot be turned off with CSS

        html += QLatin1Char('\"');

        const QTextFrameFormat fmt = doc->rootFrame()->frameFormat();
        emitBackgroundAttribute(fmt);

    } else {
        defaultCharFormat = QTextCharFormat();
    }
    html += QLatin1Char('>');

    QTextFrameFormat rootFmt = doc->rootFrame()->frameFormat();
    rootFmt.clearProperty(QTextFormat::BackgroundBrush);

    QTextFrameFormat defaultFmt;
    defaultFmt.setMargin(doc->documentMargin());

    if (rootFmt == defaultFmt)
        emitFrame(doc->rootFrame()->begin());
    else
        emitTextFrame(doc->rootFrame());

    html += QLatin1String("</body></html>");
    return html;
}


QString QTextHtmlExporter::toHtml(const QTextFrame& frame)
{
    fragmentMarkers = true;

    html.clear();

    emitFrame(frame.begin());

    return html;
}

QString QTextHtmlExporter::toHtml(const QTextTableCell& cell)
{
    fragmentMarkers = true;

    html.clear();

    emitFrame(cell.begin());

    return html;

}

void QTextHtmlExporter::emitAttribute(const char *attribute, const QString &value)
{
    html += QLatin1Char(' ');
    html += QLatin1String(attribute);
    html += QLatin1String("=\"");
#ifdef QK_QT5_PORT
    html += value.toHtmlEscaped();
#else
    html += Qt::escape(value);
#endif
    html += QLatin1Char('"');
}

bool QTextHtmlExporter::emitCharFormatStyle(const QTextCharFormat &format)
{
    bool attributesEmitted = false;

    {
        const QString family = format.fontFamily();
        if (!family.isEmpty() && family != defaultCharFormat.fontFamily()) {
            emitFontFamily(family);
            attributesEmitted = true;
        }
    }

//    if (format.hasProperty(QTextFormat::FontPointSize)
//        && format.fontPointSize() != defaultCharFormat.fontPointSize()) {
//        html += QLatin1String(" font-size:");
//        html += QString::number(format.fontPointSize());
//        html += QLatin1String("pt;");
//        attributesEmitted = true;
//    } else
//    if (format.hasProperty(QTextFormat::FontSizeAdjustment)) {
//        static const char * const sizeNames[] = {
//            "small", "medium", "large", "x-large", "xx-large"
//        };
//        const char *name = 0;
//        const int idx = format.intProperty(QTextFormat::FontSizeAdjustment) + 1;
//        if (idx >= 0 && idx <= 4) {
//            name = sizeNames[idx];
//        }
//        if (name) {
//            html += QLatin1String(" font-size:");
//            html += QLatin1String(name);
//            html += QLatin1Char(';');
//            attributesEmitted = true;
//        }
//    }

    if (format.hasProperty(QTextFormat::FontWeight)
        && format.fontWeight() != defaultCharFormat.fontWeight()) {
        html += QLatin1String(" font-weight:");
        html += QString::number(format.fontWeight() * 8);
        html += QLatin1Char(';');
        attributesEmitted = true;
    }

    if (format.hasProperty(QTextFormat::FontItalic)
        && format.fontItalic() != defaultCharFormat.fontItalic()) {
        html += QLatin1String(" font-style:");
        html += (format.fontItalic() ? QLatin1String("italic") : QLatin1String("normal"));
        html += QLatin1Char(';');
        attributesEmitted = true;
    }

    QLatin1String decorationTag(" text-decoration:");
    html += decorationTag;
    bool hasDecoration = false;
    bool atLeastOneDecorationSet = false;

    if ((format.hasProperty(QTextFormat::FontUnderline) || format.hasProperty(QTextFormat::TextUnderlineStyle))
        && format.fontUnderline() != defaultCharFormat.fontUnderline()) {
        hasDecoration = true;
        if (format.fontUnderline()) {
            html += QLatin1String(" underline");
            atLeastOneDecorationSet = true;
        }
    }

    if (format.hasProperty(QTextFormat::FontOverline)
        && format.fontOverline() != defaultCharFormat.fontOverline()) {
        hasDecoration = true;
        if (format.fontOverline()) {
            html += QLatin1String(" overline");
            atLeastOneDecorationSet = true;
        }
    }

    if (format.hasProperty(QTextFormat::FontStrikeOut)
        && format.fontStrikeOut() != defaultCharFormat.fontStrikeOut()) {
        hasDecoration = true;
        if (format.fontStrikeOut()) {
            html += QLatin1String(" line-through");
            atLeastOneDecorationSet = true;
        }
    }

    if (hasDecoration) {
        if (!atLeastOneDecorationSet)
            html += QLatin1String("none");
        html += QLatin1Char(';');
        attributesEmitted = true;
    } else {
        html.chop(qstrlen(decorationTag.latin1()));
    }

    if (format.foreground() != defaultCharFormat.foreground()
        && format.foreground().style() != Qt::NoBrush) {
        html += QLatin1String(" color:");
        html += format.foreground().color().name();
        html += QLatin1Char(';');
        attributesEmitted = true;
    }

    if (format.background() != defaultCharFormat.background()
        && format.background().style() == Qt::SolidPattern) {
        html += QLatin1String(" background-color:");
        html += format.background().color().name();
        html += QLatin1Char(';');
        attributesEmitted = true;
    }

    if (format.verticalAlignment() != defaultCharFormat.verticalAlignment()
        && format.verticalAlignment() != QTextCharFormat::AlignNormal)
    {
        html += QLatin1String(" vertical-align:");

        QTextCharFormat::VerticalAlignment valign = format.verticalAlignment();
        if (valign == QTextCharFormat::AlignSubScript)
            html += QLatin1String("sub");
        else if (valign == QTextCharFormat::AlignSuperScript)
            html += QLatin1String("super");
        else if (valign == QTextCharFormat::AlignMiddle)
            html += QLatin1String("middle");
        else if (valign == QTextCharFormat::AlignTop)
            html += QLatin1String("top");
        else if (valign == QTextCharFormat::AlignBottom)
            html += QLatin1String("bottom");

        html += QLatin1Char(';');
        attributesEmitted = true;
    }

    if (format.fontCapitalization() != QFont::MixedCase) {
        const QFont::Capitalization caps = format.fontCapitalization();
        if (caps == QFont::AllUppercase)
            html += QLatin1String(" text-transform:uppercase;");
        else if (caps == QFont::AllLowercase)
            html += QLatin1String(" text-transform:lowercase;");
        else if (caps == QFont::SmallCaps)
            html += QLatin1String(" font-variant:small-caps;");
        attributesEmitted = true;
    }

    if (format.fontWordSpacing() != 0.0) {
        html += QLatin1String(" word-spacing:");
        html += QString::number(format.fontWordSpacing());
        html += QLatin1String("px;");
        attributesEmitted = true;
    }

    return attributesEmitted;
}

void QTextHtmlExporter::emitTextLength(const char *attribute, const QTextLength &length)
{
    if (length.type() == QTextLength::VariableLength) // default
        return;

    html += QLatin1Char(' ');
    html += QLatin1String(attribute);
    html += QLatin1String("=\"");
    html += QString::number(length.rawValue());

    if (length.type() == QTextLength::PercentageLength)
        html += QLatin1String("%\"");
    else
        html += QLatin1Char('\"');
}

void QTextHtmlExporter::emitAlignment(Qt::Alignment align)
{
    if (align & Qt::AlignLeft)
        return;
    else if (align & Qt::AlignRight)
        html += QLatin1String(" align=\"right\"");
    else if (align & Qt::AlignHCenter)
        html += QLatin1String(" align=\"center\"");
    else if (align & Qt::AlignJustify)
        html += QLatin1String(" align=\"justify\"");
}

void QTextHtmlExporter::emitFloatStyle(QTextFrameFormat::Position pos, StyleMode mode)
{
    if (pos == QTextFrameFormat::InFlow)
        return;

    if (mode == EmitStyleTag)
        html += QLatin1String(" style=\"float:");
    else
        html += QLatin1String(" float:");

    if (pos == QTextFrameFormat::FloatLeft)
        html += QLatin1String(" left;");
    else if (pos == QTextFrameFormat::FloatRight)
        html += QLatin1String(" right;");
    else
        Q_ASSERT_X(0, "QTextHtmlExporter::emitFloatStyle()", "pos should be a valid enum type");

    if (mode == EmitStyleTag)
        html += QLatin1Char('\"');
}

void QTextHtmlExporter::emitBorderStyle(QTextFrameFormat::BorderStyle style)
{
    Q_ASSERT(style <= QTextFrameFormat::BorderStyle_Outset);

    html += QLatin1String(" border-style:");

    switch (style) {
    case QTextFrameFormat::BorderStyle_None:
        html += QLatin1String("none");
        break;
    case QTextFrameFormat::BorderStyle_Dotted:
        html += QLatin1String("dotted");
        break;
    case QTextFrameFormat::BorderStyle_Dashed:
        html += QLatin1String("dashed");
        break;
    case QTextFrameFormat::BorderStyle_Solid:
        html += QLatin1String("solid");
        break;
    case QTextFrameFormat::BorderStyle_Double:
        html += QLatin1String("double");
        break;
    case QTextFrameFormat::BorderStyle_DotDash:
        html += QLatin1String("dot-dash");
        break;
    case QTextFrameFormat::BorderStyle_DotDotDash:
        html += QLatin1String("dot-dot-dash");
        break;
    case QTextFrameFormat::BorderStyle_Groove:
        html += QLatin1String("groove");
        break;
    case QTextFrameFormat::BorderStyle_Ridge:
        html += QLatin1String("ridge");
        break;
    case QTextFrameFormat::BorderStyle_Inset:
        html += QLatin1String("inset");
        break;
    case QTextFrameFormat::BorderStyle_Outset:
        html += QLatin1String("outset");
        break;
    default:
        Q_ASSERT(false);
        break;
    };

    html += QLatin1Char(';');
}

void QTextHtmlExporter::emitPageBreakPolicy(QTextFormat::PageBreakFlags policy)
{
    if (policy & QTextFormat::PageBreak_AlwaysBefore)
        html += QLatin1String(" page-break-before:always;");

    if (policy & QTextFormat::PageBreak_AlwaysAfter)
        html += QLatin1String(" page-break-after:always;");
}

void QTextHtmlExporter::emitFontFamily(const QString &family)
{
//    html += QLatin1String(" font-family:");

//    QLatin1String quote("\'");
//    if (family.contains(QLatin1Char('\'')))
//        quote = QLatin1String("&quot;");

//    html += quote;
//    html += Qt::escape(family);
//    html += quote;
//    html += QLatin1Char(';');
}

void QTextHtmlExporter::emitMargins(const QString &top, const QString &bottom, const QString &left, const QString &right)
{
    html += QLatin1String(" margin-top:");
    html += top;
    html += QLatin1String("px;");

    html += QLatin1String(" margin-bottom:");
    html += bottom;
    html += QLatin1String("px;");

    html += QLatin1String(" margin-left:");
    html += left;
    html += QLatin1String("px;");

    html += QLatin1String(" margin-right:");
    html += right;
    html += QLatin1String("px;");
}

void QTextHtmlExporter::emitFragment(const QTextFragment &fragment)
{
    const QTextCharFormat format = fragment.charFormat();

    bool closeAnchor = false;

    if (format.isAnchor()) {
        const QString name = format.anchorName();
        if (!name.isEmpty()) {
            html += QLatin1String("<a name=\"");
#ifdef QK_QT5_PORT
            html += name.toHtmlEscaped();
#else
	    html += Qt::escape(name);
#endif
            html += QLatin1String("\"></a>");
        }
        const QString href = format.anchorHref();
        if (!href.isEmpty()) {
            html += QLatin1String("<a href=\"");
#ifdef QK_QT5_PORT
            html += href.toHtmlEscaped();
#else
	    html += Qt::escape(href);
#endif
            html += QLatin1String("\">");
            closeAnchor = true;
        }
    }

    QString txt = fragment.text();
    const bool isObject = txt.contains(QChar::ObjectReplacementCharacter);
    const bool isImage = isObject && format.isImageFormat();

    QLatin1String styleTag("<span style=\"");
    html += styleTag;

    bool attributesEmitted = false;
    if (!isImage)
        attributesEmitted = emitCharFormatStyle(format);
    if (attributesEmitted)
        html += QLatin1String("\">");
    else
        html.chop(qstrlen(styleTag.latin1()));

    if (isObject) {
        for (int i = 0; isImage && i < txt.length(); ++i) {
            QTextImageFormat imgFmt = format.toImageFormat();

            html += QLatin1String("<img");

            if (imgFmt.hasProperty(QTextFormat::ImageName))
                emitAttribute("src", imgFmt.name());

            if (imgFmt.hasProperty(QTextFormat::ImageWidth))
                emitAttribute("width", QString::number(imgFmt.width()));

            if (imgFmt.hasProperty(QTextFormat::ImageHeight))
                emitAttribute("height", QString::number(imgFmt.height()));

            if (imgFmt.verticalAlignment() == QTextCharFormat::AlignMiddle)
                html += QLatin1String(" style=\"vertical-align: middle;\"");
            else if (imgFmt.verticalAlignment() == QTextCharFormat::AlignTop)
                html += QLatin1String(" style=\"vertical-align: top;\"");

            if (QTextFrame *imageFrame = qobject_cast<QTextFrame *>(doc->objectForFormat(imgFmt)))
                emitFloatStyle(imageFrame->frameFormat().position());

            html += QLatin1String(" />");
        }
    } else {
        Q_ASSERT(!txt.contains(QChar::ObjectReplacementCharacter));

#ifdef QK_QT5_PORT
        txt = txt.toHtmlEscaped();
#else
	txt = Qt::escape(txt);
#endif

        // split for [\n{LineSeparator}]
        QString forcedLineBreakRegExp = QString::fromLatin1("[\\na]");
        forcedLineBreakRegExp[3] = QChar::LineSeparator;

        const QStringList lines = txt.split(QRegExp(forcedLineBreakRegExp));
        for (int i = 0; i < lines.count(); ++i) {
            if (i > 0)
                html += QLatin1String("<br />"); // space on purpose for compatibility with Netscape, Lynx & Co.
            html += lines.at(i);
        }
    }

    if (attributesEmitted)
        html += QLatin1String("</span>");

    if (closeAnchor)
        html += QLatin1String("</a>");
}

static bool isOrderedList(int style)
{
    return style == QTextListFormat::ListDecimal || style == QTextListFormat::ListLowerAlpha
           || style == QTextListFormat::ListUpperAlpha
           || style == QTextListFormat::ListUpperRoman
           || style == QTextListFormat::ListLowerRoman
       ;
}

void QTextHtmlExporter::emitBlockAttributes(const QTextBlock &block)
{
    QTextBlockFormat format = block.blockFormat();
    emitAlignment(format.alignment());
#if QT_VERSION >= 0x040700
    // assume default to not bloat the html too much
    // html += QLatin1String(" dir='ltr'");
    if (block.textDirection() == Qt::RightToLeft)
        html += QLatin1String(" dir='rtl'");
#endif

    QLatin1String style(" style=\"");
    html += style;

    const bool emptyBlock = block.begin().atEnd();
    if (emptyBlock) {
        html += QLatin1String("-qt-paragraph-type:empty;");
    }

    emitMargins(QString::number(format.topMargin()),
                QString::number(format.bottomMargin()),
                QString::number(format.leftMargin()),
                QString::number(format.rightMargin()));

    html += QLatin1String(" -qt-block-indent:");
    html += QString::number(format.indent());
    html += QLatin1Char(';');

    html += QLatin1String(" text-indent:");
    html += QString::number(format.textIndent());
    html += QLatin1String("px;");

    if (block.userState() != -1) {
        html += QLatin1String(" -qt-user-state:");
        html += QString::number(block.userState());
        html += QLatin1Char(';');
    }

    emitPageBreakPolicy(format.pageBreakPolicy());

    QTextCharFormat diff;
    if (emptyBlock) { // only print character properties when we don't expect them to be repeated by actual text in the parag
        const QTextCharFormat blockCharFmt = block.charFormat();
        diff = formatDifference(defaultCharFormat, blockCharFmt).toCharFormat();
    }

    diff.clearProperty(QTextFormat::BackgroundBrush);
    if (format.hasProperty(QTextFormat::BackgroundBrush)) {
        QBrush bg = format.background();
        if (bg.style() != Qt::NoBrush)
            diff.setProperty(QTextFormat::BackgroundBrush, format.property(QTextFormat::BackgroundBrush));
    }

    if (!diff.properties().isEmpty())
        emitCharFormatStyle(diff);

    html += QLatin1Char('"');

}

void QTextHtmlExporter::emitBlock(const QTextBlock &block)
{
    if (block.begin().atEnd()) {
        // ### HACK, remove once QTextFrame::Iterator is fixed
//        int p = block.position();
//        if (p > 0)
//            --p;
//        QTextDocumentPrivate::FragmentIterator frag = doc->docHandle()->find(p);
//        QChar ch = doc->docHandle()->buffer().at(frag->stringPosition);
//        if (ch == QTextBeginningOfFrame
//            || ch == QTextEndOfFrame)
            return;
    }

    html += QLatin1Char('\n');

    // save and later restore, in case we 'change' the default format by
    // emitting block char format information
    QTextCharFormat oldDefaultCharFormat = defaultCharFormat;

    QTextList *list = block.textList();
    if (list) {
        if (list->itemNumber(block) == 0) { // first item? emit <ul> or appropriate
            const QTextListFormat format = list->format();
            const int style = format.style();
            switch (style) {
                case QTextListFormat::ListDecimal: html += QLatin1String("<ol"); break;
                case QTextListFormat::ListDisc: html += QLatin1String("<ul"); break;
                case QTextListFormat::ListCircle: html += QLatin1String("<ul type=\"circle\""); break;
                case QTextListFormat::ListSquare: html += QLatin1String("<ul type=\"square\""); break;
                case QTextListFormat::ListLowerAlpha: html += QLatin1String("<ol type=\"a\""); break;
                case QTextListFormat::ListUpperAlpha: html += QLatin1String("<ol type=\"A\""); break;
                case QTextListFormat::ListLowerRoman: html += QLatin1String("<ol type=\"i\""); break;
                case QTextListFormat::ListUpperRoman: html += QLatin1String("<ol type=\"I\""); break;
                default: html += QLatin1String("<ul"); // ### should not happen
            }

            html += QLatin1String(" style=\"margin-top: 0px; margin-bottom: 0px; margin-left: 0px; margin-right: 0px;");

            if (format.hasProperty(QTextFormat::ListIndent)) {
                html += QLatin1String(" -qt-list-indent: ");
                html += QString::number(format.indent());
                html += QLatin1Char(';');
            }

            html += QLatin1String("\">");
        }

        html += QLatin1String("<li");

        const QTextCharFormat blockFmt = formatDifference(defaultCharFormat, block.charFormat()).toCharFormat();
        if (!blockFmt.properties().isEmpty()) {
            html += QLatin1String(" style=\"");
            emitCharFormatStyle(blockFmt);
            html += QLatin1Char('\"');

            defaultCharFormat.merge(block.charFormat());
        }
    }

    const QTextBlockFormat blockFormat = block.blockFormat();
    if (blockFormat.hasProperty(QTextFormat::BlockTrailingHorizontalRulerWidth)) {
        html += QLatin1String("<hr");

        QTextLength width = blockFormat.lengthProperty(QTextFormat::BlockTrailingHorizontalRulerWidth);
        if (width.type() != QTextLength::VariableLength)
            emitTextLength("width", width);
        else
            html += QLatin1Char(' ');

        html += QLatin1String("/>");
        return;
    }

    const bool pre = blockFormat.nonBreakableLines();
    if (pre) {
        if (list)
            html += QLatin1Char('>');
        html += QLatin1String("<pre");
    } else if (!list) {
        html += QLatin1String("<p");
    }

    emitBlockAttributes(block);

    html += QLatin1Char('>');

    QTextBlock::Iterator it = block.begin();
    if (fragmentMarkers && !it.atEnd() && block == doc->begin())
        html += QLatin1String("<!--StartFragment-->");

    for (; !it.atEnd(); ++it)
        emitFragment(it.fragment());

//    if (fragmentMarkers && block.position() + block.length() == doc->docHandle()->length())
//        html += QLatin1String("<!--EndFragment-->");

    if (pre)
        html += QLatin1String("</pre>");
    else if (list)
        html += QLatin1String("</li>");
    else
        html += QLatin1String("</p>");

    if (list) {
        if (list->itemNumber(block) == list->count() - 1) { // last item? close list
            if (isOrderedList(list->format().style()))
                html += QLatin1String("</ol>");
            else
                html += QLatin1String("</ul>");
        }
    }

    defaultCharFormat = oldDefaultCharFormat;
}

extern bool qHasPixmapTexture(const QBrush& brush);

QString QTextHtmlExporter::findUrlForImage(const QTextDocument *doc, qint64 cacheKey, bool isPixmap)
{
    QString url;
    if (!doc)
        return url;

//    if (QTextDocument *parent = qobject_cast<QTextDocument *>(doc->parent()))
//        return findUrlForImage(parent, cacheKey, isPixmap);

//    if (doc && doc->docHandle()) {
//        QTextDocumentPrivate *priv = doc->docHandle();
//        QMap<QUrl, QVariant>::const_iterator it = priv->cachedResources.constBegin();
//        for (; it != priv->cachedResources.constEnd(); ++it) {

//            const QVariant &v = it.value();
//            if (v.type() == QVariant::Image && !isPixmap) {
//                if (qvariant_cast<QImage>(v).cacheKey() == cacheKey)
//                    break;
//            }

//            if (v.type() == QVariant::Pixmap && isPixmap) {
//                if (qvariant_cast<QPixmap>(v).cacheKey() == cacheKey)
//                    break;
//            }
//        }

//        if (it != priv->cachedResources.constEnd())
//            url = it.key().toString();
//    }

    return url;
}


void QTextHtmlExporter::emitBackgroundAttribute(const QTextFormat &format)
{
    if (format.hasProperty(QTextFormat::BackgroundImageUrl)) {
        QString url = format.property(QTextFormat::BackgroundImageUrl).toString();
        emitAttribute("background", url);
    } else {
        const QBrush &brush = format.background();
        if (brush.style() == Qt::SolidPattern) {
            emitAttribute("bgcolor", brush.color().name());
        } else if (brush.style() == Qt::TexturePattern) {
            const bool isPixmap = qHasPixmapTexture(brush);
            const qint64 cacheKey = isPixmap ? brush.texture().cacheKey() : brush.textureImage().cacheKey();

            const QString url = findUrlForImage(doc, cacheKey, isPixmap);

            if (!url.isEmpty())
                emitAttribute("background", url);
        }
    }
}

void QTextHtmlExporter::emitTable(const QTextTable *table)
{
    QTextTableFormat format = table->format();

    html += QLatin1String("\n<table");

    if (format.hasProperty(QTextFormat::FrameBorder))
        emitAttribute("border", QString::number(format.border()));

    emitFrameStyle(format, TableFrame);

    emitAlignment(format.alignment());
    emitTextLength("width", format.width());

    if (format.hasProperty(QTextFormat::TableCellSpacing))
        emitAttribute("cellspacing", QString::number(format.cellSpacing()));
    if (format.hasProperty(QTextFormat::TableCellPadding))
        emitAttribute("cellpadding", QString::number(format.cellPadding()));

    emitBackgroundAttribute(format);

    html += QLatin1Char('>');

    const int rows = table->rows();
    const int columns = table->columns();

    QVector<QTextLength> columnWidths = format.columnWidthConstraints();
    if (columnWidths.isEmpty()) {
        columnWidths.resize(columns);
        columnWidths.fill(QTextLength());
    }
    Q_ASSERT(columnWidths.count() == columns);

    QVarLengthArray<bool> widthEmittedForColumn(columns);
    for (int i = 0; i < columns; ++i)
        widthEmittedForColumn[i] = false;

    const int headerRowCount = qMin(format.headerRowCount(), rows);
    if (headerRowCount > 0)
        html += QLatin1String("<thead>");

    for (int row = 0; row < rows; ++row) {
        html += QLatin1String("\n<tr>");

        for (int col = 0; col < columns; ++col) {
            const QTextTableCell cell = table->cellAt(row, col);

            // for col/rowspans
            if (cell.row() != row)
                continue;

            if (cell.column() != col)
                continue;

            html += QLatin1String("\n<td");

            if (!widthEmittedForColumn[col] && cell.columnSpan() == 1) {
                emitTextLength("width", columnWidths.at(col));
                widthEmittedForColumn[col] = true;
            }

            if (cell.columnSpan() > 1)
                emitAttribute("colspan", QString::number(cell.columnSpan()));

            if (cell.rowSpan() > 1)
                emitAttribute("rowspan", QString::number(cell.rowSpan()));

            const QTextTableCellFormat cellFormat = cell.format().toTableCellFormat();
            emitBackgroundAttribute(cellFormat);

            QTextCharFormat oldDefaultCharFormat = defaultCharFormat;

            QTextCharFormat::VerticalAlignment valign = cellFormat.verticalAlignment();

            QString styleString;
            if (valign >= QTextCharFormat::AlignMiddle && valign <= QTextCharFormat::AlignBottom) {
                styleString += QLatin1String(" vertical-align:");
                switch (valign) {
                case QTextCharFormat::AlignMiddle:
                    styleString += QLatin1String("middle");
                    break;
                case QTextCharFormat::AlignTop:
                    styleString += QLatin1String("top");
                    break;
                case QTextCharFormat::AlignBottom:
                    styleString += QLatin1String("bottom");
                    break;
                default:
                    break;
                }
                styleString += QLatin1Char(';');

                QTextCharFormat temp;
                temp.setVerticalAlignment(valign);
                defaultCharFormat.merge(temp);
            }

            if (cellFormat.hasProperty(QTextFormat::TableCellLeftPadding))
                styleString += QLatin1String(" padding-left:") + QString::number(cellFormat.leftPadding()) + QLatin1Char(';');
            if (cellFormat.hasProperty(QTextFormat::TableCellRightPadding))
                styleString += QLatin1String(" padding-right:") + QString::number(cellFormat.rightPadding()) + QLatin1Char(';');
            if (cellFormat.hasProperty(QTextFormat::TableCellTopPadding))
                styleString += QLatin1String(" padding-top:") + QString::number(cellFormat.topPadding()) + QLatin1Char(';');
            if (cellFormat.hasProperty(QTextFormat::TableCellBottomPadding))
                styleString += QLatin1String(" padding-bottom:") + QString::number(cellFormat.bottomPadding()) + QLatin1Char(';');

            if (!styleString.isEmpty())
                html += QLatin1String(" style=\"") + styleString + QLatin1Char('\"');

            html += QLatin1Char('>');

            emitFrame(cell.begin());

            html += QLatin1String("</td>");

            defaultCharFormat = oldDefaultCharFormat;
        }

        html += QLatin1String("</tr>");
        if (headerRowCount > 0 && row == headerRowCount - 1)
            html += QLatin1String("</thead>");
    }

    html += QLatin1String("</table>");
}

void QTextHtmlExporter::emitFrame(QTextFrame::Iterator frameIt)
{
    if (!frameIt.atEnd()) {
        QTextFrame::Iterator next = frameIt;
        ++next;
        if (next.atEnd()
            && frameIt.currentFrame() == 0
            && frameIt.parentFrame() != doc->rootFrame()
            && frameIt.currentBlock().begin().atEnd())
            return;
    }

    for (QTextFrame::Iterator it = frameIt;
         !it.atEnd(); ++it) {
        if (QTextFrame *f = it.currentFrame()) {
            if (QTextTable *table = qobject_cast<QTextTable *>(f)) {
                emitTable(table);
            } else {
                emitTextFrame(f);
            }
        } else if (it.currentBlock().isValid()) {
            emitBlock(it.currentBlock());
        }
    }
}

void QTextHtmlExporter::emitTextFrame(const QTextFrame *f)
{
    FrameType frameType = f->parentFrame() ? TextFrame : RootFrame;

    html += QLatin1String("\n<table");
    QTextFrameFormat format = f->frameFormat();

    if (format.hasProperty(QTextFormat::FrameBorder))
        emitAttribute("border", QString::number(format.border()));

    emitFrameStyle(format, frameType);

    emitTextLength("width", format.width());
    emitTextLength("height", format.height());

    // root frame's bcolor goes in the <body> tag
    if (frameType != RootFrame)
        emitBackgroundAttribute(format);

    html += QLatin1Char('>');
    html += QLatin1String("\n<tr>\n<td style=\"border: none;\">");
    emitFrame(f->begin());
    html += QLatin1String("</td></tr></table>");
}

void QTextHtmlExporter::emitFrameStyle(const QTextFrameFormat &format, FrameType frameType)
{
    QLatin1String styleAttribute(" style=\"");
    html += styleAttribute;
    const int originalHtmlLength = html.length();

    if (frameType == TextFrame)
        html += QLatin1String("-qt-table-type: frame;");
    else if (frameType == RootFrame)
        html += QLatin1String("-qt-table-type: root;");

    const QTextFrameFormat defaultFormat;

    emitFloatStyle(format.position(), OmitStyleTag);
    emitPageBreakPolicy(format.pageBreakPolicy());

    if (format.borderBrush() != defaultFormat.borderBrush()) {
        html += QLatin1String(" border-color:");
        html += format.borderBrush().color().name();
        html += QLatin1Char(';');
    }

    if (format.borderStyle() != defaultFormat.borderStyle())
        emitBorderStyle(format.borderStyle());

    if (format.hasProperty(QTextFormat::FrameMargin)
        || format.hasProperty(QTextFormat::FrameLeftMargin)
        || format.hasProperty(QTextFormat::FrameRightMargin)
        || format.hasProperty(QTextFormat::FrameTopMargin)
        || format.hasProperty(QTextFormat::FrameBottomMargin))
        emitMargins(QString::number(format.topMargin()),
                    QString::number(format.bottomMargin()),
                    QString::number(format.leftMargin()),
                    QString::number(format.rightMargin()));

    if (html.length() == originalHtmlLength) // nothing emitted?
        html.chop(qstrlen(styleAttribute.latin1()));
    else
        html += QLatin1Char('\"');
}
