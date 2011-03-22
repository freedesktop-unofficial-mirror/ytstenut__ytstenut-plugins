AC_INIT(ytstenut-plugins, 0.1)

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AM_CONFIG_HEADER(config.h)
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_PROG_LIBTOOL

#GETTEXT_PACKAGE=AC_PACKAGE_NAME
#AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [The name of the gettext package.])
#AC_SUBST(GETTEXT_PACKAGE)
#AM_GLIB_GNU_GETTEXT

PKG_CHECK_MODULES(TELEPATHY_GLIB, telepathy-glib)
AC_SUBST(TELEPATHY_GLIB_CFLAGS)
AC_SUBST(TELEPATHY_GLIB_LIBS)

PKG_CHECK_MODULES(TELEPATHY_YTSTENUT, [telepathy-ytstenut-glib])
AC_SUBST(TELEPATHY_YTSTENUT_CFLAGS)
AC_SUBST(TELEPATHY_YTSTENUT_LIBS)

# ------------------------------------------------------------------------------
# MISSION CONTROL PLUGIN

PKG_CHECK_MODULES(MISSION_CONTROL_MODULE, mission-control-plugins >= 5.5.4)

AC_ARG_WITH([mission-control-plugins-dir],
	[AC_HELP_STRING([--with-mission-control-plugins-dir],
		[directory to install mission-control plugins])],
		[with_mc_plugins_dir=$withval], [with_mc_plugins_dir=""])

if test -z "$with_mc_plugins_dir"; then
	AC_MSG_CHECKING([Mission Control plugins dir])
	MISSION_CONTROL_PLUGINS_DIR=`pkg-config --variable=plugindir mission-control-plugins`
else
	MISSION_CONTROL_PLUGINS_DIR="$with_bisho_modules_dir"
fi

if test "x$MISSION_CONTROL_PLUGINS_DIR" = "x"; then
	AC_MSG_ERROR([could not determine plugindir])
else
	AC_MSG_RESULT([$MISSION_CONTROL_PLUGINS_DIR])
fi
AC_SUBST(MISSION_CONTROL_PLUGINS_DIR)


# ------------------------------------------------------------------------------
# SALUT PLUGIN

PKG_CHECK_MODULES(SALUT, telepathy-salut, HAVE_SALUT=yes, HAVE_SALUT=no)
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
	mission-control/Makefile
])
