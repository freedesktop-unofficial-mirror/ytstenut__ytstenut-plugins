/*
 * ytstenut.c - Source for YstPlugin
 *
 * Copyright (C) 2011 Intel Corp.
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

#include "caps-manager.h"
#include "status.h"
#include "channel-manager.h"

#ifdef SALUT
#include <salut/plugin.h>
#include <salut/protocol.h>
typedef SalutPlugin FooPlugin;
typedef SalutPluginConnection FooConnection;
typedef SalutSidecar FooSidecar;
#else
#include <gabble/plugin.h>
#include <gabble/plugin-connection.h>
typedef GabblePlugin FooPlugin;
typedef GabblePluginConnection FooConnection;
typedef GabbleSidecar FooSidecar;
#endif

#include <telepathy-ytstenut-glib/telepathy-ytstenut-glib.h>

#define DEBUG(msg, ...) \
  g_debug ("%s: " msg, G_STRFUNC, ##__VA_ARGS__)

static const gchar * const sidecar_interfaces[] = {
  TP_YTS_IFACE_STATUS,
  NULL
};

static void plugin_iface_init (gpointer g_iface, gpointer data);

G_DEFINE_TYPE_WITH_CODE (YtstPlugin, ytst_plugin, G_TYPE_OBJECT,
#ifdef SALUT
    G_IMPLEMENT_INTERFACE (SALUT_TYPE_PLUGIN, plugin_iface_init);
#else
    G_IMPLEMENT_INTERFACE (GABBLE_TYPE_PLUGIN, plugin_iface_init);
#endif
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

#ifdef SALUT
static void
ytstenut_plugin_initialize (SalutPlugin *plugin,
    TpBaseConnectionManager *connection_manager,
    SalutCreateProtocolImpl callback)
{
  TpBaseProtocol *protocol;

  DEBUG ("%p on connection manager %p", plugin, connection_manager);

  protocol = callback (G_TYPE_NONE,
      "_ytstenut._tcp", "local-ytstenut", "Ytstenut protocol", "im-ytstenut");
  tp_base_connection_manager_add_protocol (connection_manager, protocol);
}
#endif

static void
ytstenut_plugin_create_sidecar_async (
    FooPlugin *plugin,
    const gchar *sidecar_interface,
    FooConnection *connection,
    WockySession *session,
    GAsyncReadyCallback callback,
    gpointer user_data)
{
  GSimpleAsyncResult *result = g_simple_async_result_new (G_OBJECT (plugin),
      callback, user_data, ytstenut_plugin_create_sidecar_async);

  FooSidecar *sidecar = NULL;

  if (!tp_strdiff (sidecar_interface, TP_YTS_IFACE_STATUS))
    {
      sidecar = (FooSidecar *) ytst_status_new (session, connection);
      DEBUG ("created side car for: %s", TP_YTS_IFACE_STATUS);
    }
  else
    {
      g_simple_async_result_set_error (result, TP_ERRORS,
          TP_ERROR_NOT_IMPLEMENTED, "'%s' not implemented", sidecar_interface);
    }

  if (sidecar != NULL)
    g_simple_async_result_set_op_res_gpointer (result, sidecar, g_object_unref);

  g_simple_async_result_complete_in_idle (result);

  g_object_unref (result);
}

static FooSidecar *
ytstenut_plugin_create_sidecar_finish (
     FooPlugin *plugin,
     GAsyncResult *result,
     GError **error)
{
  FooSidecar *sidecar;

  if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result),
        error))
    return NULL;

  g_return_val_if_fail (g_simple_async_result_is_valid (result,
        G_OBJECT (plugin), ytstenut_plugin_create_sidecar_async), NULL);

  sidecar =
#ifdef GABBLE
    GABBLE_SIDECAR (g_simple_async_result_get_op_res_gpointer (
        G_SIMPLE_ASYNC_RESULT (result)));
#else
    SALUT_SIDECAR (g_simple_async_result_get_op_res_gpointer (
        G_SIMPLE_ASYNC_RESULT (result)));
#endif

  return g_object_ref (sidecar);
}

static GPtrArray *
ytstenut_plugin_create_channel_managers (
    FooPlugin *plugin,
    FooConnection *plugin_connection)
{
  GPtrArray *ret = g_ptr_array_sized_new (1);
  TpBaseConnection *connection = TP_BASE_CONNECTION (plugin_connection);

  DEBUG ("%p on connection %p", plugin, plugin_connection);

  g_ptr_array_add (ret, g_object_new (YTST_TYPE_CAPS_MANAGER, NULL));
  g_ptr_array_add (ret, ytst_channel_manager_new (connection));

  return ret;
}

static void
plugin_iface_init (gpointer g_iface,
    gpointer data G_GNUC_UNUSED)
{
#ifdef SALUT
  SalutPluginInterface *iface = g_iface;
#else
  GabblePluginInterface *iface = g_iface;
    #endif

#ifdef SALUT
  iface->api_version = SALUT_PLUGIN_CURRENT_VERSION;
  iface->initialize = ytstenut_plugin_initialize;
#endif

  iface->name = "Ytstenut plugin";
  iface->version = PACKAGE_VERSION;

  iface->sidecar_interfaces = sidecar_interfaces;
  iface->create_sidecar_async = ytstenut_plugin_create_sidecar_async;
  iface->create_sidecar_finish = ytstenut_plugin_create_sidecar_finish;
  iface->create_channel_managers = ytstenut_plugin_create_channel_managers;
}

FooPlugin *
#ifdef SALUT
salut_plugin_create (void)
#else
gabble_plugin_create (void)
#endif
{
  return g_object_new (YTST_TYPE_PLUGIN, NULL);
}
