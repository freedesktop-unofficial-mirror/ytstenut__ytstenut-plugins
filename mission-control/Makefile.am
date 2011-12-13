AM_CFLAGS = \
	-I$(top_srcdir) \
	$(MISSION_CONTROL_MODULE_CFLAGS) \
	$(TELEPATHY_GLIB_CFLAGS) \
	$(TELEPATHY_YTSTENUT_CFLAGS)

pluginsdir = $(MISSION_CONTROL_PLUGINS_DIR)
plugins_LTLIBRARIES = \
	mcp-account-manager-ytstenut.la

mcp_account_manager_ytstenut_la_SOURCES = \
	mission-control-plugin.c \
	mcp-account-manager-ytstenut.c mcp-account-manager-ytstenut.h

mcp_account_manager_ytstenut_la_LIBADD = \
	$(MISSION_CONTROL_MODULE_LIBS) \
	$(TELEPATHY_GLIB_LIBS) \
	$(TELEPATHY_YTSTENUT_LIBS)

mcp_account_manager_ytstenut_la_LDFLAGS = \
	-module \
	-avoid-version \
	-no-undefined \
	-export-symbols-regex 'mcp_plugin_ref_nth_object|mcd_plugin_init'

Android.mk: Makefile.am
	androgenizer -:PROJECT ytstenut_plugins \
	 -:SHARED mcp-account-manager-ytstenut \
	 -:TAGS eng debug \
	 -:REL_TOP $(top_srcdir) -:ABS_TOP $(abs_top_srcdir) \
	 -:SOURCES $(mcp_account_manager_ytstenut_la_SOURCES) \
	 -:CFLAGS $(DEFS) $(CFLAGS) $(DEFAULT_INCLUDES) $(INCLUDES) \
	  $(AM_CFLAGS) \
	 -:CPPFLAGS $(CPPFLAGS) $(AM_CPPFLAGS) \
	 -:LDFLAGS $(mcp_account_manager_ytstenut_la_LIBADD) \
	> $@
