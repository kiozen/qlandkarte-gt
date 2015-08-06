/**********************************************************************************************
    Copyright (C) 2008 Oliver Eichler oliver.eichler@gmx.de

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
#ifndef CGARMINEXPORT_H
#define CGARMINEXPORT_H

#include <QDialog>
#include <QVector>
#include "ui_IGarminExport.h"

class CMapSelectionGarmin;
class QFile;
class QByteArray;

class CGarminExport : public QDialog, private Ui::IGarminExport
{
    Q_OBJECT;
    public:
        CGarminExport(QWidget * parent);
        virtual ~CGarminExport();

        void exportToFile(CMapSelectionGarmin& ms, const QString& filename = QString::Null());

        bool hadErrors(){return errors;}

    private slots:
        void slotOutputPath();
        void slotStart();

    private:
#pragma pack(1)
        struct gmapsupp_imghdr_t
        {
            quint8  xorByte;     ///< 0x00000000
            quint8  byte0x00000001_0x00000009[9];
            quint8  upMonth;     ///< 0x0000000A
            quint8  upYear;      ///< 0x0000000B
            quint8  byte0x0000000C_0x0000000F[4];
            char    signature[7];///< 0x00000010 .. 0x00000016
            quint8  byte0x00000017_0x00000040[42];
                                 ///< 0x00000041 .. 0x00000047
            char    identifier[7];
            quint8  byte0x00000048;
            char    desc1[20];   ///< 0x00000049 .. 0x0000005C
            quint8  byte0x0000005D_0x00000060[4];
            quint8  e1;          ///< 0x00000061
            quint8  e2;          ///< 0x00000062
            quint16 nBlocks1;    ///< 0x00000063 .. 0x00000064
            char    desc2[31];   ///< 0x00000065 .. 0x00000083
            quint8  byte0x00000084_0x000001C9[0x146];
            quint16 nBlocks2;    ///< 0x000001CA .. 0x000001CB // NEVER SET???
            quint8  byte0x0000001CC_0x000001FD[0x32];
            quint16 terminator;  ///< 0x000001FE .. 0x000001FF
            quint8  byte0x00000200_0x0000040B[0x20C];
            quint32 dataoffset;  ///< 0x0000040C .. 0x0000040F
            quint8  byte0x00000410_0x00000FFF[0xBF0];

            quint32 blocksize(){return 1 << (e1 + e2);}
        };

        // Garmin IMG file FAT block structure
        struct FATblock_t
        {
            quint8  flag;        ///< 0x00000000
            char    name[8];     ///< 0x00000001 .. 0x00000008
            char    type[3];     ///< 0x00000009 .. 0x0000000B
            quint32 size;        ///< 0x0000000C .. 0x0000000F
            quint16 part;        ///< 0x00000010 .. 0x00000011
            quint8  byte0x00000012_0x0000001F[14];
            quint16 blocks[240]; ///< 0x00000020 .. 0x000001FF
        };
#ifdef WIN32
#pragma pack()
#else
#pragma pack(0)
#endif

        // description of the TRE, RGN, ... parts of GMAPSUPP.IMG subfiles
        struct gmapsupp_subfile_part_t
        {
            gmapsupp_subfile_part_t() : offset(0), size(0){}
            /// file offset of subfile part
            quint32 offset;
            /// size of the subfile part
            quint32 size;
            /// label
            QString key;
            /// number of blocks this part will use in gmapsupp
            quint32 nBlocks;
            /// number of FAT blocks this part will use in gmapsupp
            quint32 nFATBlocks;
            /// the new offset in gmapsupp
            quint32 newOffset;
        };

        // GMAPSUPP.IMG IMG subfiles (i.e. the selected tile IMG files)
        struct gmapsupp_subfile_desc_t
        {
            /// internal filename
            QString name;
            /// location information of all parts
            QMap<QString,gmapsupp_subfile_part_t> parts;
        };

        enum exce_e {eErrOpen, eErrAccess, errFormat, errLock, errLogic};
        struct exce_t
        {
            exce_t(exce_e err, const QString& msg) : err(err), msg(msg){}
            exce_e err;
            QString msg;
        };

        struct map_t
        {
            map_t() : pid(0x0320), fid(0x0001) {}
            QString map;
            QString key;
            QString typ;
            quint32 newTypOffset;

            quint16 pid;
            quint16 fid;
        };

        struct tile_t
        {
            tile_t() : pid(0x0320), fid(0x0001), isMdr(false) {}
            quint32 id;
            QString map;
            QString name;
            QString filename;
            quint32 memsize;
            QMap<QString, gmapsupp_subfile_desc_t> subfiles;

            quint16 pid;
            quint16 fid;

            bool isMdr;
        };

        void writeStdout(const QString& msg);
        void writeStderr(const QString& msg);

        void readFile(QFile& file, quint32 offset, quint32 size, QByteArray& data, quint8 mask);
        void readTileInfo(tile_t& t);
        void addTileToMPS(tile_t& t, QDataStream& mps);
        void initGmapsuppImgHdr(gmapsupp_imghdr_t& hdr, quint32 nBlocks, quint32 dataoffset);
        void initFATBlock(FATblock_t * pFAT);

        quint32 estimateBlockCount(QVector<tile_t>& _tiles, quint8 _e2);

        QVector<map_t>  maps;
        QVector<tile_t> tiles;

        quint8  e1;
        quint8  e2;
        quint32 blocksize;

        QString filename;
        bool errors;

        bool isDialog;

};
#endif                           //CGARMINEXPORT_H
