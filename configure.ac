AC_INIT(ytstenut, 0.1)

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AM_CONFIG_HEADER(config.h)

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_PROG_LIBTOOL

PKG_CHECK_MODULES(SALUT, telepathy-salut)
AC_SUBST(SALUT_LIBS)
AC_SUBST(SALUT_CFLAGS)

plugindir=`pkg-config --variable=plugindir telepathy-salut`
AC_SUBST(plugindir)

ERROR_CFLAGS="-Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter -Wdeclaration-after-statement -Wshadow -Wmissing-prototypes -Wmissing-declarations"
AC_SUBST(ERROR_CFLAGS)

AC_OUTPUT([
	Makefile
        src/Makefile
])
