AM_CFLAGS = \
	-DG_LOG_DOMAIN=\"ytstenut\" \
	-DSALUT \
	-I$(top_srcdir)/plugin-base \
	$(SALUT_CFLAGS) \
	$(TELEPATHY_YTSTENUT_CFLAGS) \
	$(WOCKY_CFLAGS)

plugindir = $(salutplugindir)

plugin_LTLIBRARIES = ytstenut-salut.la

AM_LDFLAGS = -module -avoid-version -shared

ytstenut_salut_la_LIBADD = \
	$(TELEPATHY_YTSTENUT_LIBS) \
	$(WOCKY_LIBS)

ytstenut_salut_la_SOURCES = \
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

Android.mk: Makefile.am $(BUILT_SOURCES)
	androgenizer -:PROJECT ytstenut-plugins \
	 -:SHARED ytstenut-salut -:TAGS eng debug \
	 -:REL_TOP $(top_srcdir) -:ABS_TOP $(abs_top_srcdir) \
	 -:SOURCES $(ytstenut_salut_la_SOURCES) \
	 -:CFLAGS $(DEFS) $(CFLAGS) $(DEFAULT_INCLUDES) $(INCLUDES) \
	  $(AM_CFLAGS) -I../../wocky \
	 -:CPPFLAGS $(CPPFLAGS) $(AM_CPPFLAGS) -DSALUT \
	 -:LDFLAGS $(AM_LDFLAGS) $(ytstenut_salut_la_LIBADD) -ltelepathy-salut \
	> $@
