/*
 * plugin.c -- The salut plugin
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

#include "ytstenut-plugin.h"

#include <salut/plugin.h>
#include <salut/protocol.h>

#define DEBUG(msg, ...) \
  g_debug ("%s: " msg, G_STRFUNC, ##__VA_ARGS__)

static void plugin_iface_init (gpointer g_iface, gpointer data);

G_DEFINE_TYPE_WITH_CODE (YtstenutPlugin, ytstenut_plugin, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (SALUT_TYPE_PLUGIN, plugin_iface_init);
    )

static void
ytstenut_plugin_init (YtstenutPlugin *object)
{
  DEBUG ("%p", object);
}

static void
ytstenut_plugin_class_init (YtstenutPluginClass *klass)
{
}

static void
initialize (SalutPlugin *plugin,
    TpBaseConnectionManager *connection_manager)
{
  TpBaseProtocol *protocol;

  DEBUG ("%p on connection manager %p", plugin, connection_manager);

  protocol = salut_protocol_new (G_TYPE_NONE,
      "_ytstenut._tcp", "ytstenut", "Ytstenut protocol", "im-ytstenut");
  tp_base_connection_manager_add_protocol (connection_manager, protocol);
}

static GPtrArray *
create_channel_managers (SalutPlugin *plugin,
    TpBaseConnection *connection)
{
  DEBUG ("%p on connection %p", plugin, connection);

  return NULL;
}

static void
plugin_iface_init (
    gpointer g_iface,
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
  return g_object_new (YTSTENUT_TYPE_PLUGIN, NULL);
}
