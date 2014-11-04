#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(wclang, 0.5, t.poechtrager@gmail.com)

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_SRCDIR([src/wclang.cpp])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([foreign])

# Checks for programs.
m4_define([mingw_path], esyscmd([scripts/check_mingw.sh]))
ifelse(m4_sysval, 0, , [echo "mingw_path"; exit 1])

m4_define([clang_target_opt], m4_esyscmd([scripts/check_clang_target_option.sh]))

ifelse(m4_sysval, 0, , [
    echo "unable to determine clang target option"
    echo "clang not installed?"
    exit 1])

CLANG_TARGET_OPT="-DCLANG_TARGET_OPT="
CLANG_TARGET_OPT="${CLANG_TARGET_OPT}\"\\\""
CLANG_TARGET_OPT="${CLANG_TARGET_OPT}"clang_target_opt
CLANG_TARGET_OPT="${CLANG_TARGET_OPT}\\\"\""

MINGW_PATH="-DMINGW_PATH="
MINGW_PATH="${MINGW_PATH}\"\\\""
MINGW_PATH="${MINGW_PATH}"mingw_path
MINGW_PATH="${MINGW_PATH}\\\"\""

AC_SUBST(CLANG_TARGET_OPT)
AC_SUBST(MINGW_PATH)

ifelse(m4_sysval, 0, , [
    echo "unable to determine clang target option"
    echo "clang not installed?"
    exit 1])

AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_AWK
AC_PROG_MKDIR_P
AX_CXX_COMPILE_STDCXX_11(ext, mandatory)
AX_CXX_STDCHRONO

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([strchr strdup strrchr getpid], , exit 1)
AC_CHECK_FUNCS([execvp gettimeofday setenv getenv], , exit 1)
AC_CHECK_FUNCS([opendir fork], , exit 1)

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
