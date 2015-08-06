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

#include "CFileGenerator.h"
#include "argv.h"

#include <gdal_priv.h>
#include <stdio.h>
#include <stdlib.h>
#include <wctype.h>

#include <QtCore>


#ifndef _MKSTR_1
#define _MKSTR_1(x)         #x
#define _MKSTR(x)           _MKSTR_1(x)
#endif

#define VER_STR             _MKSTR(VER_MAJOR)"."_MKSTR(VER_MINOR)"."_MKSTR(VER_STEP)
#define WHAT_STR            "map2rmp, Version " VER_STR

int main(int argc, char ** argv)
{
    int quality                 = -1;
    int subsampling             = -1;
    bool intermediateLevels     = false;
    int skip_next_arg           =  0;

    QString provider;
    QString product;
    QString copyright;


    QStringList input;


    printf("\n****** %s ******\n", WHAT_STR);

    if(argc < 2)
    {
        fprintf(stderr,"\nusage: map2rpm -p <string> -n <string> -c <string> -q <1..100> -s <411|422|444> -i  <file1> <file2> ... <fileN> <outputfile>\n");
        fprintf(stderr,"\n");
        fprintf(stderr,"  -p    The map provider as string (mandatory)\n");
        fprintf(stderr,"  -n    The map name as string (mandatory)\n");
        fprintf(stderr,"  -c    The copyright notice (optional)\n");
        fprintf(stderr,"  -q    The JPEG quality from 1 to 100. Default is 75\n");
        fprintf(stderr,"  -s    The chroma subsampling. Default is 411\n");
        fprintf(stderr,"  -i    Add intermediate levels (optional)\n");
        fprintf(stderr,"\n");
        fprintf(stderr,"NOTE: The projection of all input files must be EPSG4326. You can use GDAL to convert\n");
        fprintf(stderr,"      your files. Use 'gdalinfo <file>' to find out the size in pixel of your file.\n");
        fprintf(stderr,"      Use gdalwarp to reproject your file:\n");
        fprintf(stderr,"\n");
        fprintf(stderr,"      gdalwarp -t_srs \"+init=epsg:4326\" -ts <width in pixel> <height in pixel> <file> <outputfile>\n");
        fprintf(stderr,"\n");
        exit(-1);
    }

    GDALAllRegister();

    for(int i = 1; i < (argc - 1); i++)
    {

        if (skip_next_arg)
        {
            skip_next_arg--;
            continue;
        }

        if (argv[i][0] == '-')
        {
            if (towupper(argv[i][1]) == 'Q')
            {
                quality = atol(argv[i+1]);
                skip_next_arg = 1;
                continue;
            }
            else if (towupper(argv[i][1]) == 'S')
            {
                subsampling = atol(argv[i+1]);
                skip_next_arg = 1;
                continue;
            }
            else if (towupper(argv[i][1]) == 'P')
            {
                provider = get_argv(i + 1, argv);
                skip_next_arg = 1;
                continue;
            }
            else if (towupper(argv[i][1]) == 'C')
            {
                copyright = get_argv(i + 1, argv);
                skip_next_arg = 1;
                continue;
            }
            else if (towupper(argv[i][1]) == 'N')
            {
                product = get_argv(i + 1, argv);
                skip_next_arg = 1;
                continue;
            }
            else if (towupper(argv[i][1]) == 'I')
            {
                intermediateLevels = true;
                continue;
            }



        }

        input << argv[i];
    }

    if(product.isEmpty() || provider.isEmpty())
    {
        fprintf(stderr,"\nYou must give a provider and product name!\nCall map2rmp without arguments for help.\n\n");
        exit(-1);
    }

    CFileGenerator generator(input, argv[argc-1], provider, product, copyright, quality, subsampling, intermediateLevels);
    generator.start();

    GDALDestroyDriverManager();
    printf("\n");

    return 0;
}
