#!/bin/sh
CLANG_BIN=`which clang`

test $? -eq 0 || {
    echo "install clang, then re-run ./bootstrap.sh"
    exit 1
}

CLANG_BIN=`readlink -f $CLANG_BIN`
CLANG_PATH=`dirname $CLANG_BIN`
MINGW_DIRS=""
MINGW_PATH=""
MINGW_INSTALLED=0
MINGW_WRONG_PATH=0

check_mingw_path()
{
    MINGW_GCC="$1-gcc"
    MINGW_BIN=`which $MINGW_GCC`

    if [ $? -eq 0 ]; then
        MINGW_INSTALLED=1
        MINGW_BIN=`readlink -f $MINGW_BIN`
        MINGW_PATH=`dirname $MINGW_BIN`

        if [ "$MINGW_DIRS" != "" ]; then
            MINGW_DIRS="$MINGW_DIRS:$MINGW_PATH"
        else
            MINGW_DIRS="$MINGW_PATH"
        fi

        if [ ! -f "$CLANG_PATH/$MINGW_GCC" ]; then
            MINGW_WRONG_PATH=1

            echo ""
            echo "$MINGW_GCC found but is not in the same directory as clang the binary!"
            echo "Please run 'ln -sf $MINGW_PATH/$1* $CLANG_PATH/' to fix this issue."
            echo ""
        fi
    fi
}

check_mingw_path i686-w64-mingw32
check_mingw_path x86_64-w64-mingw32
check_mingw_path i486-mingw32
check_mingw_path i586-mingw32
check_mingw_path i586-mingw32msvc
check_mingw_path amd64-mingw32msvc

test $MINGW_WRONG_PATH -eq 0 || {
    echo "then re-run ./bootstrap.sh"
    exit 1
}

if [ $MINGW_INSTALLED -eq 0 ]; then
    echo "mingw-w64 is not installed or not in PATH variable."
    exit 1
fi

echo $MINGW_DIRS
