#!/usr/bin python
# -*- coding: utf-8 -*-
"""Merge hgt files to a single srtm.tif with given projection.
@author Michael Rastetter
"""

import os
from subprocess import call


def find_hgt(basedir):
    """Search for '*.hgt' files in `basedir`.
    @param basedir : Directory to start search for files.
    @return List of strings with full path to hgt files.
    """
    print('Search for hgt files in: %s' % basedir)
    hgtfiles = []
    for root, _dirs, files in os.walk(basedir, topdown=False):
        for name in files:
            if os.path.splitext(name)[1].lower() != '.hgt':
                continue
            filename = os.path.join(root, name)
            hgtfiles.append(filename)
    return hgtfiles


def reproject(hgtfiles, projection):
    """Merge hgt files, reproject them and translate to srtm.tif.
    @param hgtfiles : List of strings with full paths to hgt files.
    @param projection : Projection, ellipsoid and datum of target file, string.
    """
    cmd = 'gdalwarp -r cubic ' + ' '.join(hgtfiles) + ' _merge.tif'
    call(cmd, shell=True)
    projection += ' +lon_0=0 +k=1 +x_0=0 +y_0=0 +units=m +no_defs'
    cmd = 'gdalwarp -t_srs "%s" -r cubic _merge.tif _repro.tif' % projection
    call(cmd, shell=True)
    os.remove('_merge.tif')
    cmd = 'gdal_translate -co tiled=yes -co blockxsize=256 -co blockysize=256 ' \
        '-co compress=deflate -co predictor=1 _repro.tif srtm.tif'
    call(cmd, shell=True)
    os.remove('_repro.tif')


def main():
    """Find hgt files in current dir and reproject them to a GeoTiff file.
    """
    hgtfiles = find_hgt('.')
    projection = '+proj=utm +zone=32 +ellps=WGS84 +datum=WGS84'
    reproject(hgtfiles, projection)


if __name__ == '__main__':
    main()
