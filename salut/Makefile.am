AM_CFLAGS = \
	-DG_LOG_DOMAIN=\"ytstenut\" \
	-DSALUT \
	-I$(top_srcdir)/plugin-base \
	$(SALUT_CFLAGS) \
	$(TELEPATHY_YTSTENUT_CFLAGS)

plugindir = $(salutplugindir)

plugin_LTLIBRARIES = ytstenut-salut.la

AM_LDFLAGS = -module -avoid-version -shared

ytstenut_salut_la_LIBADD = \
	$(SALUT_LIBS) \
	$(TELEPATHY_YTSTENUT_LIBS)

ytstenut_salut_la_SOURCES = \
	ytstenut.c \
	ytstenut.h \
	$(top_srcdir)/plugin-base/caps-manager.c \
	$(top_srcdir)/plugin-base/caps-manager.h \
	status.c \
	status.h \
	message-channel.c \
	message-channel.h \
	channel-manager.c \
	channel-manager.h \
	$(top_srcdir)/plugin-base/utils.c \
	$(top_srcdir)/plugin-base/utils.h
