#!/bin/sh

if [ -z "$1" ] ; then
	echo "usage: $0 <plist file>"
	exit 1
fi
if [ ! -f "$1" ] ; then
	echo "bad input file: $1"
	exit 2
fi

TMPFILE=$(mktemp /tmp/qlgt.plist.XXXXXXXXXX) || exit 3

(
cat "$1" | \
	tr '\n' '#' | \
	sed -e 's;\(CFBundleSignature</key>[^<]*<string>\)....\(</string>\);\1QLGT\2;' -e 's;#</dict>#</plist>#$;;' | \
	tr '#' '\n'

read -r -d '' TYPES <<-'EOF'
	gpx:GPS exchange data
	qlb:QLandkarte data
	tcx:TrainingsCenterExchange data
	gdb:MapSource data
	kml:Google Earth (Keyhole) data
	loc:Geocaching.com - EasyGPS data
	plt:OziExplorer track
	rte:OziExplorer route
	wpt:OziExplorer waypoint
EOF

echo -e '\t<key>CFBundleDocumentTypes</key>\n\t<array>'
IFS='
'
for n in $TYPES ; do
	echo -e '\t\t<dict>'
	EXT=${n%%:*}
	NAME=${n##*:}
	echo -e "\t\t\t<key>CFBundleTypeExtensions</key>\n\t\t\t<array>\n\t\t\t\t<string>$EXT</string>\n\t\t\t</array>"
	echo -e "\t\t\t<key>CFBundleTypeName</key>\n\t\t\t<string>$NAME</string>"
	echo -e "\t\t\t<key>CFBundleTypeIconFile</key>\n\t\t\t<string>qlandkartegt-${EXT}.icns</string>"
	echo -e '\t\t</dict>'
done
unset IFS
echo -e '\t</array>\n</dict>\n</plist>'
) > "$TMPFILE"

rm -f "$1"
cp "$TMPFILE" "$1"
rm -f "$TMPFILE"

