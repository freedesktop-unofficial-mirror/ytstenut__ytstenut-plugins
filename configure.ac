AC_INIT(ytstenut-plugins, 0.1)

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AM_CONFIG_HEADER(config.h)
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_PROG_LIBTOOL

dnl TODO: Once telepathy-salut plugin support is merged up stream, this
dnl should probably become a hard dependency.

PKG_CHECK_MODULES(SALUT, telepathy-salut, HAVE_SALUT=yes, HAVE_SALUT=no)
AM_CONDITIONAL(WITH_SALUT, test "$HAVE_SALUT" = "yes")
if test "$HAVE_SALUT" = "yes"; then
	AC_SUBST(SALUT_LIBS)
	AC_SUBST(SALUT_CFLAGS)
	salutplugindir=`pkg-config --variable=plugindir telepathy-salut`
	AC_SUBST(salutplugindir)
else
	AC_MSG_WARN([Not building the salut module. No telepathy-salut.pc file.])
fi

ERROR_CFLAGS="-Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter -Wdeclaration-after-statement -Wshadow -Wmissing-prototypes -Wmissing-declarations"
AC_SUBST(ERROR_CFLAGS)

AC_OUTPUT([
	Makefile
	salut/Makefile
])
