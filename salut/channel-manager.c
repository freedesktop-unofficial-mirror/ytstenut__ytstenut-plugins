/*
 * channel-manager.c - Source for YtstChannelManager
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
#include "message-channel.h"
#include "channel-manager.h"
#include "util.h"

#include <telepathy-glib/channel-manager.h>
#include <telepathy-glib/dbus.h>
#include <telepathy-glib/gtypes.h>
#include <telepathy-glib/interfaces.h>
#include <telepathy-glib/util.h>

#include <salut/caps-channel-manager.h>

#include <telepathy-ytstenut-glib/telepathy-ytstenut-glib.h>

#define DEBUG(msg, ...) \
  g_debug ("%s: " msg, G_STRFUNC, ##__VA_ARGS__)

static void ytst_channel_manager_iface_init (gpointer g_iface,
    gpointer iface_data);
static void ytst_caps_channel_manager_iface_init (gpointer g_iface,
    gpointer iface_data);

G_DEFINE_TYPE_WITH_CODE (YtstChannelManager, ytst_channel_manager, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (TP_TYPE_CHANNEL_MANAGER,
        ytst_channel_manager_iface_init);
    G_IMPLEMENT_INTERFACE (GABBLE_TYPE_CAPS_CHANNEL_MANAGER,
        ytst_caps_channel_manager_iface_init);
);

/* properties */
enum
{
  PROP_CONNECTION = 1,
  LAST_PROPERTY
};

/* private structure */
struct _YtstChannelManagerPrivate
{
  SalutConnection *connection;
  GQueue *channels;
  gulong status_changed_id;
  guint message_handler_id;
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

  tp_channel_manager_emit_channel_closed_for_object (self,
    TP_EXPORTABLE_CHANNEL (channel));

  if (priv->channels != NULL)
    {
      DEBUG ("Removing channel %p", channel);
      g_queue_remove (priv->channels, channel);
    }
}

static void
manager_take_ownership_of_channel (YtstChannelManager *self,
    YtstMessageChannel *channel)
{
  YtstChannelManagerPrivate *priv = self->priv;

  /* Takes ownership of channel */
  g_assert (g_queue_index (priv->channels, channel) == -1);
  g_queue_push_tail (priv->channels, channel);

  g_signal_connect (channel, "closed", G_CALLBACK (on_channel_closed), self);
}

static gboolean
message_stanza_callback (WockyPorter *porter,
    WockyStanza *stanza,
    gpointer user_data)
{
  YtstChannelManager *self = YTST_CHANNEL_MANAGER (user_data);
  YtstChannelManagerPrivate *priv = self->priv;

  WockyNode *top;
  WockyStanzaSubType sub_type = WOCKY_STANZA_SUB_TYPE_NONE;

  TpBaseConnection *base_conn = TP_BASE_CONNECTION (priv->connection);
  TpHandleRepoIface *handle_repo = tp_base_connection_get_handles (base_conn,
       TP_HANDLE_TYPE_CONTACT);
  YtstMessageChannel *channel;
  TpHandle handle;
  WockyLLContact *contact = WOCKY_LL_CONTACT (
      wocky_stanza_get_from_contact (stanza));

  /* needs to be type get or set */
  wocky_stanza_get_type_info (stanza, NULL, &sub_type);
  if (sub_type != WOCKY_STANZA_SUB_TYPE_GET
      && sub_type != WOCKY_STANZA_SUB_TYPE_SET)
    return FALSE;

  /* we must have an ID */
  top = wocky_stanza_get_top_node (stanza);
  if (wocky_node_get_attribute (top, "id") == NULL)
    return FALSE;

  handle = tp_handle_lookup (handle_repo,
      wocky_ll_contact_get_jid (contact), NULL, NULL);
  g_assert (handle != 0);

  channel = ytst_message_channel_new (priv->connection, contact, stanza, handle,
      base_conn->self_handle);
  manager_take_ownership_of_channel (self, channel);
  tp_channel_manager_emit_new_channel (self, TP_EXPORTABLE_CHANNEL (channel),
      NULL);

  return TRUE;
}

static void
manager_close_all (YtstChannelManager *self)
{
  YtstChannelManagerPrivate *priv = self->priv;

  if (priv->channels != NULL)
    {
      DEBUG ("closing channels");
      g_queue_foreach (priv->channels, (GFunc) g_object_unref, NULL);
      g_queue_free (priv->channels);
      priv->channels = NULL;
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
  WockySession *session;

  priv->channels = g_queue_new ();

  session = salut_connection_get_session (priv->connection);

  priv->message_handler_id = wocky_porter_register_handler_from_anyone (
      wocky_session_get_porter (session),
      WOCKY_STANZA_TYPE_IQ, WOCKY_STANZA_SUB_TYPE_NONE,
      WOCKY_PORTER_HANDLER_PRIORITY_NORMAL,
      message_stanza_callback, self,
      '(', "message",
        ':', YTST_MESSAGE_NS,
      ')', NULL);

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
  WockySession *session;

  if (priv->dispose_has_run)
    return;

  priv->dispose_has_run = TRUE;

  session = salut_connection_get_session (priv->connection);

  wocky_porter_unregister_handler (
      wocky_session_get_porter (session),
      priv->message_handler_id);
  priv->message_handler_id = 0;

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
}

typedef struct
{
  TpExportableChannelFunc func;
  gpointer data;
} foreach_closure;

static void
run_foreach_one (gpointer value,
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

  g_queue_foreach (priv->channels, run_foreach_one, &f);
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
  WockySession *session;
  WockyContactFactory *factory;
  WockyLLContact *contact;
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

  session = salut_connection_get_session (priv->connection);
  factory = wocky_session_get_contact_factory (session);
  contact = wocky_contact_factory_lookup_ll_contact (factory, name);
  if (contact == NULL)
    {
      g_set_error (&error, TP_ERRORS, TP_ERROR_NOT_AVAILABLE,
          "%s is not online", name);
      goto error;
    }

  request = ytst_message_channel_build_request (request_properties,
      salut_connection_get_name (priv->connection), contact, &error);
  if (request == NULL)
    goto error;

  channel = ytst_message_channel_new (priv->connection, contact, request, handle,
      base_conn->self_handle);
  manager_take_ownership_of_channel (self, channel);

  g_object_unref (request);

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

static void
ytst_caps_channel_manager_iface_init (gpointer g_iface,
    gpointer iface_data)
{
  GabbleCapsChannelManagerIface *iface = g_iface;

  /* we don't need any of these */
  iface->reset_caps = NULL;
  iface->get_contact_caps = NULL;
  iface->represent_client = NULL;
}

/* public functions */
YtstChannelManager *
ytst_channel_manager_new (SalutConnection *connection)
{
  return g_object_new (YTST_TYPE_CHANNEL_MANAGER,
      "connection", connection,
      NULL);
}
