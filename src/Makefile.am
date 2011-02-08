AM_CFLAGS = \
	$(ERROR_CFLAGS) \
	-DG_LOG_DOMAIN=\"ytstenut\" \
	$(SALUT_CFLAGS)

LDADD = \
	$(SALUT_LIBS)

plugin_LTLIBRARIES = ytstenut.la

AM_LDFLAGS = -module -avoid-version -shared

ytstenut_la_SOURCES = \
	plugin.c \
	plugin.h
