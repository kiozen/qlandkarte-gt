/** ********************************************************************************************
    Copyright (c) ???????

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
********************************************************************************************* */
#ifndef QFILEEXT_H
#define QFILEEXT_H
#include <QtCore/QFile>

class QFileExt : public QFile
{
    public:
        QFileExt(const QString &filename)
            : QFile(filename)
            , m_mapped(NULL)
            {}

        // data access function
        const char *data(qint64 offset)
        {
            if(!m_mapped)
                m_mapped = reinterpret_cast<const char*>(map(0, size()));
            return m_mapped + offset;
        }
    private:
        const char *m_mapped;
};
#endif                           //QFILEEXT_H
