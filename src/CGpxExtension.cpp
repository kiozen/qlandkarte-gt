/**********************************************************************************************
    Copyright (C) 2010 Christian Treffs ctreffs@gmail.com

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

// Includes
#include "CGpxExtension.h"

void CGpxExtPt::setValues(const QDomNode& parent)
{
    QDomNode child = parent.firstChild();
    while (!child.isNull())
    {
        if (child.isElement()) {values.insert(child.nodeName(), child.toElement().text());}
        child = child.nextSibling();
    }
}


int CGpxExtPt::getSize()
{
    return values.size();
}


QString CGpxExtPt::getValue (const QString& name)
{
    return values.value(name);
}


void CGpxExtTr::addKey2List(const QDomNode& parent)
{
    QDomNode child = parent.firstChild();

    while (!child.isNull())
    {
        if (child.isElement())
        {
            set.insert(child.nodeName());
        }
        child = child.nextSibling();

    }
}
