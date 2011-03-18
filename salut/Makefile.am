AM_CFLAGS = \
	-DG_LOG_DOMAIN=\"ytstenut\" \
	$(SALUT_CFLAGS)

LDADD = \
	$(SALUT_LIBS)

plugindir = $(salutplugindir)

plugin_LTLIBRARIES = ytstenut.la

AM_LDFLAGS = -module -avoid-version -shared

ytstenut_la_SOURCES = \
	ytstenut.c \
	ytstenut.h \
	caps-manager.c \
	caps-manager.h
