/*
 * ytst-channel-manager.c - Source for YtstChannelManager
 * Copyright (C) 2005, 2011 Collabora Ltd.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "extensions/extensions.h"
#include "ytst-message-channel.h"
#include "ytst-channel-manager.h"
#include "salut-xmpp-connection-manager.h"

#include <gibber/gibber-xmpp-connection.h>
#include <gibber/gibber-namespaces.h>

#include <telepathy-glib/channel-manager.h>
#include <telepathy-glib/dbus.h>
#include <telepathy-glib/gtypes.h>
#include <telepathy-glib/interfaces.h>

#include <telepathy-ytstenut-glib/telepathy-ytstenut-glib.h>

#define DEBUG_FLAG DEBUG_IM
#include "debug.h"

static void ytst_channel_manager_iface_init (gpointer g_iface,
    gpointer iface_data);

G_DEFINE_TYPE_WITH_CODE (YtstChannelManager, ytst_channel_manager, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (TP_TYPE_CHANNEL_MANAGER,
      ytst_channel_manager_iface_init);
);

/* properties */
enum
{
  PROP_CONNECTION = 1,
  PROP_CONTACT_MANAGER,
  PROP_XMPP_CONNECTION_MANAGER,
  LAST_PROPERTY
};

/* private structure */
struct _YtstChannelManagerPrivate
{
  SalutContactManager *contact_manager;
  SalutConnection *connection;
  SalutXmppConnectionManager *xmpp_connection_manager;
  GHashTable *channels;
  gulong status_changed_id;
  gboolean dispose_has_run;
};

/* -----------------------------------------------------------------------------
 * INTERNAL
 */

static void
on_channel_closed (YtstMessageChannel *channel,
    gpointer user_data)
{
  YtstChannelManager *self = YTST_CHANNEL_MANAGER (user_data);
  YtstChannelManagerPrivate *priv = self->priv;
  const gchar *id;

  tp_channel_manager_emit_channel_closed_for_object (self,
    TP_EXPORTABLE_CHANNEL (channel));

  if (priv->channels != NULL)
    {
      id = ytst_message_channel_get_id (channel);
      DEBUG ("Removing channel with id %s", id);
      g_hash_table_remove (priv->channels, id);
    }
}

static void
manager_take_ownership_of_channel (YtstChannelManager *self,
    YtstMessageChannel *channel)
{
  YtstChannelManagerPrivate *priv = self->priv;
  const gchar *id;

  id = ytst_message_channel_get_id (channel);
  g_assert (id != NULL);

  /* Takes ownership of channel */
  g_assert (g_hash_table_lookup (priv->channels, id) == NULL);
  g_hash_table_insert (priv->channels, g_strdup (id), channel);

  g_signal_connect (channel, "closed", G_CALLBACK (on_channel_closed), self);
}

static gboolean
message_stanza_filter (SalutXmppConnectionManager *mgr,
    GibberXmppConnection *conn,
    WockyStanza *stanza,
    SalutContact *contact,
    gpointer user_data)
{
  YtstChannelManager *self = YTST_CHANNEL_MANAGER (user_data);
  YtstChannelManagerPrivate *priv = self->priv;
  gboolean result;
  gchar *id;

  if (!ytst_message_channel_is_ytstenut_request_with_id (stanza, &id))
    return FALSE;

  /* We are interested by this stanza only if we need to create a new ytstenut
   * channel to handle it */
  result = (g_hash_table_lookup (priv->channels, id) == NULL);
  g_free (id);
  return result;
}

static void
message_stanza_callback (SalutXmppConnectionManager *mgr,
    GibberXmppConnection *conn,
    WockyStanza *stanza,
    SalutContact *contact,
    gpointer user_data)
{
  YtstChannelManager *self = YTST_CHANNEL_MANAGER (user_data);
  YtstChannelManagerPrivate *priv = self->priv;
  TpBaseConnection *base_conn = TP_BASE_CONNECTION (priv->connection);
  TpHandleRepoIface *handle_repo = tp_base_connection_get_handles (base_conn,
       TP_HANDLE_TYPE_CONTACT);
  YtstMessageChannel *channel;
  TpHandle handle;

  handle = tp_handle_lookup (handle_repo, contact->name, NULL, NULL);
  g_assert (handle != 0);

  channel = ytst_message_channel_new (priv->connection, contact, stanza, handle,
      base_conn->self_handle, priv->xmpp_connection_manager, conn);
  manager_take_ownership_of_channel (self, channel);
  tp_channel_manager_emit_new_channel (self, TP_EXPORTABLE_CHANNEL (channel),
      NULL);
}

static void
manager_close_all (YtstChannelManager *self)
{
  YtstChannelManagerPrivate *priv = self->priv;

  if (priv->channels != NULL)
    {
      GHashTable *tmp = priv->channels;

      DEBUG ("closing channels");
      priv->channels = NULL;
      g_hash_table_destroy (tmp);
    }

  if (priv->status_changed_id != 0UL)
    {
      g_signal_handler_disconnect (priv->connection, priv->status_changed_id);
      priv->status_changed_id = 0UL;
    }
}

static void
on_connection_status_changed (SalutConnection *conn,
    guint status,
    guint reason,
    YtstChannelManager *self)
{
  if (status == TP_CONNECTION_STATUS_DISCONNECTED)
    manager_close_all (self);
}

/* -----------------------------------------------------------------------------
 * OBJECT
 */

static void
ytst_channel_manager_init (YtstChannelManager *self)
{
  YtstChannelManagerPrivate *priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      YTST_TYPE_CHANNEL_MANAGER, YtstChannelManagerPrivate);
  self->priv = priv;
}

static void
ytst_channel_manager_get_property (GObject *object,
    guint property_id,
    GValue *value,
    GParamSpec *pspec)
{
  YtstChannelManager *self = YTST_CHANNEL_MANAGER (object);
  YtstChannelManagerPrivate *priv = self->priv;

  switch (property_id)
    {
      case PROP_CONNECTION:
        g_value_set_object (value, priv->connection);
        break;
      case PROP_CONTACT_MANAGER:
        g_value_set_object (value, priv->contact_manager);
        break;
      case PROP_XMPP_CONNECTION_MANAGER:
        g_value_set_object (value, priv->xmpp_connection_manager);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
ytst_channel_manager_set_property (GObject *object,
    guint property_id,
    const GValue *value,
    GParamSpec *pspec)
{
  YtstChannelManager *self = YTST_CHANNEL_MANAGER (object);
  YtstChannelManagerPrivate *priv = self->priv;

  switch (property_id)
    {
      case PROP_CONNECTION:
        priv->connection = g_value_get_object (value);
        break;
      case PROP_CONTACT_MANAGER:
        priv->contact_manager = g_value_dup_object (value);
        break;
      case PROP_XMPP_CONNECTION_MANAGER:
        priv->xmpp_connection_manager = g_value_dup_object (value);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
ytst_channel_manager_constructed (GObject *object)
{
  YtstChannelManager *self = YTST_CHANNEL_MANAGER (object);
  YtstChannelManagerPrivate *priv = self->priv;

  priv->channels = g_hash_table_new_full (g_str_hash, g_str_equal,
      g_free, g_object_unref);

  salut_xmpp_connection_manager_add_stanza_filter (
      priv->xmpp_connection_manager, NULL,
      message_stanza_filter, message_stanza_callback, self);

  priv->status_changed_id = g_signal_connect (priv->connection,
      "status-changed", (GCallback) on_connection_status_changed, self);

  if (G_OBJECT_CLASS (ytst_channel_manager_parent_class)->constructed)
    G_OBJECT_CLASS (ytst_channel_manager_parent_class)->constructed (object);
}

static void
ytst_channel_manager_dispose (GObject *object)
{
  YtstChannelManager *self = YTST_CHANNEL_MANAGER (object);
  YtstChannelManagerPrivate *priv = self->priv;

  if (priv->dispose_has_run)
    return;

  priv->dispose_has_run = TRUE;

  salut_xmpp_connection_manager_remove_stanza_filter (
      priv->xmpp_connection_manager, NULL,
      message_stanza_filter, message_stanza_callback, self);

  if (priv->contact_manager != NULL)
    {
      g_object_unref (priv->contact_manager);
      priv->contact_manager = NULL;
    }

  if (priv->xmpp_connection_manager != NULL)
    {
      g_object_unref (priv->xmpp_connection_manager);
      priv->xmpp_connection_manager = NULL;
    }

  manager_close_all (self);

  if (G_OBJECT_CLASS (ytst_channel_manager_parent_class)->dispose)
    G_OBJECT_CLASS (ytst_channel_manager_parent_class)->dispose (object);
}

static void
ytst_channel_manager_class_init (YtstChannelManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *param_spec;

  g_type_class_add_private (klass, sizeof (YtstChannelManagerPrivate));

  object_class->constructed = ytst_channel_manager_constructed;
  object_class->dispose = ytst_channel_manager_dispose;
  object_class->get_property = ytst_channel_manager_get_property;
  object_class->set_property = ytst_channel_manager_set_property;

  param_spec = g_param_spec_object (
      "connection",
      "SalutConnection object",
      "Salut connection object that owns this channel factory object.",
      SALUT_TYPE_CONNECTION,
      G_PARAM_CONSTRUCT_ONLY |
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_CONNECTION, param_spec);

  param_spec = g_param_spec_object (
      "contact-manager",
      "SalutContactManager object",
      "Salut Contact Manager associated with the Salut Connection of this "
      "manager",
      SALUT_TYPE_CONTACT_MANAGER,
      G_PARAM_CONSTRUCT_ONLY |
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_CONTACT_MANAGER,
      param_spec);

  param_spec = g_param_spec_object (
      "xmpp-connection-manager",
      "SalutXmppConnectionManager object",
      "Salut Xmpp Connection Manager associated with the Salut Connection "
      "of this manager",
      SALUT_TYPE_XMPP_CONNECTION_MANAGER,
      G_PARAM_CONSTRUCT_ONLY |
      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_XMPP_CONNECTION_MANAGER,
      param_spec);
}

typedef struct
{
  TpExportableChannelFunc func;
  gpointer data;
} foreach_closure;

static void
run_foreach_one (gpointer key,
    gpointer value,
    gpointer data)
{
  TpExportableChannel *chan = TP_EXPORTABLE_CHANNEL (value);
  foreach_closure *f = data;
  f->func (chan, f->data);
}

static void
ytst_channel_manager_foreach_channel (TpChannelManager *iface,
    TpExportableChannelFunc func,
    gpointer user_data)
{
  YtstChannelManager *self = YTST_CHANNEL_MANAGER (iface);
  YtstChannelManagerPrivate *priv = self->priv;
  foreach_closure f = { func, user_data };

  g_hash_table_foreach (priv->channels, run_foreach_one, &f);
}

static const gchar * const channel_fixed_properties[] = {
    TP_IFACE_CHANNEL ".ChannelType",
    TP_IFACE_CHANNEL ".TargetHandleType",
    TP_YTS_IFACE_CHANNEL ".RequestType",
    NULL
};


static const gchar * const channel_allowed_properties[] = {
    TP_IFACE_CHANNEL ".TargetHandle",
    TP_IFACE_CHANNEL ".TargetID",
    TP_YTS_IFACE_CHANNEL ".RequestBody",
    TP_YTS_IFACE_CHANNEL ".RequestAttributes",
    NULL
};

static void
ytst_channel_manager_type_foreach_channel_class (GType type,
    TpChannelManagerTypeChannelClassFunc func,
    gpointer user_data)
{
  GHashTable *table = g_hash_table_new_full (g_str_hash, g_str_equal,
      NULL, (GDestroyNotify) tp_g_value_slice_free);
  GValue *value;

  value = tp_g_value_slice_new (G_TYPE_STRING);
  g_value_set_static_string (value, TP_YTS_IFACE_CHANNEL);
  g_hash_table_insert (table, (gchar *) channel_fixed_properties[0],
      value);

  value = tp_g_value_slice_new (G_TYPE_UINT);
  g_value_set_uint (value, TP_HANDLE_TYPE_CONTACT);
  g_hash_table_insert (table, (gchar *) channel_fixed_properties[1],
      value);

  func (type, table, channel_allowed_properties, user_data);

  g_hash_table_destroy (table);
}

static gboolean
ytst_channel_manager_create_channel (TpChannelManager *manager,
    gpointer request_token,
    GHashTable *request_properties)
{
  YtstChannelManager *self = YTST_CHANNEL_MANAGER (manager);
  YtstChannelManagerPrivate *priv = self->priv;
  TpBaseConnection *base_conn = (TpBaseConnection *) priv->connection;
  TpHandleRepoIface *handle_repo = tp_base_connection_get_handles (
      base_conn, TP_HANDLE_TYPE_CONTACT);
  TpHandle handle;
  GError *error = NULL;
  const gchar *name;
  SalutContact *contact;
  WockyStanza *request;
  GSList *tokens = NULL;
  YtstMessageChannel *channel;

  if (tp_strdiff (tp_asv_get_string (request_properties,
          TP_IFACE_CHANNEL ".ChannelType"),
          TP_YTS_IFACE_CHANNEL))
    return FALSE;

  if (tp_asv_get_uint32 (request_properties,
        TP_IFACE_CHANNEL ".TargetHandleType", NULL) != TP_HANDLE_TYPE_CONTACT)
    return FALSE;

  handle = tp_asv_get_uint32 (request_properties,
      TP_IFACE_CHANNEL ".TargetHandle", NULL);

  if (!tp_handle_is_valid (handle_repo, handle, &error))
    goto error;

  /* Check if there are any other properties that we don't understand */
  if (tp_channel_manager_asv_has_unknown_properties (request_properties,
          channel_fixed_properties, channel_allowed_properties, &error))
      goto error;

  name = tp_handle_inspect (handle_repo, handle);
  DEBUG ("Requested channel for handle: %u (%s)", handle, name);

  contact = salut_contact_manager_get_contact (priv->contact_manager, handle);
  if (contact == NULL)
    {
      g_set_error (&error, TP_ERRORS, TP_ERROR_NOT_AVAILABLE,
          "%s is not online", name);
      goto error;
    }

  request = ytst_message_channel_build_request (request_properties,
      priv->connection->name, contact->name, &error);
  if (request == NULL)
    {
      g_object_unref (contact);
      goto error;
    }

  channel = ytst_message_channel_new (priv->connection, contact, request, handle,
      base_conn->self_handle, priv->xmpp_connection_manager, NULL);
  manager_take_ownership_of_channel (self, channel);

  if (request_token != NULL)
    tokens = g_slist_prepend (tokens, request_token);
  tp_channel_manager_emit_new_channel (self, TP_EXPORTABLE_CHANNEL (channel),
      tokens);

  return TRUE; /* We will handle this one */

error:
  g_return_val_if_fail (error, FALSE);
  tp_channel_manager_emit_request_failed (self, request_token,
      error->domain, error->code, error->message);
  g_error_free (error);
  return TRUE; /* We tried to handle */
}

static void
ytst_channel_manager_iface_init (gpointer g_iface,
    gpointer iface_data)
{
  TpChannelManagerIface *iface = g_iface;

  iface->foreach_channel = ytst_channel_manager_foreach_channel;
  iface->type_foreach_channel_class =
    ytst_channel_manager_type_foreach_channel_class;

  /*
   * Each channel only supports one request/reply, so we create
   * a channel and never reuse. Same implementation for all three.
   */
  iface->create_channel = ytst_channel_manager_create_channel;
  iface->request_channel = ytst_channel_manager_create_channel;
  iface->ensure_channel = ytst_channel_manager_create_channel;
}

/* public functions */
YtstChannelManager *
ytst_channel_manager_new (SalutConnection *connection,
    SalutContactManager *contact_manager,
    SalutXmppConnectionManager *xmpp_connection_manager)
{
  return g_object_new (YTST_TYPE_CHANNEL_MANAGER,
      "connection", connection,
      "contact-manager", contact_manager,
      "xmpp-connection-manager", xmpp_connection_manager,
      NULL);
}
