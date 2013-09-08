#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(wclang, 0.1, t.poechtrager@gmail.com)
AC_CONFIG_SRCDIR([src/wclang.cpp])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([foreign])

# Checks for programs.
m4_define([CLANG_TARGET_OPT], m4_esyscmd([scripts/check_clang_target_option.sh]))

ifelse(m4_sysval, 0, , [
    echo "unable to determine clang target option"
    echo "clang not installed?"
    exit 1])

CXXFLAGS+="-DCLANG_TARGET_OPT="
CXXFLAGS+="\"\\\""
CXXFLAGS+=CLANG_TARGET_OPT
CXXFLAGS+="\"\\\""

AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_AWK
AC_PROG_MKDIR_P
AX_CXX_COMPILE_STDCXX_11(noext, mandatory)
AX_CXX_STDCHRONO

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([strchr strdup strrchr])
AC_CHECK_FUNCS([execvp gettimeofday], , exit 1)
AC_CHECK_FUNCS([execvpe])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
