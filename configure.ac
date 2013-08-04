#                                              -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([webcontrol-squid-helper], [0.1], [nischu7@nischu7.com])
#AC_CONFIG_SRCDIR([main.c])
#AC_CONFIG_HEADERS([config.h])
AM_CONFIG_HEADER([src/core/config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([])
#AC_CONFIG_FILES([Makefile])
AC_OUTPUT(Makefile src/Makefile src/core/Makefile src/thirdparty/Makefile src/thirdparty/c-pthread-queue/Makefile src/plugins/Makefile src/plugins/mod_activedirectory/Makefile src/plugins/mod_memcached/Makefile src/plugins/mod_txtfile/Makefile)

dnl libtool
AC_DISABLE_STATIC
AC_ENABLE_SHARED

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL
AC_LIBTOOL_DLOPEN

# Checks for headers
AC_CHECK_HEADERS([ev.h]     , , [AC_MSG_ERROR([Missing libev headers... :(])])
AC_CHECK_HEADERS([confuse.h], , [AC_MSG_ERROR([Missing libconfuse headers... :(])])

# Checks for libraries.
AC_CHECK_LIB([pthread], pthread_create)
AC_CHECK_LIB([ev], [ev_io_start])
AC_CHECK_LIB([confuse], [cfg_init])
AC_CHECK_LIB([dl], [dlopen])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_C_CHAR_UNSIGNED
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T

dnl Checks for library functions.
#AC_FUNC_FORK
#AC_FUNC_MALLOC
#AC_FUNC_MMAP
#AC_FUNC_REALLOC
#AC_TYPE_SIGNAL
#AC_FUNC_STAT
#AC_FUNC_STRFTIME
AC_CHECK_FUNCS([inet_pton])



dnl Check for modules

modules=""
AC_ARG_WITH(modules, [  --with-modules Which modules to compile with ], 
[
        modules="$withval"  
])


AC_SUBST(moduledirs)
#AC_SUBST(modulemakefiles)
for a in $modules
do
	moduledir="mod_${a}"

	moduledirs="$moduledirs ${moduledir}"
	#modulemakefiles="$modulemakefiles src/plugins/${moduledir}/Makefile"
done

export moduledirs
#export modulemakefiles

dnl Check for IPv6 support

AC_ARG_ENABLE(ipv6,
 AC_HELP_STRING([--disable-ipv6],[disable IPv6 support]),
 [case "${enableval}" in
   yes) ipv6=true ;;
    no) ipv6=false ;;
     *) AC_MSG_ERROR(bad value ${enableval} for --enable-ipv6) ;;
  esac],[ipv6=true])

if test x$ipv6 = xtrue; then
  AC_CACHE_CHECK([for IPv6 support], ac_cv_ipv6_support,
  [AC_TRY_LINK([ #include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>], [struct sockaddr_in6 s; struct in6_addr t=in6addr_any; int i=AF_INET6; s; t.s6_addr[0] = 0; ],
  [ac_cv_ipv6_support=yes], [ac_cv_ipv6_support=no])])

  if test "$ac_cv_ipv6_support" = yes; then
    AC_DEFINE(HAVE_IPV6,1,[Whether to enable IPv6 support])
  fi
fi

#AC_MSG_CHECKING(whether we will be doing verbose logging)
#AC_ARG_ENABLE(verbose-logging, 
# [  --enable-verbose-logging	Do verbose logging],enable_verbose_logging=yes ,enable_verbose_logging=no)

#AC_MSG_CHECKING(whether we will support request multiplexing)
#AC_ARG_ENABLE(request-multiplexing, 
# [  --disable-multiplexing	Do request multiplexing],enable_request_multiplexing=yes ,enable_request_multiplexing=no)


AC_ARG_ENABLE(debug,
AS_HELP_STRING([--enable-debug],
               [enable debugging, default: no]),
[case "${enableval}" in
             yes) debug=true ;;
             no)  debug=false ;;
             *)   AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
esac],
[debug=false])

AM_CONDITIONAL(DEBUG, test x"$debug" = x"true")

if test x"$debug" = x"true"; then
	AC_DEFINE(HAVE_DEBUG,1,[Whether this is a debug build])
fi

#AC_MSG_ERROR(moduledirs)

#AC_OUTPUT(Makefile src/Makefile src/core/Makefile src/plugins/Makefile $modulemakefiles)

CFLAGS="${CFLAGS} -Wall -std=gnu99"

AC_OUTPUT

