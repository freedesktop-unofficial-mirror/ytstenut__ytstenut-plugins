AM_CFLAGS = \
	-DG_LOG_DOMAIN=\"ytstenut\" \
	$(SALUT_CFLAGS)

plugindir = $(salutplugindir)

plugin_LTLIBRARIES = ytstenut-salut.la

AM_LDFLAGS = -module -avoid-version -shared

ytstenut_salut_la_LIBADD = \
	$(SALUT_LIBS)

ytstenut_salut_la_SOURCES = \
	ytstenut.c \
	ytstenut.h \
	caps-manager.c \
	caps-manager.h
