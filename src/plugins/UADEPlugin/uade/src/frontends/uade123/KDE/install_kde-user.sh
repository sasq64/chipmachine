#! /bin/sh

PREFIX=`kde-config --localprefix`

if test -x "$PREFIX"; then
    echo -e "installing amiga music mimetype to:\n $PREFIX/share/mimelnk/audio/\n"
    mkdir -p $PREFIX/share/mimelnk/audio/
    cp -rf ./mimelnk/audio/x-amiga.desktop $PREFIX/share/mimelnk/audio/

    echo -e "installing uade123 application type to:\n $PREFIX/share/applnk/Multimedia/\n"
    mkdir -p $PREFIX/share/applnk/Multimedia/
    cp -rf ./applnk/Multimedia/uade123.desktop $PREFIX/share/applnk/Multimedia/
    
fi
