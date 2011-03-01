AM_CFLAGS = \
	-I$(top_srcdir) \
	$(MISSION_CONTROL_MODULE_CFLAGS) \
	$(TELEPATHY_GLIB_CFLAGS)

pluginsdir = $(MISSION_CONTROL_PLUGINS_DIR)
plugins_LTLIBRARIES = \
	mcp-account-manager-ytstenut.la

mcp_account_manager_ytstenut_la_SOURCES = \
	mission-control-plugin.c \
	mcp-account-manager-ytstenut.c mcp-account-manager-ytstenut.h

mcp_account_manager_ytstenut_la_LIBADD = \
	$(MISSION_CONTROL_MODULE_LIBS) \
	$(TELEPATHY_GLIB_LIBS)

mcp_account_manager_ytstenut_la_LDFLAGS = \
	-module \
	-avoid-version \
	-no-undefined \
	-export-symbols-regex mcp_plugin_ref_nth_object
