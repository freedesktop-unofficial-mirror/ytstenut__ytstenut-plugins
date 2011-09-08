AC_INIT(ytstenut-plugins, 0.2.0)

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AM_CONFIG_HEADER(config.h)
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_PROG_LIBTOOL

GETTEXT_PACKAGE=AC_PACKAGE_NAME
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [The name of the gettext package.])
AC_SUBST(GETTEXT_PACKAGE)
AM_GLIB_GNU_GETTEXT

PKG_CHECK_MODULES(TELEPATHY_GLIB, telepathy-glib)
AC_SUBST(TELEPATHY_GLIB_CFLAGS)
AC_SUBST(TELEPATHY_GLIB_LIBS)

PKG_CHECK_MODULES(TELEPATHY_YTSTENUT, [telepathy-ytstenut-glib])
AC_SUBST(TELEPATHY_YTSTENUT_CFLAGS)
AC_SUBST(TELEPATHY_YTSTENUT_LIBS)

# ------------------------------------------------------------------------------
# MISSION CONTROL PLUGIN

PKG_CHECK_MODULES(MISSION_CONTROL_MODULE, mission-control-plugins >= 5.7.7+)

AC_ARG_WITH([mission-control-plugins-dir],
	[AC_HELP_STRING([--with-mission-control-plugins-dir],
		[directory to install mission-control plugins])],
		[with_mc_plugins_dir=$withval], [with_mc_plugins_dir=""])

if test -z "$with_mc_plugins_dir"; then
	AC_MSG_CHECKING([Mission Control plugins dir])
	MISSION_CONTROL_PLUGINS_DIR=`pkg-config --variable=plugindir mission-control-plugins`
else
	MISSION_CONTROL_PLUGINS_DIR="$with_mc_plugins_dir"
fi

if test "x$MISSION_CONTROL_PLUGINS_DIR" = "x"; then
	AC_MSG_ERROR([could not determine plugindir])
else
	AC_MSG_RESULT([$MISSION_CONTROL_PLUGINS_DIR])
fi
AC_SUBST(MISSION_CONTROL_PLUGINS_DIR)

AC_MSG_CHECKING([Mission Control executable])
MISSION_CONTROL_EXECUTABLE=`pkg-config --variable=missioncontrolpath mission-control-plugins`
if test "x$MISSION_CONTROL_EXECUTABLE" = "x"; then
	AC_MSG_ERROR([could not determine location of mission control executable])
else
	AC_MSG_RESULT([$MISSION_CONTROL_EXECUTABLE])
fi
AC_SUBST(MISSION_CONTROL_EXECUTABLE)

# ------------------------------------------------------------------------------
# SALUT PLUGIN

PKG_CHECK_MODULES(SALUT, telepathy-salut, HAVE_SALUT=yes, HAVE_SALUT=no)
AC_SUBST(SALUT_LIBS)
AC_SUBST(SALUT_CFLAGS)
salutplugindir=`pkg-config --variable=plugindir telepathy-salut`
AC_SUBST(salutplugindir)

AC_MSG_CHECKING([telepathy-salut executable])
SALUT_EXECUTABLE=`pkg-config --variable=salutpath telepathy-salut`
if test "$SALUT_EXECUTABLE" = ""; then
	AC_MSG_ERROR([could not determine location of telepathy-salut executable])
else
	AC_MSG_RESULT([$SALUT_EXECUTABLE])
fi
AC_SUBST(SALUT_EXECUTABLE)

AM_PATH_PYTHON([2.5])

# Check for a python >= 2.5 with twisted to run python tests
AC_MSG_CHECKING([for Python with Avahi, Twisted and XMPP protocol support])
if $PYTHON -c "import twisted.words.xish.domish, twisted.words.protocols.jabber, twisted.internet.reactor, avahi" >/dev/null 2>&1; then
    TEST_PYTHON="$PYTHON"
else
    TEST_PYTHON=false
fi
AC_MSG_RESULT([$TEST_PYTHON])
AC_SUBST(TEST_PYTHON)
AM_CONDITIONAL([WANT_TWISTED_TESTS], test false != "$TEST_PYTHON")

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

CFLAGS="$CFLAGS -Werror -Wall -Wextra -Wno-missing-field-initializers \
	-Wno-unused-parameter -Wdeclaration-after-statement -Wshadow \
	-Wmissing-prototypes -Wmissing-declarations"

AC_SUBST(CFLAGS)

AC_OUTPUT([
	Makefile
	mission-control/Makefile
	salut/Makefile
	tests/Makefile
	tests/twisted/Makefile
	tests/twisted/tools/Makefile
	tests/twisted/tools/exec-with-log.sh
	tests/twisted/tools/salut-exec-with-log.sh
	tests/twisted/tools/tmp-session-bus.conf
	tests/twisted/tools/org.freedesktop.Telepathy.MissionControl5.service
	tests/twisted/tools/org.freedesktop.Telepathy.Client.Logger.service
	tests/twisted/tools/org.freedesktop.Telepathy.ConnectionManager.salut.service
])
