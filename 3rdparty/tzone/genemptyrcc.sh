#!/bin/sh

echo removing old zone info
rm -rf zoneinfo*

echo "empty zone" >zoneinfo

ZF=zonefiles.qrc

cat <<EOF >$ZF
<!DOCTYPE RCC>
<RCC version="1.0">
<qresource>
 <file>zoneinfo</file>
</qresource>
</RCC>
EOF

echo generating source...
rcc $ZF -name ${ZF%.qrc} -o ${ZF%.qrc}.cpp

echo Done.
