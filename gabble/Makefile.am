AM_CFLAGS = \
	-DG_LOG_DOMAIN=\"ytstenut\" \
	-DGABBLE \
	-I$(top_srcdir)/plugin-base \
	$(GABBLE_CFLAGS) \
	$(TELEPATHY_YTSTENUT_CFLAGS) \
	$(WOCKY_CFLAGS)

plugindir = $(gabbleplugindir)

plugin_LTLIBRARIES = ytstenut-gabble.la

AM_LDFLAGS = -module -avoid-version -no-undefined -shared

ytstenut_gabble_la_LIBADD = \
	$(TELEPATHY_YTSTENUT_LIBS) \
	$(WOCKY_LIBS)

copied_files = \
	ytstenut.c \
	caps-manager.c \
	channel-manager.c \
	utils.c

ytstenut_gabble_la_SOURCES = \
	$(copied_files) \
	status.c \
	message-channel.c

CLEANFILES = $(copied_files)

Android.mk: Makefile.am $(BUILT_SOURCES)
	for i in $(copied_files); do \
	  cp $(top_srcdir)/plugin-base/$$i .; \
	done
	androgenizer -:PROJECT ytstenut-plugins \
	 -:SHARED ytstenut-gabble -:TAGS eng debug \
	 -:REL_TOP $(top_srcdir) -:ABS_TOP $(abs_top_srcdir) \
	 -:SOURCES $(ytstenut_gabble_la_SOURCES) \
	 -:CFLAGS $(DEFS) $(CFLAGS) $(DEFAULT_INCLUDES) $(INCLUDES) \
	  $(AM_CFLAGS) \
	 -:CPPFLAGS $(CPPFLAGS) $(AM_CPPFLAGS) \
	 -:LDFLAGS $(AM_LDFLAGS) $(ytstenut_gabble_la_LIBADD) -ltelepathy-gabble \
	> $@
