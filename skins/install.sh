#!/bin/sh

SOURCES="*.skin tabs title"

displayHelp()
{
    clear
    echo "This script installs the skins into the correct directory"
    echo
    echo "  + To install them in your personal directories, type :"
    echo "      ./install.sh --user"
    echo
    echo "  + To install them in the system directories, type :"
    echo "      ./install.sh --system"
    echo
}

userInstall()
{
    DATA_DIR=`kde-config --localprefix`'share/apps/yakuake/default/'

    mkdir -p $DATA_DIR && cp -Rf $SOURCES $DATA_DIR
}

systemInstall()
{
    DATA_DIR=`kde-config --install data`'/yakuake/default/'

    su -c "mkdir -p $DATA_DIR && cp -Rf $SOURCES $DATA_DIR && chmod -R 755 $DATA_DIR"
}

if [ $1 == "--user" ]; then
    userInstall
    exit 0
fi

if [ $1 == "--system" ]; then
    systemInstall
    exit 0
fi

displayHelp
exit 1
