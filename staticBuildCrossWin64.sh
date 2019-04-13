#!/bin/bash

#
# Copyright (C) 2018-2019 QuasarApp.
# Distributed under the lgplv3 software license, see the accompanying
# Everyone is permitted to copy and distribute verbatim copies
# of this license document, but changing it is not allowed.
#

IS_SNAP="$1"
QT_DIR=staticQtWin64

if [ -e "$PREFIX"]
then
    echo "build for snap"
    SNAP_DEFINES="$IS_SNAP"
fi

declare -a QTLIBS

BASE_DIR=$(dirname "$(readlink -f "$0")")
QTLIBS=( libQt5Sql.a libQt5Xml.a libQt5Core.a libQt5Test.a libQt5Network.a libQt5Bootstrap.a libQt5Concurrent.a)

RELEASE_DIR=$BASE_DIR/distro

if [ -e "$PREFIX"]
then
    echo "PREFIX is empty, use default install path $RELEASE_DIR"
else
    echo "use PREFIX path!"
    RELEASE_DIR=$PREFIX
fi

cd $BASE_DIR

#git submodule update --init --recursive

make clean
find $BASE_DIR -type f -name 'Makefile' -exec rm {} \;
rm $BASE_DIR/QuasarAppLib/Makefile.QuasarApp
rm -rdf $RELEASE_DIR

cd $BASE_DIR/qtBase

for var in "${QTLIBS[@]}"
do
        if [ -e "$BASE_DIR/$QT_DIR/lib/$var" ]
	then
	    echo "$var - ok"
	else
	    echo "$var - not exits!. rebuild qt ..."
            rm -rdf $BASE_DIR/$QT_DIR
            git clean -xdf
            git checkout v5.12.2
            ./configure -xplatform win32-g++ -device-option CROSS_COMPILE=x86_64-w64-mingw32- -confirm-license -prefix $BASE_DIR/$QT_DIR -release -optimize-size -static -no-opengl -no-openssl -opensource -nomake tests -nomake examples -no-gui -no-widgets -no-dbus -no-accessibility    
            make -j$(nproc)
            make install
            break
	fi
done
	
cd ..
rm -rdf $BASE_DIR/build

export PATH=$PATH:$BASE_DIR/$QT_DIR
$BASE_DIR/$QT_DIR/bin/qmake QMAKE_LFLAGS+="-static" -r $BASE_DIR/CQtDeployer.pro

make -j$(nproc)

if [ $? -eq 0 ]
then    
    echo ""
    echo "Build is successfully completed!"
else
    echo ""
    echo "Build is failed!" >&2
    exit 1
fi

mkdir -p $RELEASE_DIR

cp $BASE_DIR/build/release/cqtdeployer $RELEASE_DIR/

strip $RELEASE_DIR/*

