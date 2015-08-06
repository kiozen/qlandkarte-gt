# NOTE: QLandkarte GT development came to an end. The future is [QMapShack](https://bitbucket.org/maproom/qmapshack/wiki/Home)#

## Compile Instructions ##

You need a working mercurial, g++, cmake and QT4 installation to compile QLandkarte GT on your computer. Clone and compile the code base by::

    hg clone https://bitbucket.org/kiozen/qlandkarte-gt QLandkarteGT
    mkdir build_QLandkarteGT
    cd build_QLandkarteGT
    ccmake ../QLandkarteGT
    make

To update the code to the cutting edge do in QLandkarte GT::

    hg pull
    hg update

And change back to build_QLandkarteGT::

    make

As everything is better with a GUI you might want to have a look at TortoiseHg.

Dependencies

Next to QT4 you will need the development packages of::

    Proj4
    GDAL
    libexif
    gpsd