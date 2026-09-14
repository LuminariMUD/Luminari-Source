dnl Process this file with autoconf to produce a configure script.
dnl Version: Keep in sync with src/constants.c, src/structs.h, CMakeLists.txt
AC_INIT([LuminariMUD],[2.5063-beta],[bugs@luminarimud.com])
AC_CONFIG_SRCDIR([src/act.comm.c])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_SUBST(MYFLAGS)
AC_SUBST(NETLIB)
AC_SUBST(CRYPTLIB)
AC_SUBST(CSTD_CFLAGS)

AC_CONFIG_HEADERS([src/conf.h:conf.h.in])

dnl Unknown configure options are fatal by default.  The production profile was
dnl once requested through an option configure.ac never defined, and the build
dnl silently used the default flags; a caller may still pass
dnl --enable-option-checking=no or =warn to override this deliberately.
m4_divert_text([DEFAULTS], [enable_option_checking=fatal])
AC_DEFINE([CIRCLE_UNIX], [1], [Define for Unix-based systems])

dnl Find the 'more' program
AC_CHECK_PROGS(MORE, less most more cat)

dnl Checks for programs.
dnl Feature detection must not depend on the warning policy.  Autoconf's own
dnl probes (AC_CHECK_FUNCS declares each function as "char f(void)") warn
dnl under -Wall and would flip to "not found" under a caller's -Werror,
dnl activating fallback definitions.  Keep -Werror out of every probe and put
dnl it back on the compile line at the end, after the last check.
dnl An unset CFLAGS must stay unset so AC_PROG_CC still supplies -g -O2.
luminari_user_cflags_set=${CFLAGS+set}
luminari_user_cflags=$CFLAGS
if test "x$luminari_user_cflags_set" = xset; then
  CFLAGS=`printf '%s\n' "$CFLAGS" | sed 's/\(^\| \)-Werror\(=[^ ]*\)\?\( \|$\)/ /g'`
fi
AC_PROG_CC
if test "x$luminari_user_cflags_set" != xset; then
  luminari_user_cflags=$CFLAGS
fi
PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES([LIBEVENT], [libevent_core >= 2.1.12], [],
  [AC_MSG_ERROR([libevent core 2.1.12-stable or newer is required; install the libevent development package])])

dnl Require GNU C23. Older compilers may expose the required feature set
dnl under the pre-publication gnu2x flag spelling.
AC_MSG_CHECKING([for the compiler flag for GNU C23])
OLD_CFLAGS=$CFLAGS
CSTD_CFLAGS=""
for cstd_flag in -std=gnu23 -std=gnu2x; do
  CFLAGS="$OLD_CFLAGS $cstd_flag"
  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([], [[
      bool value = true;
      void *ptr = nullptr;
      static_assert(sizeof(bool) > 0);
      return ptr == nullptr && value ? 0 : 1;
    ]])],
    [CSTD_CFLAGS=$cstd_flag; break],
    [])
done
CFLAGS=$OLD_CFLAGS

if test "x$CSTD_CFLAGS" = x; then
  AC_MSG_RESULT([not found])
  AC_MSG_ERROR([a compiler with GNU C23 support is required])
fi

AC_MSG_RESULT([$CSTD_CFLAGS])
if test "x$CSTD_CFLAGS" = "x-std=gnu2x"; then
  AC_MSG_NOTICE([using the compiler's legacy gnu2x spelling for GNU C23])
fi

dnl Warning policy.  The tiers live in scripts/deployment/production_profile.sh,
dnl which CMake calls too, so both builds apply the same probed flag set.
dnl Only the baseline tier may be combined with -Werror; the migration tier is
dnl held to a shrinking budget by scripts/ci/check_warning_budget.py instead.
AC_ARG_ENABLE([warning-tier],
  [AS_HELP_STRING([--enable-warning-tier=TIER],
    [warning tier: baseline (default), migration (budgeted), or analysis (scheduled diagnostics)])],
  [], [enable_warning_tier=baseline])
AC_ARG_ENABLE([werror],
  [AS_HELP_STRING([--enable-werror],
    [treat baseline warnings as errors, as the strict CI builds do])],
  [], [enable_werror=no])
case $enable_warning_tier in
  baseline | migration | analysis) ;;
  *) AC_MSG_ERROR([unknown warning tier: $enable_warning_tier]) ;;
esac
AS_IF([test "x$enable_werror" != xno && test "x$enable_warning_tier" != xbaseline],
  [AC_MSG_ERROR([--enable-werror applies to the baseline tier only; the $enable_warning_tier tier is tracked by its warning budget])])

dnl Production build profile.  scripts/deployment/production_profile.sh is the
dnl single source of truth shared with CMake: it feature-detects the
dnl optimization and hardening flags and reports anything the toolchain
dnl rejects.  LTO and PGO are explicit, measured profiles and never default.
AC_ARG_ENABLE([production],
  [AS_HELP_STRING([--enable-production],
    [build the optimized and hardened production profile (-O2 -g plus feature-detected hardening)])],
  [], [enable_production=no])
AC_ARG_ENABLE([lto],
  [AS_HELP_STRING([--enable-lto],
    [add link-time optimization to the build (explicit, measured profile)])],
  [], [enable_lto=no])
AC_ARG_WITH([pgo-generate],
  [AS_HELP_STRING([--with-pgo-generate=DIR],
    [instrument the build to write profile data into DIR])],
  [], [with_pgo_generate=no])
AC_ARG_WITH([pgo-use],
  [AS_HELP_STRING([--with-pgo-use=PATH],
    [optimize using previously collected profile data at PATH])],
  [], [with_pgo_use=no])

PRODUCTION_CFLAGS=""
PRODUCTION_LDFLAGS=""
production_profile_args="--warnings $enable_warning_tier"
AS_IF([test "x$enable_production" != xno],
  [production_profile_args="$production_profile_args --production"])
AS_IF([test "x$enable_lto" != xno],
  [production_profile_args="$production_profile_args --lto"])
AS_IF([test "x$with_pgo_generate" != xno],
  [production_profile_args="$production_profile_args --pgo-generate $with_pgo_generate"])
AS_IF([test "x$with_pgo_use" != xno],
  [production_profile_args="$production_profile_args --pgo-use $with_pgo_use"])

AC_MSG_CHECKING([the build profile for $CC])
production_profile_output=`$SHELL "$srcdir/scripts/deployment/production_profile.sh" \
  --cc "$CC" --cflags "$CSTD_CFLAGS" $production_profile_args 2>conftest.profile.err`
production_profile_status=$?
if test $production_profile_status -ne 0; then
  AC_MSG_RESULT([failed])
  cat conftest.profile.err >&AS_MESSAGE_LOG_FD
  production_profile_error=`cat conftest.profile.err`
  rm -f conftest.profile.err
  AC_MSG_ERROR([$production_profile_error])
fi
rm -f conftest.profile.err
PRODUCTION_CFLAGS=`printf '%s\n' "$production_profile_output" | sed -n 's/^PRODUCTION_CFLAGS=//p'`
PRODUCTION_LDFLAGS=`printf '%s\n' "$production_profile_output" | sed -n 's/^PRODUCTION_LDFLAGS=//p'`
production_supported=`printf '%s\n' "$production_profile_output" | sed -n 's/^PRODUCTION_SUPPORTED=//p'`
production_unsupported=`printf '%s\n' "$production_profile_output" | sed -n 's/^PRODUCTION_UNSUPPORTED=//p'`
WARNING_CFLAGS=`printf '%s\n' "$production_profile_output" | sed -n 's/^WARNING_CFLAGS=//p'`
warning_unsupported=`printf '%s\n' "$production_profile_output" | sed -n 's/^WARNING_UNSUPPORTED=//p'`
AC_MSG_RESULT([${PRODUCTION_CFLAGS:-default} $PRODUCTION_LDFLAGS])
AS_IF([test "x$enable_production" != xno],
  [AC_MSG_NOTICE([production hardening enabled: ${production_supported:-none}])])
for production_feature in $production_unsupported; do
  AC_MSG_WARN([hardening feature not supported by $CC and left out: $production_feature])
done
MYFLAGS="$WARNING_CFLAGS"
AS_IF([test "x$enable_werror" != xno], [MYFLAGS="$MYFLAGS -Werror"])
AC_MSG_NOTICE([warning tier $enable_warning_tier for $CC: $MYFLAGS])
for warning_flag in $warning_unsupported; do
  AC_MSG_WARN([warning flag not supported by $CC and left out: $warning_flag])
done
dnl Automake compiles with $(AM_CFLAGS) $(CFLAGS), so anything carried in
dnl AM_CFLAGS loses to a user CFLAGS such as -D_FORTIFY_SOURCE=2.  Append the
dnl profile to CFLAGS/LDFLAGS themselves, after the user's values, so the
dnl repository policy is the last word exactly as it is under CMake.
luminari_profile_cflags=$PRODUCTION_CFLAGS
LDFLAGS="$LDFLAGS $PRODUCTION_LDFLAGS"

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

dnl Check for json-c library for terrain bridge API
AC_CHECK_LIB(json-c, json_object_new_object,
    [AC_DEFINE([HAVE_JSON_C], [1], [Define if json-c library is available])],
    [AC_MSG_WARN([json-c library not found - terrain bridge API will be disabled])])

dnl Checks for header files.
AC_CHECK_INCLUDES_DEFAULT
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h sys/fcntl.h errno.h net/errno.h string.h strings.h)
AC_CHECK_HEADERS(limits.h sys/time.h sys/select.h sys/types.h unistd.h)
AC_CHECK_HEADERS(memory.h crypt.h assert.h arpa/telnet.h arpa/inet.h)
AC_CHECK_HEADERS(sys/stat.h sys/socket.h sys/resource.h netinet/in.h netdb.h)
AC_CHECK_HEADERS(signal.h sys/uio.h mcheck.h)
luminari_saved_CPPFLAGS=$CPPFLAGS
CPPFLAGS="$CPPFLAGS $LIBEVENT_CFLAGS"
AC_CHECK_HEADERS(event2/event.h, [],
  [AC_MSG_ERROR([libevent headers are required; install the libevent development package])])
CPPFLAGS=$luminari_saved_CPPFLAGS

dnl AC_UNSAFE_CRYPT removed - was a custom macro

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_CHECK_TYPE(ssize_t, int)
AC_CHECK_HEADERS([time.h sys/time.h])

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
    [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[$headers]],[[struct in_addr tp = {0}; return tp.s_addr != 0;]])], [ac_cv_struct_in_addr=yes], [ac_cv_struct_in_addr=no])])

  if test $ac_cv_struct_in_addr = yes; then
    AC_DEFINE([HAVE_STRUCT_IN_ADDR], [1], [Define if struct in_addr is available])
  fi
fi


dnl Check for the 'typedef socklen_t' definition. Even uglier, yes.
if test $ac_cv_header_sys_socket_h = no; then
  ac_cv_socklen_t = no;
else
  AC_CACHE_CHECK([for typedef socklen_t], ac_cv_socklen_t,
    [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/socket.h>]],[[socklen_t sl = 0; return (int)sl;]])], [ac_cv_socklen_t=yes], [ac_cv_socklen_t=no])])
fi

if test $ac_cv_socklen_t = no; then
  AC_DEFINE([socklen_t], [int], [Define socklen_t as int if not available])
fi


dnl Checks for library functions.
AC_FUNC_VPRINTF
AC_CHECK_FUNCS(gettimeofday select snprintf strcasecmp strdup strerror stricmp strlcat strlcpy strncasecmp strnicmp strstr vsnprintf)

dnl Check for functions that parse IP addresses
ORIGLIBS=$LIBS
LIBS="$LIBS $NETLIB"
AC_CHECK_FUNCS(inet_addr inet_aton)
LIBS=$ORIGLIBS

dnl Check for prototypes - removed custom AC_CHECK_PROTO macros
dnl Modern compilers have standard prototypes for these functions

dnl Every probe is done: restore the caller's CFLAGS verbatim (including any
dnl -Werror) and append the profile so the repository policy is the last word.
CFLAGS="$luminari_user_cflags $luminari_profile_cflags"

AC_CONFIG_FILES([Makefile util/Makefile])
AC_OUTPUT
#
echo "Configuration completed.  To compile, type:  make all"
