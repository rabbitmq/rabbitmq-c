AC_INIT([librabbitmq],[0.0.1],[tonyg@rabbitmq.com])
AC_CONFIG_SRCDIR(librabbitmq/f.c)
AM_INIT_AUTOMAKE

AC_CONFIG_HEADER([config.h])

dnl Program checks
AC_PROG_CC

dnl Library checks
AM_PROG_LIBTOOL

dnl Header-file checks
AC_HEADER_STDC

dnl Only use -Wall if we have gcc
if test "x$GCC" = "xyes"; then
  if test -z "`echo "$CFLAGS" | grep "\-Wall" 2> /dev/null`" ; then
    CFLAGS="$CFLAGS -Wall"
  fi
fi

AC_OUTPUT(
Makefile
librabbitmq/Makefile
)
