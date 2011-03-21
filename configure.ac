AC_INIT(ytstenut-plugins, 0.1)

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AM_CONFIG_HEADER(config.h)
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_PROG_LIBTOOL

# ------------------------------------------------------------------------------
# salut

PKG_CHECK_MODULES(SALUT, telepathy-salut)
AC_SUBST(SALUT_LIBS)
AC_SUBST(SALUT_CFLAGS)
salutplugindir=`pkg-config --variable=plugindir telepathy-salut`
AC_SUBST(salutplugindir)

# ------------------------------------------------------------------------------

AC_ARG_ENABLE(debug,
	AC_HELP_STRING([--enable-debug],
	[Compile binaries in debug mode]))

AC_MSG_CHECKING(for DEBUG mode)
if test "$enable_debug" = "yes"; then
	CFLAGS="$CFLAGS -g -O0 -Werror -DG_DISABLE_DEPRECATED"
	AC_DEFINE_UNQUOTED(_DEBUG, 1, [In debug mode])
	AC_MSG_RESULT(yes)
else
	AC_MSG_RESULT(no)
fi

CFLAGS="$CFLAGS -Wall -Wextra -Wno-missing-field-initializers \
	-Wno-unused-parameter -Wdeclaration-after-statement -Wshadow \
	-Wmissing-prototypes -Wmissing-declarations"

AC_SUBST(CFLAGS)

AC_OUTPUT([
	Makefile
	salut/Makefile
])
