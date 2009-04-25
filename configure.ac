AC_INIT([librabbitmq],[0.0.1],[tonyg@rabbitmq.com])
AC_CONFIG_SRCDIR(librabbitmq/codegen.py)
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

sibling_codegen_dir=$(pwd)/../rabbitmq-codegen
AMQP_CODEGEN_DIR=$(test -d $sibling_codegen_dir && echo $sibling_codegen_dir || echo codegen)
AMQP_SPEC_JSON_PATH=$AMQP_CODEGEN_DIR/amqp-0.8.json

AC_SUBST(AMQP_CODEGEN_DIR)
AC_SUBST(AMQP_SPEC_JSON_PATH)

AC_OUTPUT(
Makefile
librabbitmq/Makefile
examples/Makefile
)
