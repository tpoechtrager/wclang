#!/bin/sh
./cleanup.sh
aclocal -I m4 && automake --add-missing && autoconf
