#!/bin/sh

export LC_ALL="C"

which clang 2>&1 1>/dev/null || exit 1

x=`clang -target i686-w64-mingw32 2>&1`

case "$x" in
    *i686-w64-mingw32*)
        echo "-ccc-host-triple"
        exit 0
    ;;
esac

echo "-target"

