#!/bin/sh

echo removing old zone info
rm -rf zoneinfo*

echo regenerating zone info
make -C db

ZF=zonefiles.qrc

cat <<EOF >$ZF
<!DOCTYPE RCC>
<RCC version="1.0">
<qresource>
EOF

for i in zoneinfo* ; do
	echo Scanning $i
	echo '<file alias="'$i'/+VERSION">db/version.txt</file>' >>$ZF
	for f in `find $i -type f` ; do
		echo '<file>'$f'</file>' >>$ZF
	done
done

cat <<EOF >>$ZF
</qresource>
</RCC>
EOF

echo generating source...
rcc $ZF -name ${ZF%.qrc} -o ${ZF%.qrc}.cpp

echo Done.
