AM_CFLAGS = \
	-DG_LOG_DOMAIN=\"ytstenut\" \
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
	caps-manager.c \
	caps-manager.h \
	status.c \
	status.h \
	message-channel.c \
	message-channel.h \
	channel-manager.c \
	channel-manager.h \
	utils.c \
	utils.h
