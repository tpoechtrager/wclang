
AU_ALIAS([AC_CXX_STDCHRONO], [AX_CXX_STDCHRONO])
AC_DEFUN([AX_CXX_STDCHRONO],
[AC_CACHE_CHECK(whether std::chrono is supported,
ax_cv_cxx_stdchrono,
[AC_LANG_SAVE
 AC_LANG_CPLUSPLUS
  AC_TRY_LINK(
  [#include <chrono>],
  [std::chrono::steady_clock::now();],
                 [ax_cv_cxx_stdchrono=yes],
                 [ax_cv_cxx_stdchrono=no])
 AC_LANG_RESTORE
])
if test "$ax_cv_cxx_stdchrono" = yes; then
  AC_DEFINE(HAVE_STD_CHRONO,1,[define if std::chrono is suppored])
fi
])
