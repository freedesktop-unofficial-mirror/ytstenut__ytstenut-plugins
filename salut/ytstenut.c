/*
 * ytstenut.c - Source for YstPlugin
 * Copyright (C) 2011 Collabora Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"

#include "ytstenut.h"

#include <salut/plugin.h>
#include <salut/protocol.h>

#include "caps-manager.h"

#define DEBUG(msg, ...) \
  g_debug ("%s: " msg, G_STRFUNC, ##__VA_ARGS__)

static void plugin_iface_init (gpointer g_iface, gpointer data);

G_DEFINE_TYPE_WITH_CODE (YtstPlugin, ytst_plugin, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (SALUT_TYPE_PLUGIN, plugin_iface_init);
    )

static void
ytst_plugin_init (YtstPlugin *object)
{
  DEBUG ("%p", object);
}

static void
ytst_plugin_class_init (YtstPluginClass *klass)
{
}

static void
initialize (SalutPlugin *plugin,
    TpBaseConnectionManager *connection_manager)
{
  TpBaseProtocol *protocol;

  DEBUG ("%p on connection manager %p", plugin, connection_manager);

  protocol = salut_protocol_new (G_TYPE_NONE,
      "_ytstenut._tcp", "local-ytstenut", "Ytstenut protocol", "im-ytstenut");
  tp_base_connection_manager_add_protocol (connection_manager, protocol);
}

static GPtrArray *
create_channel_managers (SalutPlugin *plugin,
    TpBaseConnection *connection)
{
  GPtrArray *ret = g_ptr_array_sized_new (1);

  DEBUG ("%p on connection %p", plugin, connection);

  g_ptr_array_add (ret, g_object_new (YTST_TYPE_CAPS_MANAGER, NULL));

  return ret;
}

static void
plugin_iface_init (gpointer g_iface,
    gpointer data G_GNUC_UNUSED)
{
  SalutPluginInterface *iface = g_iface;

  iface->api_version = SALUT_PLUGIN_CURRENT_VERSION;
  iface->name = "Ytstenut plugin";
  iface->version = PACKAGE_VERSION;

  iface->initialize = initialize;
  iface->create_channel_managers = create_channel_managers;
}

SalutPlugin *
salut_plugin_create (void)
{
  return g_object_new (YTST_TYPE_PLUGIN, NULL);
}
