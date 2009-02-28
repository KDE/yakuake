#! /usr/bin/env bash
$EXTRACTRC `find . -name \*.ui -o -name \*.rc -o -name \*.kcfg` >> rc.cpp
$XGETTEXT rc.cpp app/config/*.cpp app/*.cpp app/*.h -o $podir/yakuake.pot
rm -f rc.cpp
