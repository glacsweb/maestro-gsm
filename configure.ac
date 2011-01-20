#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT
#AC_CONFIG_SRCDIR([src/nodenet.c])

AC_CANONICAL_TARGET([])

AM_INIT_AUTOMAKE(nodenet, 0.0.0)
AM_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC

AC_ARG_ENABLE(debug,
    [  --enable-debug          enable debug information [default=no]],
    [
        if test "$enableval" = "yes"; then
            CFLAGS="$CFLAGS -Wall -DDEBUG"
        else
            CFLAGS="$CFLAGS -Wall -DNDEBUG"
        fi
    ],[
        CFLAGS="$CFLAGS -Wall -DNDEBUG"
    ]
)

CPPFLAGS="$CPPFLAGS"
LIBS="$LIBS -lm"

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h stdint.h stdlib.h string.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Checks for library functions.
AC_TYPE_SIGNAL

AC_CHECK_LIB(util, openpty)

AC_CONFIG_FILES([
    Makefile
    src/Makefile
])
AC_PROG_RANLIB
AC_OUTPUT
