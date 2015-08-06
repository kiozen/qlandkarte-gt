#!/bin/sh

set -e

cd `dirname $0`

echo Cleaning up.
rm -f tzdata*

echo Attempting to determine the newest version...
rsync -avz rsync://rsync.iana.org/tz/tzdata-latest.tar.gz .
latest=`readlink tzdata-latest.tar.gz`
rm -f tzdata-latest.tar.gz

echo Downloading version $latest ...
rsync -avz --progress rsync://rsync.iana.org/tz/$latest .
latest=`basename $latest`

echo Unpacking data...
gunzip <$latest | tar xv

echo Updating version info...
echo $latest |cut -d . -f 1|sed s/tzdata// >version.txt

echo Please regenerate the included zone info by calling genrcc.sh in the parent dir.
