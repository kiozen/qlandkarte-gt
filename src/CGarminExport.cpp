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
#include "CGarminExport.h"
#include "CMapSelectionGarmin.h"
#include "Platform.h"
#include "CGarminTile.h"
#include "CSettings.h"

#include <QtGui>
#include <QFileDialog>
#include <QScrollBar>

#undef DEBUG_SHOW_SECT_DESC

CGarminExport::CGarminExport(QWidget * parent)
: QDialog(parent)
, e1(9)
, e2(7)
, blocksize(pow(2.0f, e1 + e2))
, errors(false)
, isDialog(true)
{
    setupUi(this);
    toolPath->setIcon(QPixmap(":/icons/iconFileLoad16x16.png"));

    SETTINGS;
    labelPath->setText(cfg.value("path/export","./").toString());

    linePrefix->setText("gmapsupp");

    connect(toolPath, SIGNAL(clicked()), this, SLOT(slotOutputPath()));
    connect(pushExport, SIGNAL(clicked()), this, SLOT(slotStart()));

}


CGarminExport::~CGarminExport()
{

}


void CGarminExport::slotOutputPath()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Select output path..."), labelPath->text(), FILE_DIALOG_FLAGS);
    if(path.isEmpty()) return;

    SETTINGS;
    cfg.setValue("path/export", path);
    labelPath->setText(path);
}


void CGarminExport::exportToFile(CMapSelectionGarmin& ms, const QString& fn)
{
    maps.clear();
    tiles.clear();
    writeStdout(tr("Creating image from maps:\n"));

    QMap<QString,CMapSelectionGarmin::map_t>::const_iterator map = ms.maps.begin();
    while(map != ms.maps.end())
    {

        map_t myMap;
        myMap.map = map->name;
        myMap.key = map->unlockKey;
        myMap.typ = map->typfile;
        myMap.fid = map->fid;
        myMap.pid = map->pid;

        maps << myMap;
        if(myMap.key.isEmpty())
        {
            writeStdout(tr("Map: %1").arg(myMap.map));
        }
        else
        {
            writeStdout(tr("Map: %1 (Key: %2)").arg(myMap.map).arg(myMap.key));
        }

        writeStdout("Tiles:");

        QMap<QString, CMapSelectionGarmin::tile_t>::const_iterator tile = map->tiles.begin();
        while(tile != map->tiles.end())
        {
            tile_t myTile;
            myTile.id       = tile->id;
            myTile.map      = map->name;
            myTile.name     = tile->name;
            myTile.filename = tile->filename;
            myTile.memsize  = tile->memSize;
            myTile.fid      = tile->fid;
            myTile.pid      = tile->pid;
            tiles << myTile;

            writeStdout(tr("    %1 (%2 MB)").arg(myTile.name).arg(double(myTile.memsize) / (1024 * 1024), 0, 'f', 2));
            ++tile;
        }

        if(!map->mdrfile.isEmpty())
        {
            tile_t  mdrTile;
            quint32 total = 0;

            mdrTile.filename = map->mdrfile;
            mdrTile.name     = "MDR file";
            try
            {
                readTileInfo(mdrTile);

                QMap<QString,gmapsupp_subfile_desc_t>::const_iterator subfile = mdrTile.subfiles.begin();
                while(subfile != mdrTile.subfiles.end())
                {
                    const QList<gmapsupp_subfile_part_t>& parts = subfile->parts.values();
                    QList<gmapsupp_subfile_part_t>::const_iterator part = parts.begin();
                    while(part != parts.end())
                    {
                        total += part->size;
                        ++part;
                    }
                    ++subfile;
                }

                mdrTile.memsize = total;
                mdrTile.isMdr   = true;
                writeStdout(tr("    %1 (%2 MB)").arg(mdrTile.name).arg(double(mdrTile.memsize) / (1024 * 1024), 0, 'f', 2));

                tiles << mdrTile;
            }
            catch(const exce_t& /*e*/)
            {
                //writeStderr(e.msg + tr(" (this is not fatal)"));
            }

        }

        writeStdout(" ");
        ++map;
    }

    if(fn.isEmpty())
    {
        exec();
    }
    else
    {
        filename = fn;
        linePrefix->setEnabled(false);
        labelPath->setEnabled(false);
        pushClose->setEnabled(false);
        toolPath->setEnabled(false);
        isDialog = false;

        show();
        slotStart();
    }
}


quint32 CGarminExport::estimateBlockCount(QVector<tile_t>& _tiles, quint8 _e2)
{
    tile_t tile;
    quint32 _blocksize  = pow(2.0f, e1 + _e2);
    quint32 _mask       = 0xFFFFFFFF >> (32 - e1 - _e2);
    quint32 _totalSize  = 0;
    quint32 _totalSize2  = 0;
    quint32 _nBlocks    = 0;

    foreach(tile, _tiles)
    {
        quint32 size = tile.memsize;
        _totalSize2 += size;
        if(size & _mask)
        {
            size = (size + _blocksize) & ~_mask;
        }

        _totalSize += size;
    }

    _nBlocks = _totalSize >> (e1 + _e2);

    //    qDebug() << hex << _e2 << _mask << _totalSize << _nBlocks << _totalSize2 << _blocksize;

    return _nBlocks;
}


void CGarminExport::writeStdout(const QString& msg)
{
    textBrowser->setTextColor(Qt::blue);
    textBrowser->append(msg);
    textBrowser->verticalScrollBar()->setValue(textBrowser->verticalScrollBar()->maximum());

    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
}


void CGarminExport::writeStderr(const QString& msg)
{
    textBrowser->setTextColor(Qt::red);
    textBrowser->append(msg);
    textBrowser->verticalScrollBar()->setValue(textBrowser->verticalScrollBar()->maximum());

    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
}


void CGarminExport::readFile(QFile& file, quint32 offset, quint32 size, QByteArray& data, quint8 mask)
{
    file.seek(offset);
    data = file.read(size);

    if((quint32)data.size() != size)
    {
        throw exce_t(eErrOpen, tr("Failed to read: ") + file.fileName());
    }

    quint8 * p = (quint8*)data.data();
    for(quint32 i = 0; i < size; ++i)
    {
        *p++ ^= mask;
    }

}


void CGarminExport::initGmapsuppImgHdr(gmapsupp_imghdr_t& hdr, quint32 nBlocks, quint32 dataoffset)
{
    // get the date
    QDateTime date = QDateTime::currentDateTime();

    // zero structure
    memset(&hdr,0,sizeof(gmapsupp_imghdr_t));

    // space-fill the descriptor strings
    memset(&hdr.desc1,0x20,sizeof(hdr.desc1));
    memset(&hdr.desc2,0x20,sizeof(hdr.desc2) - 1);

    // put in current month and year
    hdr.upMonth = date.date().month();
    hdr.upYear  = date.date().year() - 1900;

    // copy the signature to the 7 byte string
    strncpy(hdr.signature,"DSKIMG",7);

    // copy the identifier to the 7 byte string
    strncpy(hdr.identifier,"GARMIN",7);

    // identify creator in the map description
    memcpy(hdr.desc1,"QLandkarte",10);

    // as far as is known, E1 is always 9, to force a minimum 512 block size
    hdr.e1 = e1;
    // the E2 corresponding to the optimum block size
    hdr.e2 = e2;

    // set the number of file blocks in the named field (note: unaligned!)
    gar_store(uint16_t, hdr.nBlocks1, (quint16) nBlocks);

    // add the "partition table" terminator
    hdr.terminator = gar_endian(uint16_t, 0xAA55);

    // add the data offset
    hdr.dataoffset = gar_endian(uint32_t, dataoffset);

    // create a pointer to set various unnamed fields
    quint8 * p = (quint8*)&hdr;

    // various - watch out for unaligned destinations
                                 // non-standard?
    *(p + 0x0E)               = 0x01;
                                 // standard value
    *(p + 0x17)               = 0x02;
                                 // standard is 0x0004
    *(quint16*)(p + 0x18)     = gar_endian(uint16_t, 0x0020);
                                 // standard is 0x0010
    *(quint16*)(p + 0x1A)     = gar_endian(uint16_t, 0x0020);
                                 // standard is 0x0020
    *(quint16*)(p + 0x1C)     = gar_endian(uint16_t, 0x03C7);
                                 // copies (0x1a)
    gar_ptr_store(uint16_t, p + 0x5D, 0x0020);
                                 // copies (0x18)
    gar_ptr_store(uint16_t, p + 0x5F, 0x0020);

    // date stuff
    gar_ptr_store(uint16_t, p + 0x39, date.date().year());
    *(p + 0x3B)               = date.date().month();
    *(p + 0x3C)               = date.date().day();
    *(p + 0x3D)               = date.time().hour();
    *(p + 0x3E)               = date.time().minute();
    *(p + 0x3F)               = date.time().second();
                                 // 0x02 standard. bit for copied map set?
    *(p + 0x40)               = 0x08;

    // more
                                 // copies (0x18) to a *2* byte field
    *(quint16*)(p + 0x1C4)    = gar_endian(uint16_t, 0x0020);
    /*  // not really needed ???
     *(p + 0x1C0)               = 0x01; // standard value
     *(p + 0x1C3)               = 0x15; // normal, but *not* (0x1A) - 1
     *(p + 0x1C4)               = 0x10; // value above
     *(p + 0x1C5)               = 0x00;
     */

    // set the number of file blocks at 0x1CA
    *(quint16*)(p + 0x1CA) = gar_endian(uint16_t, (quint16) nBlocks);

}


void CGarminExport::initFATBlock(FATblock_t * pFAT)
{
    memset(pFAT, 0xFF, sizeof(FATblock_t));
    pFAT->flag = 0x01;
    memset(pFAT->name, 0x20, sizeof(pFAT->name));
    memset(pFAT->type, 0x20, sizeof(pFAT->type));
    //     memset(pFAT->byte0x00000012_0x0000001F, 0x00, sizeof(pFAT->byte0x00000012_0x0000001F));
}


void CGarminExport::readTileInfo(tile_t& t)
{
    char tmpstr[64];
    quint8 mask;

    QMap<QString, gmapsupp_subfile_desc_t>& subfiles = t.subfiles;

    qint64  fsize = QFileInfo(t.filename).size();

    QFile file(t.filename);
    if(!file.open(QIODevice::ReadOnly))
    {
        throw exce_t(eErrOpen, tr("Failed to open: ") + t.filename);
    }
    file.read((char*)&mask, 1);

    // read hdr_img_t
    QByteArray imghdr;
    readFile(file, 0, sizeof(CGarminTile::hdr_img_t), imghdr, mask);
    CGarminTile::hdr_img_t * pImgHdr = (CGarminTile::hdr_img_t*)imghdr.data();

    if(strncmp(pImgHdr->signature,"DSKIMG",7) != 0)
    {
        throw exce_t(errFormat,tr("Bad file format: ") + t.filename);
    }
    if(strncmp(pImgHdr->identifier,"GARMIN",7) != 0)
    {
        throw exce_t(errFormat,tr("Bad file format: ") + t.filename);
    }

    size_t blocksize = pImgHdr->blocksize();

    // 1st read FAT
    QByteArray FATblock;
    readFile(file, sizeof(CGarminTile::hdr_img_t), sizeof(CGarminTile::FATblock_t), FATblock, mask);
    const CGarminTile::FATblock_t * pFATBlock = (const CGarminTile::FATblock_t * )FATblock.data();

    size_t dataoffset = sizeof(CGarminTile::hdr_img_t);

    // skip dummy blocks at the beginning
    while(dataoffset < (size_t)fsize)
    {
        if(pFATBlock->flag != 0x00)
        {
            break;
        }
        dataoffset += sizeof(CGarminTile::FATblock_t);
        readFile(file, dataoffset, sizeof(CGarminTile::FATblock_t), FATblock, mask);
        pFATBlock = (const CGarminTile::FATblock_t * )FATblock.data();
    }

    QSet<QString> subfileNames;
    while(dataoffset < (size_t)fsize)
    {
        if(pFATBlock->flag != 0x01)
        {
            break;
        }

        memcpy(tmpstr,pFATBlock->name,sizeof(pFATBlock->name) + sizeof(pFATBlock->type));
        tmpstr[sizeof(pFATBlock->name) + sizeof(pFATBlock->type)] = 0;

        if((pFATBlock->part == 0) && subfileNames.contains(tmpstr))
        {
            writeStderr(t.name + tr("contains a duplicate internal filename. Skipped!"));
            return;
        }

        if(gar_load(uint32_t, pFATBlock->size) != 0 && !subfileNames.contains(tmpstr) && tmpstr[0] != 0x20)
        {
            subfileNames << tmpstr;

            memcpy(tmpstr,pFATBlock->name,sizeof(pFATBlock->name));
            tmpstr[sizeof(pFATBlock->name)] = 0;

            // skip MAPSORC.MPS section
            if(strcmp(tmpstr,"MAPSOURC") && strcmp(tmpstr,"SENDMAP2"))
            {

                gmapsupp_subfile_desc_t& subfile = subfiles[tmpstr];
                subfile.name = tmpstr;

                memcpy(tmpstr,pFATBlock->type,sizeof(pFATBlock->type));
                tmpstr[sizeof(pFATBlock->type)] = 0;

                gmapsupp_subfile_part_t& part = subfile.parts[tmpstr];
                part.size   = gar_load(uint32_t, pFATBlock->size);
                part.offset = gar_load(uint16_t, pFATBlock->blocks[0]) * blocksize;
                part.key    = tmpstr;
            }

        }

        dataoffset += sizeof(CGarminTile::FATblock_t);
        readFile(file, dataoffset, sizeof(CGarminTile::FATblock_t), FATblock, mask);
        pFATBlock = (const CGarminTile::FATblock_t * )FATblock.data();
    }

    if((dataoffset == sizeof(CGarminTile::hdr_img_t)) || (dataoffset >= (size_t)fsize))
    {
        throw exce_t(errFormat,tr("Failed to read file structure: ") + t.filename);
    }

#ifdef DEBUG_SHOW_SECT_DESC
    {
        quint32 total = 0;
        QMap<QString,gmapsupp_subfile_desc_t>::const_iterator subfile = subfiles.begin();
        while(subfile != subfiles.end())
        {
            qDebug() << "--- subfile" << subfile->name << "---";

            const QList<gmapsupp_subfile_part_t>& parts = subfile->parts.values();
            QList<gmapsupp_subfile_part_t>::const_iterator part = parts.begin();
            while(part != parts.end())
            {
                qDebug() << part->key << hex << part->offset << part->size;
                total += part->size;
                ++part;
            }
            ++subfile;
        }
        qDebug() << "total" << total;
    }
#endif                       //DEBUG_SHOW_SECT_DESC

}


void CGarminExport::addTileToMPS(tile_t& t, QDataStream& mps)
{
    mps << quint8('L');
    // size of the next data
    mps << quint16(16 + ((t.map.size() + 1)<<1) + t.name.size() + 1);
    // file and product id
    mps << t.fid << t.pid;
    // file ID, map name, tile name
    mps << t.id;
    mps.writeRawData(t.map.toLatin1(),t.map.size() + 1);
    mps.writeRawData(t.name.toLatin1(),t.name.size() + 1);
    mps.writeRawData(t.map.toLatin1(),t.map.size() + 1);

    QString intname = t.subfiles.keys()[0];
    // ??? wow. :-/ write the number in the internal name
    if(intname[0].isDigit())
    {
        mps << (quint32)intname.toInt(0);
    }
    else
    {
        mps << (quint32)intname.mid(1).toInt(0,16);
    }
    // terminator?
    mps << (quint32)0;

}


void CGarminExport::slotStart()
{
    quint32 i;
    quint16 blockcnt  = 0;
    QByteArray mapsourc;
    QDataStream mps(&mapsourc,QIODevice::WriteOnly);
    mps.setVersion(QDataStream::Qt_4_5);
    mps.setByteOrder(QDataStream::LittleEndian);

    pushExport->setEnabled(false);
    pushClose->setEnabled(false);

    try
    {
        quint32 totalBlocks = 0;
        quint32 totalFATs   = 1; // one for the FAT itself
        quint32 maxFATs     = (240 * blocksize) / sizeof(CGarminTile::FATblock_t);
        quint32 maxFileSize = 0xFFFFFFFF;

        /////////////////////////////////////////////////////////////
        // first run. read file structure of all tiles
        /////////////////////////////////////////////////////////////
        QVector<tile_t>::iterator tile = tiles.begin();
        while(tile != tiles.end())
        {
            // read img file
            try
            {
                readTileInfo(*tile);
            }
            catch(const exce_t& e)
            {
                writeStderr(e.msg);
                return;
            }

            // iterate over all subfiles and their parts to count FAT blocks and blocks
            QMap<QString, gmapsupp_subfile_desc_t>& subfiles          = tile->subfiles;
            QMap<QString,gmapsupp_subfile_desc_t>::iterator subfile   = subfiles.begin();
            while(subfile != subfiles.end())
            {

                QMap<QString, gmapsupp_subfile_part_t>& parts         = subfile->parts;
                QMap<QString, gmapsupp_subfile_part_t>::iterator part = parts.begin();
                while(part != parts.end())
                {

                    part->nBlocks    = ceil( double(part->size) / blocksize );
                    part->nFATBlocks = ceil( double(part->nBlocks) / 240 );
                    totalBlocks     += part->nBlocks;
                    totalFATs       += part->nFATBlocks;

                    ++part;
                }

                ++subfile;
            }

            // add tile to mapsource.mps section
            if(!tile->isMdr)
            {
                addTileToMPS(*tile, mps);
            }

            ++tile;
        }

        /////////////////////////////////////////////////////////////
        // test for typ files
        /////////////////////////////////////////////////////////////
        quint32 nBlockTyp = 0;
        quint32 nFATTyp   = 0;
        QVector<map_t>::iterator map = maps.begin();
        while(map != maps.end())
        {
            if(!map->typ.isEmpty())
            {
                QFileInfo fi(map->typ);
                nBlockTyp += ceil(double(fi.size()) / blocksize);
                nFATTyp   += ceil( double(nBlockTyp) / 240 );
            }
            ++map;
        }

        totalBlocks += nBlockTyp;
        totalFATs   += nFATTyp;

        /////////////////////////////////////////////////////////////
        // add map strings and keys to mapsourc.mps
        /////////////////////////////////////////////////////////////
        map = maps.begin();
        while(map != maps.end())
        {
            mps << quint8('F');
            mps << quint16(5 + map->map.toLatin1().size());
            // I suspect this should really be the basic file name of the .img set:
            mps << map->fid << map->pid;

            mps.writeRawData(map->map.toLatin1(),map->map.toLatin1().size());
            mps.writeRawData("\0",1);

            if(!map->key.isEmpty())
            {
                mps << (quint8)'U' << (quint16)26;
                mps.writeRawData(map->key.toLatin1(),26);
            }
            ++map;
        }

        // add mapsourc.mps to block and FAT counters
        quint32 nBlockMps  = ceil( double(mapsourc.size()) / blocksize );
        quint32 nFATMps    = ceil( double(nBlockMps) / 240 );
        totalBlocks       += nBlockMps;
        totalFATs         += nFATMps;

        // calculate blocks used for FAT
        quint32 nBlocksFat = ceil(double(sizeof(gmapsupp_imghdr_t) + totalFATs * sizeof(CGarminTile::FATblock_t)) / blocksize);
        totalBlocks       += nBlocksFat;

        /////////////////////////////////////////////////////////////
        // a small sanity check
        /////////////////////////////////////////////////////////////
        quint32 filesize   = totalBlocks * blocksize;
        quint32 dataoffset = nBlocksFat * blocksize;

        if(totalFATs > maxFATs)
        {
            writeStderr(tr("FAT entries: %1 (of %2) Failed!").arg(totalFATs).arg(maxFATs));
            throw exce_t(errLogic, tr("Too many tiles."));
        }
        else
        {
            writeStdout(tr("FAT entries: %1 (of %2) ").arg(totalFATs).arg(maxFATs));
        }

        if(totalBlocks >= 65536)
        {
            writeStderr(tr("Block count: %1 (of %2) Failed!").arg(totalBlocks).arg(65536));
            throw exce_t(errLogic, tr("Too many tiles."));
        }
        else
        {
            writeStdout(tr("Block count: %1 (of %2)").arg(totalBlocks).arg(65536));
        }

        if(filesize > maxFileSize)
        {
            writeStderr(tr("File size: %1 MB (of %2 MB) Failed!").arg(double(filesize) / (1024 * 1024), 0, 'f', 2).arg(double(maxFileSize) / (1024 * 1024), 0, 'f', 2));
            throw exce_t(errLogic, tr("Too many tiles."));
        }
        else
        {
            writeStdout(tr("File size: %1 MB (of %2 MB)").arg(double(filesize) / (1024 * 1024), 0, 'f', 2).arg(double(maxFileSize) / (1024 * 1024), 0, 'f', 2));
        }

        /////////////////////////////////////////////////////////////
        // start to create the file
        /////////////////////////////////////////////////////////////
        if(filename.isEmpty())
        {
            QDir path(labelPath->text());
            filename = path.filePath(linePrefix->text() + ".img");
        }
        QFile gmapsupp(filename);
        gmapsupp.open(QIODevice::WriteOnly);

        // initialize the complete file with 0xFF
        writeStdout(tr("Initialize %1").arg(gmapsupp.fileName()));
        QByteArray dummyblock(blocksize, (char)0xFF);
        for(i = 0; i < totalBlocks; ++i)
        {
            gmapsupp.write(dummyblock);
        }

        /////////////////////////////////////////////////////////////
        // write Garmin file header
        /////////////////////////////////////////////////////////////
        writeStdout(tr("Write header..."));
        gmapsupp_imghdr_t gmapsupp_imghdr;
        initGmapsuppImgHdr(gmapsupp_imghdr, totalBlocks, dataoffset);
        gmapsupp.seek(0);
        gmapsupp.write((char*)&gmapsupp_imghdr, sizeof(gmapsupp_imghdr));

        // write FAT entry that defines the FAT table
        QByteArray FATblock(sizeof(FATblock_t), (char)0x0FF);
        FATblock_t * pFAT = (FATblock_t*)FATblock.data();
        initFATBlock(pFAT);

        pFAT->size = gar_endian(uint32_t, dataoffset);
                                 //???
        pFAT->part = gar_endian(uint16_t, 3);

        for(i = 0; i < nBlocksFat; ++i)
        {
            pFAT->blocks[i] = gar_endian(uint16_t, blockcnt++);
        }
        gmapsupp.write(FATblock);

        /////////////////////////////////////////////////////////////
        // write all FAT entries for map data
        /////////////////////////////////////////////////////////////
        quint32 newOffset = dataoffset;

        tile = tiles.begin();
        while(tile != tiles.end())
        {
            QMap<QString, gmapsupp_subfile_desc_t>& subfiles          = tile->subfiles;
            QMap<QString,gmapsupp_subfile_desc_t>::iterator subfile   = subfiles.begin();
            while(subfile != subfiles.end())
            {
                QMap<QString, gmapsupp_subfile_part_t>& parts         = subfile->parts;
                QMap<QString, gmapsupp_subfile_part_t>::iterator part = parts.begin();
                while(part != parts.end())
                {
                    quint16 partno   = 0;
                    quint16 blockidx = 0;

                    initFATBlock(pFAT);
                    memcpy(pFAT->name, subfile.key().toLatin1(), sizeof(pFAT->name));
                    memcpy(pFAT->type, part.key().toLatin1(), sizeof(pFAT->type));
                    pFAT->size = gar_endian(uint32_t, part->size);
                    pFAT->part = gar_endian(uint16_t, partno++ << 8);

                    for(i = 0; i < part->nBlocks; ++i, ++blockidx)
                    {
                        if(blockidx == 240)
                        {
                            gmapsupp.write(FATblock);
                            initFATBlock(pFAT);
                            memcpy(pFAT->name, subfile.key().toLatin1(), sizeof(pFAT->name));
                            memcpy(pFAT->type, part.key().toLatin1(), sizeof(pFAT->type));
                            pFAT->size  = 0;
                            pFAT->part  = gar_endian(uint16_t, partno++ << 8);
                            blockidx    = 0;
                        }
                        pFAT->blocks[blockidx] = gar_endian(uint16_t, blockcnt++);
                    }
                    gmapsupp.write(FATblock);

                    part->newOffset  = newOffset;
                    newOffset       += part->nBlocks * blocksize;
                    ++part;
                }
                ++subfile;
            }
            ++tile;
        }

        /////////////////////////////////////////////////////////////
        // write MAPSOURCMPS FAT entries
        /////////////////////////////////////////////////////////////
        quint16 partno          = 0;
        quint16 blockidx        = 0;
        quint32 newMpsOffset    = newOffset;

        initFATBlock(pFAT);
        memcpy(pFAT->name, "MAPSOURC", sizeof(pFAT->name));
        memcpy(pFAT->type, "MPS", sizeof(pFAT->type));
        pFAT->size = gar_endian(uint32_t, mapsourc.size());
        pFAT->part = gar_endian(uint16_t, partno++ << 8);

        for(i = 0; i < nBlockMps; ++i, ++blockidx)
        {
            if(blockidx == 240)
            {
                gmapsupp.write(FATblock);
                initFATBlock(pFAT);
                memcpy(pFAT->name, "MAPSOURC", sizeof(pFAT->name));
                memcpy(pFAT->type, "MPS", sizeof(pFAT->type));
                pFAT->size  = 0;
                pFAT->part  = gar_endian(uint16_t, partno++ << 8);
                blockidx    = 0;
            }
            pFAT->blocks[blockidx] = gar_endian(uint16_t, blockcnt++);
        }
        gmapsupp.write(FATblock);
        newOffset += nBlockMps * blocksize;

        /////////////////////////////////////////////////////////////
        // write typfile FAT blocks
        /////////////////////////////////////////////////////////////
        map = maps.begin();
        while(map != maps.end())
        {
            if(!map->typ.isEmpty())
            {
                QFileInfo fi(map->typ);
                quint32 nBlock = ceil(double(fi.size()) / blocksize);

                quint16 partno   = 0;
                quint16 blockidx = 0;

                initFATBlock(pFAT);
                memcpy(pFAT->name, fi.baseName().toLatin1(), sizeof(pFAT->name));
                memcpy(pFAT->type, "TYP", sizeof(pFAT->type));
                pFAT->size = gar_endian(uint32_t, fi.size());
                pFAT->part = gar_endian(uint16_t, partno++ << 8);
                for(i = 0; i < nBlock; ++i, ++blockidx)
                {
                    if(blockidx == 240)
                    {
                        gmapsupp.write(FATblock);
                        initFATBlock(pFAT);
                        memcpy(pFAT->name, fi.baseName().toLatin1(), sizeof(pFAT->name));
                        memcpy(pFAT->type, "TYP", sizeof(pFAT->type));
                        pFAT->size  = 0;
                        pFAT->part  = gar_endian(uint16_t, partno++ << 8);
                        blockidx    = 0;
                    }
                    pFAT->blocks[blockidx] = gar_endian(uint16_t, blockcnt++);
                }
                gmapsupp.write(FATblock);

                map->newTypOffset = newOffset;
                newOffset        += nBlock * blocksize;

            }
            ++map;
        }

        /////////////////////////////////////////////////////////////
        // write map data
        /////////////////////////////////////////////////////////////
        writeStdout(tr("Copy tile data..."));
        tile = tiles.begin();
        while(tile != tiles.end())
        {
            quint8 mask;

            writeStdout(tr("    Copy %1...").arg(tile->name));

            QFile file(tile->filename);
            file.open(QIODevice::ReadOnly);
            file.read((char*)&mask,1);

            QMap<QString, gmapsupp_subfile_desc_t>& subfiles          = tile->subfiles;
            QMap<QString,gmapsupp_subfile_desc_t>::iterator subfile   = subfiles.begin();
            while(subfile != subfiles.end())
            {
                QMap<QString, gmapsupp_subfile_part_t>& parts         = subfile->parts;
                QMap<QString, gmapsupp_subfile_part_t>::iterator part = parts.begin();
                while(part != parts.end())
                {
                    QByteArray data;
                    readFile(file, part->offset, part->size, data, mask);

                    gmapsupp.seek(part->newOffset);
                    gmapsupp.write(data);

                    ++part;
                }
                ++subfile;
            }
            ++tile;
        }

        /////////////////////////////////////////////////////////////
        // Copy typ file data
        /////////////////////////////////////////////////////////////
        writeStdout(tr("Copy typ files..."));
        map = maps.begin();
        while(map != maps.end())
        {
            if(!map->typ.isEmpty())
            {
                writeStdout(map->typ);
                QFile file(map->typ);
                file.open(QIODevice::ReadOnly);
                QByteArray data = file.readAll();
                file.close();

                gmapsupp.seek(map->newTypOffset);
                gmapsupp.write(data);
            }
            ++map;
        }

        /////////////////////////////////////////////////////////////
        // Write MAPSORCMPS
        /////////////////////////////////////////////////////////////
        writeStdout(tr("Write map lookup table..."));
        gmapsupp.seek(newMpsOffset);
        gmapsupp.write(mapsourc);

        gmapsupp.close();
        //         QFile::remove("gmapsupp_cpy.img");
        //         gmapsupp.copy("gmapsupp_cpy.img");
    }
    catch(const exce_t e)
    {
        writeStderr(e.msg);
        writeStdout(tr("Abort due to errors."));
        errors = true;
    }

    writeStdout("----------");

    if(isDialog)
    {
        pushClose->setEnabled(true);
    }
}
