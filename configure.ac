AC_PREREQ([2.53])
AC_INIT(rlm_memcached.c)
AC_REVISION($Revision$)
AC_DEFUN(modname,[rlm_memcached])

if test x$with_[]modname != xno; then

    AC_PROG_CC

    smart_try_dir=
    FR_SMART_CHECK_LIB(memcached, memcached)
    if test "x$ac_cv_lib_memcached_memcached" != "xyes"; then
        fail="$fail libmemcached"
    fi

    smart_try_dir=
    FR_SMART_CHECK_INCLUDE([libmemcached/memcached.h])
    if test "$ac_cv_header_libmemcached_memcached_h" != "yes"; then
        fail="$fail libmemcached/memcached.h"
    fi

    targetname=modname

else
    targetname=
    echo \*\*\* module modname is disabled.
fi

if test x"$fail" != x""; then
    if test x"${enable_strict_dependencies}" = x"yes"; then
        AC_MSG_ERROR([set --without-]modname[ to disable it explicitly.])
    else
        AC_MSG_WARN([silently not building ]modname[.])
        AC_MSG_ERROR([FAILURE: ]modname[ requires: $fail.]);
        targetname=""
    fi
fi

mod_ldflags=$SMART_LIBS
mod_cflags="$SMART_CPPFLAGS"

AC_SUBST(mod_ldflags)
AC_SUBST(mod_cflags)
AC_SUBST(targetname)
AC_OUTPUT(all.mk)
