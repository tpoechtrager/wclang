#!/bin/sh
./cleanup.sh
aclocal && automake --add-missing && autoconf
