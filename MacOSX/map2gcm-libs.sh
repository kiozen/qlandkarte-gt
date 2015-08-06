#!/bin/sh
# $Id:$

if [ -z "$1" ] ; then
    echo "usage: $0 <bundle path>"
    exit 1
fi

MAP2GCM=$1/Contents/Resources/map2gcm
QTLIBS=$(otool -L "$MAP2GCM" | sed -n -e 's/^[[:space:]]*\(Qt[^ ]*\).*$/\1/p')
for l in $QTLIBS ; do
    install_name_tool -change "$l" "@executable_path/../Frameworks/$l" "$MAP2GCM"
done

