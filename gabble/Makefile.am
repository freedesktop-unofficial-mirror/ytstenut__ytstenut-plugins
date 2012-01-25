AM_CFLAGS = \
	-DG_LOG_DOMAIN=\"ytstenut\" \
	-DGABBLE \
	-I$(top_srcdir)/plugin-base \
	$(GABBLE_CFLAGS) \
	$(TELEPATHY_YTSTENUT_CFLAGS)

plugindir = $(gabbleplugindir)

plugin_LTLIBRARIES = ytstenut-gabble.la

AM_LDFLAGS = -module -avoid-version -no-undefined -shared

ytstenut_gabble_la_LIBADD = \
	$(GABBLE_LIBS) \
	$(TELEPATHY_YTSTENUT_LIBS)

ytstenut_gabble_la_SOURCES = \
	$(top_srcdir)/plugin-base/ytstenut.c \
	$(top_srcdir)/plugin-base/ytstenut.h \
	$(top_srcdir)/plugin-base/caps-manager.c \
	$(top_srcdir)/plugin-base/caps-manager.h \
	status.c \
	status.h \
	message-channel.c \
	message-channel.h \
	$(top_srcdir)/plugin-base/channel-manager.c \
	$(top_srcdir)/plugin-base/channel-manager.h \
	$(top_srcdir)/plugin-base/utils.c \
	$(top_srcdir)/plugin-base/utils.h

