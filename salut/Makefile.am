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

copied_files = \
	ytstenut.c \
	caps-manager.c \
	channel-manager.c \
	utils.c

ytstenut_salut_la_SOURCES = \
	$(copied_files) \
	status.c \
	status.h \
	message-channel.c \
	message-channel.h

Android.mk: Makefile.am $(BUILT_SOURCES)
	for i in $(copied_files); do \
		cp $(top_srcdir)/plugin-base/$$i .; \
	done
	androgenizer -:PROJECT ytstenut-plugins \
	 -:SHARED ytstenut-salut -:TAGS eng debug \
	 -:REL_TOP $(top_srcdir) -:ABS_TOP $(abs_top_srcdir) \
	 -:SOURCES $(ytstenut_salut_la_SOURCES) \
	 -:CFLAGS $(DEFS) $(CFLAGS) $(DEFAULT_INCLUDES) $(INCLUDES) \
	  $(AM_CFLAGS) -I../../wocky \
	 -:CPPFLAGS $(CPPFLAGS) $(AM_CPPFLAGS) \
	 -:LDFLAGS $(AM_LDFLAGS) $(ytstenut_salut_la_LIBADD) $(SALUT_LIBS) -ltelepathy-salut \
	> $@
