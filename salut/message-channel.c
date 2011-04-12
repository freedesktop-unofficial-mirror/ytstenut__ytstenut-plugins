/*
 * ytst-message-channel.c - Source for YtstMessageChannel
 * Copyright (C) 2005-2008, 2010, 2011 Collabora Ltd.
 *   @author: Sjoerd Simons <sjoerd@luon.net>
 *   @author: Stef Walter <stefw@collabora.co.uk>
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

#include "ytst-message-channel.h"

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include <dbus/dbus-glib.h>
#include <telepathy-glib/channel.h>
#include <telepathy-glib/channel-iface.h>
#include <telepathy-glib/dbus.h>
#include <telepathy-glib/exportable-channel.h>
#include <telepathy-glib/gtypes.h>
#include <telepathy-glib/interfaces.h>
#include <telepathy-glib/svc-generic.h>
#include <telepathy-glib/svc-channel.h>

#include <telepathy-ytstenut-glib/telepathy-ytstenut-glib.h>

#include <gibber/gibber-namespaces.h>
#include <gibber/gibber-xmpp-connection.h>

#include <wocky/wocky-namespaces.h>
#include <wocky/wocky-utils.h>
#include <wocky/wocky-xmpp-reader.h>
#include <wocky/wocky-xmpp-writer.h>
#include <wocky/wocky-xmpp-error-enumtypes.h>

#define DEBUG_FLAG DEBUG_IM
#include "debug.h"
#include "salut-connection.h"
#include "salut-contact.h"
#include "salut-xmpp-connection-manager.h"
#include "salut-signals-marshal.h"
#include "salut-util.h"

#include "ytst-util.h"

#define EL_YTSTENUT_MESSAGE "message"

static void channel_ytstenut_iface_init (gpointer g_iface,
    gpointer iface_data);

G_DEFINE_TYPE_WITH_CODE (YtstMessageChannel, ytst_message_channel,
    TP_TYPE_BASE_CHANNEL,
    G_IMPLEMENT_INTERFACE (TP_TYPE_YTS_SVC_CHANNEL,
      channel_ytstenut_iface_init);
);

static const gchar *ytst_message_channel_interfaces[] = {
    NULL
};

/* properties */
enum
{
  PROP_CONTACT,
  PROP_XMPP_CONNECTION,
  PROP_XMPP_CONNECTION_MANAGER,
  PROP_TARGET_SERVICE,
  PROP_INITIATOR_SERVICE,
  PROP_REQUEST,
  PROP_REQUEST_TYPE,
  PROP_REQUEST_ATTRIBUTES,
  PROP_REQUEST_BODY,
  LAST_PROPERTY
};

/* private structure */
struct _YtstMessageChannelPrivate
{
  gboolean dispose_has_run;
  SalutContact *contact;
  GibberXmppConnection *xmpp_connection;
  SalutXmppConnectionManager *xmpp_connection_manager;

  GCancellable *cancellable;
  DBusGMethodInvocation *requesting;
  gboolean requested;
  gboolean replied;

  WockyStanza *request;
};

/* -----------------------------------------------------------------------------
 * INTERNAL
 */

static guint32
channel_get_message_type (WockyStanza *message)
{
  WockyStanzaSubType sub_type;

  wocky_stanza_get_type_info (message, NULL, &sub_type);
  switch (sub_type)
    {
      case WOCKY_STANZA_SUB_TYPE_GET:
        return TP_YTS_REQUEST_TYPE_GET;
      case WOCKY_STANZA_SUB_TYPE_SET:
        return TP_YTS_REQUEST_TYPE_SET;
      case WOCKY_STANZA_SUB_TYPE_RESULT:
        return TP_YTS_REPLY_TYPE_RESULT;
      case WOCKY_STANZA_SUB_TYPE_ERROR:
        return TP_YTS_REPLY_TYPE_ERROR;
      default:
        return 0;
    }
}

static gchar *
channel_get_message_body (WockyStanza *message)
{
  WockyXmppWriter *writer;
  WockyNode *top, *body;
  WockyNodeTree *tree;
  const guint8 *output;
  gsize length;
  gchar *result;

  top = wocky_stanza_get_top_node (message);
  body = wocky_node_get_first_child (top);

  writer = wocky_xmpp_writer_new_no_stream ();
  tree = wocky_node_tree_new_from_node (body);
  wocky_xmpp_writer_write_node_tree (writer, tree, &output, &length);
  result = g_strndup ((const gchar*) output, length);
  g_object_unref (writer);
  g_object_unref (tree);

  return result;
}

static GHashTable *
channel_new_message_attributes (void)
{
  return g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
}

static gboolean
attribute_to_hashtable (const gchar *key,
    const gchar *value,
    const gchar *pref,
    const gchar *ns,
    gpointer user_data)
{
  /* We only expose non namespace attributes in these properties */
  if (ns == NULL)
    g_hash_table_insert (user_data, g_strdup (key), g_strdup (value));
  return TRUE;
}

static GHashTable *
channel_get_message_attributes (WockyStanza *message)
{
  WockyNode *top, *body;
  GHashTable *attributes;

  top = wocky_stanza_get_top_node (message);
  body = wocky_node_get_first_child (top);

  attributes = channel_new_message_attributes ();
  wocky_node_each_attribute (body, attribute_to_hashtable, attributes);
  return attributes;
}

static const gchar *
channel_get_message_attribute (WockyStanza *message, const gchar *key)
{
  WockyNode *top, *body;

  top = wocky_stanza_get_top_node (message);
  body = wocky_node_get_first_child (top);

  return wocky_node_get_attribute (body, key);
}

static gboolean
channel_message_stanza_filter (SalutXmppConnectionManager *mgr,
    GibberXmppConnection *conn,
    WockyStanza *stanza,
    SalutContact *contact,
    gpointer user_data)
{
  YtstMessageChannel *self = YTST_MESSAGE_CHANNEL (user_data);
  YtstMessageChannelPrivate *priv = self->priv;
  gboolean result;
  gchar *id;

  /* We can't have received a reply yet */
  if (priv->replied)
    return FALSE;

  /* The channel has to match */
  if (priv->contact != contact)
    return FALSE;

  /* It has to be a ytstenut message */
  if (!ytst_message_channel_is_ytstenut_request_with_id (stanza, &id))
    return FALSE;

  /* And the id has to match */
  result = !wocky_strdiff (id, ytst_message_channel_get_id (self));

  g_free (id);
  return result;
}

static void
channel_message_stanza_callback (SalutXmppConnectionManager *mgr,
    GibberXmppConnection *conn,
    WockyStanza *stanza,
    SalutContact *contact,
    gpointer user_data)
{
  YtstMessageChannel *self = YTST_MESSAGE_CHANNEL (user_data);
  YtstMessageChannelPrivate *priv = self->priv;
  WockyXmppErrorType error_type;
  GError *core_error = NULL;
  WockyNode *specialized_node = NULL;
  GHashTable *attributes;
  gchar *body;

  g_assert (!priv->replied);
  priv->replied = TRUE;

  if (wocky_stanza_extract_errors (stanza, &error_type, &core_error,
      NULL, &specialized_node))
    {
      g_assert (core_error);
      tp_yts_svc_channel_emit_failed (self,
          ytst_message_error_type_from_wocky (error_type),
          wocky_enum_to_nick (WOCKY_TYPE_XMPP_ERROR_TYPE, core_error->code),
          specialized_node ? specialized_node->name : "",
          core_error->message ? core_error->message : "");
      g_clear_error (&core_error);
    }
  else
    {
      attributes = channel_get_message_attributes (stanza);
      body = channel_get_message_body (stanza);
      tp_yts_svc_channel_emit_replied (self, attributes, body);
      g_hash_table_destroy (attributes);
      g_free (body);
    }
}

static void
channel_connection_disconnected (YtstMessageChannel *self)
{
  YtstMessageChannelPrivate *priv = self->priv;

  if (priv->xmpp_connection != NULL)
    {
      DEBUG ("connection closed. Remove filters");
      salut_xmpp_connection_manager_remove_stanza_filter (
          priv->xmpp_connection_manager, priv->xmpp_connection,
          channel_message_stanza_filter, channel_message_stanza_callback, self);
      g_object_unref (priv->xmpp_connection);
      priv->xmpp_connection = NULL;
    }

  tp_base_channel_close (TP_BASE_CHANNEL (self));
}


static void
set_attributes_on_body (WockyNodeTree *body,
    GHashTable *attributes)
{
  GHashTableIter iter;
  WockyNode *node;
  const gchar *name, *value;

  node = wocky_node_tree_get_top_node (body);
  g_hash_table_iter_init (&iter, attributes);
  while (g_hash_table_iter_next (&iter, (gpointer *) &name,
      (gpointer *) &value))
    wocky_node_set_attribute (node, name, value);
}

static WockyNodeTree *
parse_message_body (const gchar *body,
    GError **error)
{
  WockyXmppReader *reader;
  WockyNodeTree *tree;
  WockyNode *node;
  GError *err = NULL;

  if (body == NULL || *body == '\0')
    body = "<ytstenut:message xmlns:ytstenut=\"" YTST_MESSAGE_NS "\" />";

  reader = wocky_xmpp_reader_new_no_stream ();
  wocky_xmpp_reader_push (reader, (guint8 *) body, strlen (body));
  tree = WOCKY_NODE_TREE (wocky_xmpp_reader_pop_stanza (reader));
  g_object_unref (reader);

  if (tree == NULL)
    {
      err = wocky_xmpp_reader_get_error (reader);
      g_set_error (error, TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
          "Invalid XML%s%s",
          err && err->message ? ": " : ".",
          err && err->message ? err->message : "");
      g_clear_error (&err);
      return NULL;
    }

  /* Make sure it smells right */
  node = wocky_node_tree_get_top_node (tree);
  if (!wocky_node_has_ns (node, YTST_MESSAGE_NS))
    {
      g_set_error_literal (error, TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
          "Must be a of the ytstenut namespace");
      node = NULL;
    }
  if (wocky_strdiff (node->name, EL_YTSTENUT_MESSAGE))
    {
      g_set_error_literal (error, TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
          "Must be a <ytstenut:message> element");
      node = NULL;
    }

  if (node == NULL)
    {
      g_object_unref (tree);
      tree = NULL;
    }

  return tree;
}

static void
on_xmpp_connection_manager_connection_closed (SalutXmppConnectionManager *mgr,
    GibberXmppConnection *conn,
    SalutContact *contact,
    gpointer user_data)
{
  YtstMessageChannel *self = YTST_MESSAGE_CHANNEL (user_data);
  YtstMessageChannelPrivate *priv = self->priv;

  if (conn != NULL && priv->xmpp_connection == conn)
    channel_connection_disconnected (self);
}

/* -----------------------------------------------------------------------------
 * OBJECT
 */

static void
ytst_message_channel_close (TpBaseChannel *chan)
{
  YtstMessageChannel *self = YTST_MESSAGE_CHANNEL (chan);
  YtstMessageChannelPrivate *priv = self->priv;

  if (!g_cancellable_is_cancelled (priv->cancellable))
    g_cancellable_cancel (priv->cancellable);

  tp_base_channel_destroyed (chan);
}

static void
ytst_message_channel_init (YtstMessageChannel *self)
{
  YtstMessageChannelPrivate *priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      YTST_TYPE_MESSAGE_CHANNEL, YtstMessageChannelPrivate);
  self->priv = priv;
  priv->cancellable = g_cancellable_new ();
}

static void
ytst_message_channel_get_property (GObject *object,
    guint property_id,
    GValue *value,
    GParamSpec *pspec)
{
  YtstMessageChannel *self = YTST_MESSAGE_CHANNEL (object);
  YtstMessageChannelPrivate *priv = self->priv;

  switch (property_id)
    {
      case PROP_CONTACT:
        g_value_set_object (value, priv->contact);
        break;
      case PROP_XMPP_CONNECTION:
        g_value_set_object (value, priv->xmpp_connection);
        break;
      case PROP_XMPP_CONNECTION_MANAGER:
        g_value_set_object (value, priv->xmpp_connection_manager);
        break;
      case PROP_TARGET_SERVICE:
        g_value_set_string (value, channel_get_message_attribute (priv->request,
            "to-service"));
        break;
      case PROP_INITIATOR_SERVICE:
        g_value_set_string (value, channel_get_message_attribute (priv->request,
            "from-service"));
        break;
      case PROP_REQUEST:
        g_value_set_object (value, priv->request);
        break;
      case PROP_REQUEST_TYPE:
        g_value_set_uint (value, channel_get_message_type (priv->request));
        break;
      case PROP_REQUEST_ATTRIBUTES:
        g_value_take_boxed (value,
            channel_get_message_attributes (priv->request));
        break;
      case PROP_REQUEST_BODY:
        g_value_take_string (value, channel_get_message_body (priv->request));
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
ytst_message_channel_set_property (GObject *object,
    guint property_id,
    const GValue *value,
    GParamSpec *pspec)
{
  YtstMessageChannel *self = YTST_MESSAGE_CHANNEL (object);
  YtstMessageChannelPrivate *priv = self->priv;

  switch (property_id)
    {
      case PROP_CONTACT:
        priv->contact = g_value_dup_object (value);
        break;
      case PROP_XMPP_CONNECTION:
        priv->xmpp_connection = g_value_dup_object (value);
        break;
      case PROP_XMPP_CONNECTION_MANAGER:
        priv->xmpp_connection_manager = g_value_dup_object (value);
        break;
      case PROP_REQUEST:
        g_assert (priv->request == NULL);
        priv->request = g_value_dup_object (value);
        g_assert (priv->request);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
ytst_message_channel_constructed (GObject *object)
{
  YtstMessageChannel *self = YTST_MESSAGE_CHANNEL (object);
  YtstMessageChannelPrivate *priv = self->priv;

  g_signal_connect (priv->xmpp_connection_manager, "connection-closed",
      G_CALLBACK (on_xmpp_connection_manager_connection_closed), self);

  if (G_OBJECT_CLASS (ytst_message_channel_parent_class)->constructed)
    G_OBJECT_CLASS (ytst_message_channel_parent_class)->constructed (object);
}

static void
ytst_message_channel_dispose (GObject *object)
{
  YtstMessageChannel *self = YTST_MESSAGE_CHANNEL (object);
  YtstMessageChannelPrivate *priv = self->priv;

  if (priv->dispose_has_run)
    return;

  priv->dispose_has_run = TRUE;

  if (!g_cancellable_is_cancelled (priv->cancellable))
    g_cancellable_cancel (priv->cancellable);

  g_signal_handlers_disconnect_matched (priv->xmpp_connection_manager,
      G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, self);

  channel_connection_disconnected (self);
  g_assert (!priv->xmpp_connection);

  g_object_unref (priv->contact);
  priv->contact = NULL;

  if (priv->xmpp_connection_manager != NULL)
    {
      g_object_unref (priv->xmpp_connection_manager);
      priv->xmpp_connection_manager = NULL;
    }

  /* release any references held by the object here */

  if (G_OBJECT_CLASS (ytst_message_channel_parent_class)->dispose)
    G_OBJECT_CLASS (ytst_message_channel_parent_class)->dispose (object);
}

static void
ytst_message_channel_finalize (GObject *object)
{
  YtstMessageChannel *self = YTST_MESSAGE_CHANNEL (object);
  YtstMessageChannelPrivate *priv = self->priv;

  /* free any data held directly by the object here */

  g_object_unref (priv->cancellable);
  if (priv->request)
    g_object_unref (priv->request);

  G_OBJECT_CLASS (ytst_message_channel_parent_class)->finalize (object);
}

static void
ytst_message_channel_class_init (YtstMessageChannelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  TpBaseChannelClass *base_class = TP_BASE_CHANNEL_CLASS (klass);

  GParamSpec *param_spec;

  static TpDBusPropertiesMixinPropImpl ytstenut_props[] = {
      { "TargetService", "target-service", NULL },
      { "InitiatorService", "initiator-service", NULL },
      { "RequestType", "request-type", NULL },
      { "RequestBody", "request-body", NULL },
      { "RequestAttributes", "request-attributes", NULL },
      { NULL }
  };

  g_type_class_add_private (klass, sizeof (YtstMessageChannelPrivate));

  object_class->dispose = ytst_message_channel_dispose;
  object_class->finalize = ytst_message_channel_finalize;
  object_class->constructed = ytst_message_channel_constructed;
  object_class->get_property = ytst_message_channel_get_property;
  object_class->set_property = ytst_message_channel_set_property;

  base_class->channel_type = TP_YTS_IFACE_CHANNEL;
  base_class->interfaces = ytst_message_channel_interfaces;
  base_class->target_handle_type = TP_HANDLE_TYPE_CONTACT;
  base_class->close = ytst_message_channel_close;

  param_spec = g_param_spec_object (
      "contact",
      "SalutContact object",
      "Salut Contact to which this channel is dedicated",
      SALUT_TYPE_CONTACT,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_CONTACT, param_spec);

  param_spec = g_param_spec_object (
      "xmpp-connection",
      "GibberXmppConnection object",
      "Gibber XMPP Connection used for communication",
      GIBBER_TYPE_XMPP_CONNECTION,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_XMPP_CONNECTION,
      param_spec);

  param_spec = g_param_spec_object (
      "xmpp-connection-manager",
      "SalutXmppConnectionManager object",
      "Salut XMPP Connection manager used for this IM channel",
      SALUT_TYPE_XMPP_CONNECTION_MANAGER,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_XMPP_CONNECTION_MANAGER,
      param_spec);

  param_spec = g_param_spec_object ("request", "Request Stanza",
      "The stanza of the request iq", WOCKY_TYPE_STANZA,
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_REQUEST,
      param_spec);

  param_spec = g_param_spec_string ("target-service", "Target Service",
      "Target Ytstenut Service Name", "",
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_TARGET_SERVICE,
      param_spec);

  param_spec = g_param_spec_string ("initiator-service", "Initiator Service",
      "Initiator Ytstenut Service Name", "",
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_INITIATOR_SERVICE,
      param_spec);

  param_spec = g_param_spec_uint ("request-type", "Request Type",
      "The type of the ytstenut request message", TP_YTS_REQUEST_TYPE_GET,
      NUM_TP_YTS_REQUEST_TYPES, TP_YTS_REQUEST_TYPE_GET,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_REQUEST_TYPE, param_spec);

  param_spec = g_param_spec_string ("request-body", "Request Body",
      "The UTF-8 encoded XML body of request message", "",
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_REQUEST_BODY, param_spec);

  param_spec = g_param_spec_boxed ("request-attributes", "Request Attributes",
      "The attributes of the ytstenut request message",
      TP_HASH_TYPE_STRING_STRING_MAP,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_REQUEST_ATTRIBUTES,
      param_spec);

  tp_dbus_properties_mixin_implement_interface (object_class,
      TP_YTS_IFACE_QUARK_CHANNEL,
      tp_dbus_properties_mixin_getter_gobject_properties, NULL,
      ytstenut_props);

  wocky_xmpp_error_register_domain (ytst_message_error_get_domain ());
}

static void
channel_complete_request (YtstMessageChannel *self)
{
  YtstMessageChannelPrivate *priv = self->priv;
  DBusGMethodInvocation *invocation;
  GError *error = NULL;

  g_assert (!priv->requested);
  g_assert (priv->requesting);

  gibber_xmpp_connection_send (priv->xmpp_connection, priv->request, &error);

  invocation = priv->requesting;
  priv->requesting = NULL;

  if (error != NULL)
    {
      dbus_g_method_return_error (invocation, error);
      g_clear_error (&error);
    }
  else
    {
      priv->requested = TRUE;
      tp_yts_svc_channel_return_from_request (invocation);
    }
}

static void
on_request_connection_ready (GObject *source_object,
    GAsyncResult *res,
    gpointer user_data)
{
  YtstMessageChannel *self = YTST_MESSAGE_CHANNEL (user_data);
  YtstMessageChannelPrivate *priv = self->priv;
  GError *error = NULL;
  GibberXmppConnection *conn;

  g_assert (!priv->requested);
  g_assert (priv->requesting);

  conn = salut_xmpp_connection_manager_request_connection_finish (
      SALUT_XMPP_CONNECTION_MANAGER (source_object), res, &error);
  if (error != NULL)
    {
      dbus_g_method_return_error (priv->requesting, error);
      g_clear_error (&error);
      priv->requesting = NULL;
    }
  else
    {
      priv->xmpp_connection = conn;
      salut_xmpp_connection_manager_remove_stanza_filter (
          priv->xmpp_connection_manager, priv->xmpp_connection,
          channel_message_stanza_filter, channel_message_stanza_callback, self);
      channel_complete_request (self);
    }
}

static void
ytst_message_channel_request (TpYtsSvcChannel *channel,
    DBusGMethodInvocation *context)
{
  YtstMessageChannel *self = YTST_MESSAGE_CHANNEL (channel);
  YtstMessageChannelPrivate *priv = self->priv;
  GError *error = NULL;

  if (priv->requested || priv->requesting)
    {
      g_set_error_literal (&error, TP_ERRORS, TP_ERROR_NOT_AVAILABLE,
          "Request() has already been called");
      dbus_g_method_return_error (context, error);
      g_clear_error (&error);
      return;
    }

  priv->requesting = context;
  if (priv->xmpp_connection == NULL)
    {
      salut_xmpp_connection_manager_request_connection_async (
          priv->xmpp_connection_manager, priv->contact,
          priv->cancellable, on_request_connection_ready, self);
    }
  else
    {
      channel_complete_request (self);
    }
}

static void
ytst_message_channel_reply (TpYtsSvcChannel *channel,
    GHashTable *attributes,
    const gchar *body,
    DBusGMethodInvocation *context)
{
  YtstMessageChannel *self = YTST_MESSAGE_CHANNEL (channel);
  YtstMessageChannelPrivate *priv = self->priv;
  WockyNodeTree *body_tree = NULL;
  WockyNode *req_node;
  WockyNode *msg_node;
  const gchar *from, *to, *id;
  WockyStanza *reply;
  GError *error = NULL;

  /* Can't call this method from this side */
  if (tp_channel_get_requested (TP_CHANNEL (channel)))
    {
      g_set_error_literal (&error, TP_ERRORS, TP_ERROR_NOT_AVAILABLE,
          "Fail() may not be called on the request side of a channel");
      goto done;
    }

  /* Can't call this after a successful call */
  if (priv->replied)
    {
      g_set_error_literal (&error, TP_ERROR, TP_ERROR_NOT_AVAILABLE,
          "Fail() or Reply() has already been successfully called");
      goto done;
    }

  body_tree = parse_message_body (body, &error);
  if (body_tree == NULL)
    goto done;

  /* All attributes override anything in the body */
  set_attributes_on_body (body_tree, attributes);

  /* Add the from and to service properties as well */
  msg_node = wocky_node_tree_get_top_node (body_tree);
  wocky_node_set_attribute (msg_node, "to-service",
      channel_get_message_attribute (priv->request, "from-service"));
  wocky_node_set_attribute (msg_node, "from-service",
      channel_get_message_attribute (priv->request, "to-service"));

  req_node = wocky_stanza_get_top_node (priv->request);
  from = wocky_node_get_attribute (req_node, "from");
  to = wocky_node_get_attribute (req_node, "to");
  id = wocky_node_get_attribute (req_node, "id");
  g_return_if_fail (id != NULL);

  reply = wocky_stanza_build (WOCKY_STANZA_TYPE_IQ,
      WOCKY_STANZA_SUB_TYPE_RESULT, to, from, '@', "id", id, NULL);

  /* Now append the message node */
  wocky_node_add_node_tree (wocky_stanza_get_top_node (reply), body_tree);
  g_object_unref (body_tree);

  gibber_xmpp_connection_send (priv->xmpp_connection, reply, &error);
  g_object_unref (reply);

done:
  if (error != NULL)
    {
      dbus_g_method_return_error (context, error);
      g_clear_error (&error);
    }
  else
    {
      priv->replied = TRUE;
      tp_yts_svc_channel_return_from_reply (context);
    }
}

static void
ytst_message_channel_fail (TpYtsSvcChannel *channel,
    guint error_type,
    const gchar *stanza_error_name,
    const gchar *ytstenut_error_name,
    const gchar *text,
    DBusGMethodInvocation *context)
{
  YtstMessageChannel *self = YTST_MESSAGE_CHANNEL (channel);
  YtstMessageChannelPrivate *priv = self->priv;
  const gchar *type;
  GError *error = NULL;
  WockyStanza *reply;

  /* Can't call this method from this side */
  if (tp_channel_get_requested (TP_CHANNEL (channel)))
    {
      g_set_error_literal (&error, TP_ERRORS, TP_ERROR_NOT_AVAILABLE,
          "Fail() may not be called on the request side of a channel");
      goto done;
    }

  /* Can't call this after a successful call */
  if (priv->replied)
    {
      g_set_error_literal (&error, TP_ERRORS, TP_ERROR_NOT_AVAILABLE,
          "Fail() or Reply() has already been called");
      goto done;
    }

  /* Must be one of the valid error types */
  type = wocky_enum_to_nick (WOCKY_TYPE_XMPP_ERROR_TYPE,
      ytst_message_error_type_to_wocky (error_type));
  if (type == NULL)
    {
      g_set_error_literal (&error, TP_ERRORS, TP_ERROR_INVALID_ARGUMENT,
          "ErrorType is set to an invalid value.");
      goto done;
    }

  reply = wocky_stanza_build_iq_error (priv->request,
      '(', "error",
        '@', "type", type,
        '(', stanza_error_name, ':', WOCKY_XMPP_NS_STANZAS, ')',
        '(', ytstenut_error_name, ':', WOCKY_XMPP_NS_PUBSUB_ERRORS, ')',
        '(', "text", '$', text, ')',
      ')',
      NULL);

  gibber_xmpp_connection_send (priv->xmpp_connection, reply, &error);
  g_object_unref (reply);

done:
  if (error != NULL)
    {
      dbus_g_method_return_error (context, error);
      g_error_free (error);
    }
  else
    {
      priv->replied = TRUE;
      tp_yts_svc_channel_return_from_fail (context);
    }
}

static void
channel_ytstenut_iface_init (gpointer g_iface,
    gpointer iface_data)
{
  TpYtsSvcChannelClass *klass = (TpYtsSvcChannelClass *) g_iface;

#define IMPLEMENT(x) tp_yts_svc_channel_implement_##x (\
    klass, ytst_message_channel_##x)
  IMPLEMENT(request);
  IMPLEMENT(reply);
  IMPLEMENT(fail);
#undef IMPLEMENT
}

/* -----------------------------------------------------------------------------
 * PUBLIC METHODS
 */

YtstMessageChannel *
ytst_message_channel_new (SalutConnection *connection,
    SalutContact *contact,
    WockyStanza *request,
    TpHandle handle,
    TpHandle initiator,
    SalutXmppConnectionManager *xmpp_manager,
    GibberXmppConnection *conn)
{
  TpBaseConnection *base_conn;
  gchar *path, *id, *escaped;
  YtstMessageChannel *channel;

  g_return_val_if_fail (SALUT_IS_CONNECTION (connection), NULL);
  g_return_val_if_fail (SALUT_IS_CONTACT (contact), NULL);
  g_return_val_if_fail (WOCKY_IS_STANZA (request), NULL);
  g_return_val_if_fail (SALUT_IS_XMPP_CONNECTION_MANAGER (xmpp_manager), NULL);
  g_return_val_if_fail (!conn || GIBBER_IS_XMPP_CONNECTION (conn), NULL);

  base_conn = TP_BASE_CONNECTION (connection);

  if (!ytst_message_channel_is_ytstenut_request_with_id (request, &id))
    g_return_val_if_reached (NULL);

  escaped = tp_escape_as_identifier (id);
  g_free (id);

  path = g_strdup_printf ("%s/YtstenutChannel/%s",
      base_conn->object_path, escaped);
  g_free (escaped);

  channel = g_object_new (YTST_TYPE_MESSAGE_CHANNEL,
      "connection", connection,
      "contact", contact,
      "request", request,
      "handle", handle,
      "object-path", path,
      "initiator-handle", initiator,
      "xmpp-connection", conn,
      "xmpp-connection-manager", xmpp_manager,
      NULL);

  tp_base_channel_register (TP_BASE_CHANNEL (channel));

  g_free (path);
  return channel;
}

const gchar *
ytst_message_channel_get_id (YtstMessageChannel *self)
{
  YtstMessageChannelPrivate *priv = self->priv;
  WockyNode *top;
  const gchar *id;

  g_return_val_if_fail (YTST_IS_MESSAGE_CHANNEL (self), NULL);

  top = wocky_stanza_get_top_node (priv->request);
  id = wocky_node_get_attribute (top, "id");
  g_assert (id);
  return id;
}

gboolean
ytst_message_channel_is_ytstenut_request_with_id (WockyStanza *stanza,
    gchar **id)
{
  WockyNode *top, *message;
  WockyStanzaType type = WOCKY_STANZA_TYPE_NONE;
  WockyStanzaSubType sub_type = WOCKY_STANZA_SUB_TYPE_NONE;
  const gchar *value;

  g_return_val_if_fail (WOCKY_IS_STANZA (stanza), FALSE);

  /* Check that it's an iq of get/set type */
  wocky_stanza_get_type_info (stanza, &type, &sub_type);
  if (type != WOCKY_STANZA_TYPE_IQ)
    return FALSE;
  if (sub_type != WOCKY_STANZA_SUB_TYPE_GET &&
      sub_type != WOCKY_STANZA_SUB_TYPE_SET)
    return FALSE;

  /* It must have an id */
  top = wocky_stanza_get_top_node (stanza);
  value = wocky_node_get_attribute (top, "id");
  if (value == NULL)
    return FALSE;

  /* And it must have one (and only one) child element in ytstenut ns */
  message = wocky_node_get_first_child (top);
  if (!message || wocky_strdiff (message->name, EL_YTSTENUT_MESSAGE) ||
      !wocky_node_has_ns (message, YTST_MESSAGE_NS))
    return FALSE;

  if (id != NULL)
    *id = g_strdup (value);
  return TRUE;
}

WockyStanza *
ytst_message_channel_build_request (GHashTable *request_props,
    const gchar *from,
    const gchar *to,
    GError **error)
{
  WockyStanzaSubType sub_type = WOCKY_STANZA_SUB_TYPE_NONE;
  TpYtsRequestType request_type;
  WockyStanza *request;
  const gchar *body;
  WockyNodeTree *tree;
  GHashTable *attributes;
  const gchar *initiator_service;
  const gchar *target_service;
  gchar *id;

  request_type = tp_asv_get_uint32 (request_props,
      TP_YTS_IFACE_CHANNEL ".RequestType", NULL);
  switch (request_type)
    {
      case TP_YTS_REQUEST_TYPE_GET:
        sub_type = WOCKY_STANZA_SUB_TYPE_GET;
        break;
      case TP_YTS_REQUEST_TYPE_SET:
        sub_type = WOCKY_STANZA_SUB_TYPE_SET;
        break;
      default:
        g_set_error (error, TP_ERRORS, TP_ERROR_INVALID_ARGUMENT,
            "The RequestType property is invalid.");
        return NULL;
    }

  target_service = tp_asv_get_string (request_props,
      TP_YTS_IFACE_CHANNEL ".TargetService");
  if (target_service == NULL)
    {
      g_set_error (error, TP_ERRORS, TP_ERROR_INVALID_ARGUMENT,
          "The TargetService property must be set.");
      return NULL;
    }
  else if (!tp_dbus_check_valid_bus_name (target_service,
      TP_DBUS_NAME_TYPE_WELL_KNOWN, NULL))
    {
      g_set_error (error, TP_ERRORS, TP_ERROR_INVALID_ARGUMENT,
          "The TargetService property has an invalid syntax.");
      return NULL;
    }

  initiator_service = tp_asv_get_string (request_props,
      TP_YTS_IFACE_CHANNEL ".InitiatorService");
  if (initiator_service == NULL)
    {
      g_set_error (error, TP_ERRORS, TP_ERROR_INVALID_ARGUMENT,
          "The InitiatorService property must be set.");
      return NULL;
    }
  else if (!tp_dbus_check_valid_bus_name (initiator_service,
      TP_DBUS_NAME_TYPE_WELL_KNOWN, NULL))
    {
      g_set_error (error, TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
          "The InitiatorService property has an invalid syntax.");
      return NULL;
    }

  attributes = tp_asv_get_boxed (request_props,
      TP_YTS_IFACE_CHANNEL ".RequestAttributes",
      TP_HASH_TYPE_STRING_STRING_MAP);
  if (!attributes && tp_asv_lookup (request_props,
          TP_YTS_IFACE_CHANNEL ".RequestAttributes"))
    {
      g_set_error (error, TP_ERRORS, TP_ERROR_INVALID_ARGUMENT,
          "The RequestAttributes property is invalid.");
      return NULL;
    }

  body = tp_asv_get_string (request_props,
      TP_YTS_IFACE_CHANNEL ".RequestBody");
  tree = parse_message_body (body, error);
  if (!tree)
    {
      g_prefix_error (error, "The RequestBody property is invalid: ");
      return NULL;
    }

  if (attributes != NULL)
    set_attributes_on_body (tree, attributes);

  wocky_node_set_attribute (wocky_node_tree_get_top_node (tree),
      "from-service", initiator_service);
  wocky_node_set_attribute (wocky_node_tree_get_top_node (tree),
      "to-service", target_service);

  id = salut_generate_id ();
  request = wocky_stanza_build (WOCKY_STANZA_TYPE_IQ, sub_type, from,
      to, '@', "id", id, NULL);
  g_free (id);
  wocky_node_add_node_tree (wocky_stanza_get_top_node (request), tree);
  g_object_unref (tree);

  return request;
}
