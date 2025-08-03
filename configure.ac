dnl Process this file with autoconf to produce a configure script.
AC_INIT([LuminariMUD],[1.0],[bugs@luminarimud.com])
AC_CONFIG_SRCDIR([src/act.comm.c])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_SUBST(MYFLAGS)
AC_SUBST(NETLIB)
AC_SUBST(CRYPTLIB)

AC_CONFIG_HEADERS([src/conf.h:conf.h.in])
AC_DEFINE([CIRCLE_UNIX], [1], [Define for Unix-based systems])

dnl Find the 'more' program
AC_CHECK_PROGS(MORE, less most more cat)

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX

dnl If we're using gcc, use gcc options.
dnl If not, test for various common switches to make a 'cc' compiler
dnl compile ANSI C code.
if test "x$GCC" = xyes; then

  dnl Determine if gcc -Wall causes warnings on isascii(), etc.
  AC_CACHE_CHECK(whether ${CC-cc} -Wall also needs -Wno-char-subscripts,
	ac_cv_char_warn,
  [
    OLDCFLAGS=$CFLAGS
    CFLAGS="$CFLAGS -Wall -Werror"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <ctype.h>]],
       [[ int i; char c = '0';
         i = isascii(c);
         i = isdigit(c);
         i = isprint(c);
       ]])], [ac_cv_char_warn=no], [ac_cv_char_warn=yes])
    CFLAGS=$OLDCFLAGS
  ])

  dnl If Determine if gcc can accept -Wno-char-subscripts
  AC_CACHE_CHECK(whether ${CC-cc} accepts -Wno-char-subscripts, ac_cv_gcc_ncs,
  [
    OLDCFLAGS=$CFLAGS
    CFLAGS="$CFLAGS -Wno-char-subscripts"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM()], [ac_cv_gcc_ncs=yes], [ac_cv_gcc_ncs=no])
    CFLAGS=$OLDCFLAGS
  ])

  dnl If Determine if gcc can accept -fno-builtin
  AC_CACHE_CHECK(whether ${CC-cc} accepts -fno-builtin, ac_cv_gcc_fnb,
  [
    OLDCFLAGS=$CFLAGS
    CFLAGS="$CFLAGS -fno-builtin"
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM()], [ac_cv_gcc_fnb=yes], [ac_cv_gcc_fnb=no])
    CFLAGS=$OLDCFLAGS
  ])

  dnl If gcc -Wall gives no warnings with isascii(), use "-Wall";
  dnl Otherwise, if gcc -Wall gives isascii warnings:
  dnl    If we can use -Wno-char-subscripts, use "-Wall -Wno-char-subscripts"
  dnl    If can't use -Wno-char-subscripts, use no flags at all.

  if test ${ac_cv_char_warn:-ERROR} = no; then
    MYFLAGS="-Wall"
  else
    if test ${ac_cv_gcc_ncs:-ERROR} = yes; then
      MYFLAGS="-Wall -Wno-char-subscripts"
    else
      MYFLAGS=""
    fi
  fi

else
  dnl We aren't using gcc so we can't assume any special flags.
  MYFLAGS=""

fi

dnl Checks for libraries.  We check for the library only if the function is
dnl not available without the library.
AC_CHECK_FUNC(gethostbyaddr, ,
    [AC_CHECK_LIB(nsl, gethostbyaddr, NETLIB="-lnsl $NETLIB")])

AC_CHECK_FUNC(socket, ,
    [AC_CHECK_LIB(socket, socket, NETLIB="-lsocket $NETLIB")])

AC_CHECK_FUNC(malloc, ,
    [AC_CHECK_LIB(malloc, malloc)])

AC_CHECK_FUNC(crypt, [AC_DEFINE([CIRCLE_CRYPT], [1], [Define if crypt() is available])],
    [AC_CHECK_LIB(crypt, crypt, [AC_DEFINE([CIRCLE_CRYPT], [1], [Define if crypt() is available]) CRYPTLIB="-lcrypt"])]
    )

dnl Checks for header files.
AC_CHECK_INCLUDES_DEFAULT
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h sys/fcntl.h errno.h net/errno.h string.h strings.h)
AC_CHECK_HEADERS(limits.h sys/time.h sys/select.h sys/types.h unistd.h)
AC_CHECK_HEADERS(memory.h crypt.h assert.h arpa/telnet.h arpa/inet.h)
AC_CHECK_HEADERS(sys/stat.h sys/socket.h sys/resource.h netinet/in.h netdb.h)
AC_CHECK_HEADERS(signal.h sys/uio.h mcheck.h)

dnl AC_UNSAFE_CRYPT removed - was a custom macro

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_CHECK_TYPE(ssize_t, int)
AC_CHECK_HEADERS([time.h sys/time.h])
AC_CHECK_DECLS([struct timeval],,,[#include <sys/time.h>])

dnl Check for the 'struct in_addr' definition. Ugly, yes.
if test $ac_cv_header_netinet_in_h = no; then
  ac_cv_struct_in_addr = no
else
  if test $ac_cv_header_sys_types_h = yes; then
    headers=`cat << EOF
#include <sys/types.h>
#include <netinet/in.h>
EOF
`
  else
    headers="#include <netinet/in.h>"
  fi

  AC_CACHE_CHECK([for struct in_addr], ac_cv_struct_in_addr,
    [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[$headers]],[[struct in_addr tp; tp.s_addr;]])], [ac_cv_struct_in_addr=yes], [ac_cv_struct_in_addr=no])])

  if test $ac_cv_struct_in_addr = yes; then
    AC_DEFINE([HAVE_STRUCT_IN_ADDR], [1], [Define if struct in_addr is available])
  fi
fi


dnl Check for the 'typedef socklen_t' definition. Even uglier, yes.
if test $ac_cv_header_sys_socket_h = no; then
  ac_cv_socklen_t = no;
else
  AC_CACHE_CHECK([for typedef socklen_t], ac_cv_socklen_t,
    [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/socket.h>]],[[socklen_t sl; sl=0;]])], [ac_cv_socklen_t=yes], [ac_cv_socklen_t=no])])
fi

if test $ac_cv_socklen_t = no; then
  AC_DEFINE([socklen_t], [int], [Define socklen_t as int if not available])
fi


dnl Checks for library functions.
AC_FUNC_VPRINTF
AC_CHECK_FUNCS(gettimeofday select snprintf strcasecmp strdup strerror stricmp strlcpy strncasecmp strnicmp strstr vsnprintf)

dnl Check for functions that parse IP addresses
ORIGLIBS=$LIBS
LIBS="$LIBS $NETLIB"
AC_CHECK_FUNCS(inet_addr inet_aton)
LIBS=$ORIGLIBS

dnl Check for prototypes - removed custom AC_CHECK_PROTO macros
dnl Modern compilers have standard prototypes for these functions

AC_CONFIG_FILES([Makefile util/Makefile])
AC_OUTPUT
#
echo "Configuration completed.  To compile, type:  make all"
