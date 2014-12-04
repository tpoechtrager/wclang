#!/usr/bin/env bash

dir=`pwd`
packagetmp=`mktemp -d`
cp -r . $packagetmp || exit $?
pushd $packagetmp >/dev/null
./cleanup.sh 2>/dev/null
rm -rf .git 2>/dev/null
rm wclang.tar.xz 2>/dev/null
XZ_OPT=-9 tar cJf $dir/wclang.tar.xz * || exit $?
popd >/dev/null
rm -rf $packagetmp || exit $?
