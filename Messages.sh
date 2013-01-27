#! /bin/sh
$EXTRACTRC `find . -name \*.rc -o -name \*.ui -o -name \*.kcfg` >> rc.cpp
$XGETTEXT `find . -name \*.cpp -o -name \*.h` -o $podir/audex.pot
rm -f rc.cpp
