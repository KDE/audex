#! /bin/sh
# GUI (KI18n, gettext); the labels in audex.kcfg are not shown anywhere
$EXTRACTRC `find . -name \*.rc -o -name \*.ui | grep -v '^\./tests/'` >> rc.cpp
$XGETTEXT `find . -name \*.cpp -o -name \*.h | grep -v '^\./tests/'` -o $podir/audex.pot
rm -f rc.cpp
# engine, encoder plugins and audex-cli (Qt tr)
$EXTRACT_TR_STRINGS `find . -name \*.cpp -o -name \*.h | grep -v '^\./tests/'` -o $podir/audex_qt.pot
