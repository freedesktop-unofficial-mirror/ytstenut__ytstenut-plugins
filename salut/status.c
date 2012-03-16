/*
 * status.c - Header for YtstStatus
 *
 * Copyright (C) 2011 Intel Corp.
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

#include "config.h"

#include "status.h"

#include <string.h>

#include <salut/plugin.h>

#include <telepathy-glib/svc-generic.h>
#include <telepathy-glib/gtypes.h>

#include <telepathy-ytstenut-glib/telepathy-ytstenut-glib.h>

#include "utils.h"

#define DEBUG(msg, ...) \
  g_debug ("%s: " msg, G_STRFUNC, ##__VA_ARGS__)

static void sidecar_iface_init (SalutSidecarInterface *iface);

static void ytst_status_iface_init (TpYtsSvcStatusClass *iface);

G_DEFINE_TYPE_WITH_CODE (YtstStatus, ytst_status, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (SALUT_TYPE_SIDECAR, sidecar_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_YTS_SVC_STATUS, ytst_status_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_DBUS_PROPERTIES,
      tp_dbus_properties_mixin_iface_init);
);

/* properties */
enum
{
  PROP_SESSION = 1,
  PROP_CONNECTION,
  PROP_DISCOVERED_STATUSES,
  PROP_DISCOVERED_SERVICES,
  LAST_PROPERTY
};

/* private structure */
struct _YtstStatusPrivate
{
  WockySession *session;
  SalutPluginConnection *connection;

  guint handler_id;
  gulong capabilities_changed_id;

  /* GHashTable<gchar*,
   *     GHashTable<gchar*,
   *         GHashTable<gchar*,gchar*>>>
   */
  GHashTable *discovered_statuses;

  /* GHashTable<gchar*,
   *     GHashTable<gchar*,
   *         GValueArray(gchar*,GHashTable<gchar*,gchar*>,GPtrArray<gchar*>)>>
   */
  GHashTable *discovered_services;

  gboolean dispose_has_run;
};

/* -----------------------------------------------------------------------------
 * INTERNAL
 */


/* -----------------------------------------------------------------------------
 * OBJECT
 */

static void
ytst_status_init (YtstStatus *self)
{
  YtstStatusPrivate *priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      YTST_TYPE_STATUS, YtstStatusPrivate);
  self->priv = priv;
}

static void
ytst_status_get_property (GObject *object,
    guint property_id,
    GValue *value,
    GParamSpec *pspec)
{
  YtstStatus *self = YTST_STATUS (object);
  YtstStatusPrivate *priv = self->priv;

  switch (property_id)
    {
      case PROP_SESSION:
        g_value_set_object (value, priv->session);
        break;
      case PROP_CONNECTION:
        g_value_set_object (value, priv->connection);
        break;
      case PROP_DISCOVERED_STATUSES:
        g_value_set_boxed (value, priv->discovered_statuses);
        break;
      case PROP_DISCOVERED_SERVICES:
        g_value_set_boxed (value, priv->discovered_services);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
ytst_status_set_property (GObject *object,
    guint property_id,
    const GValue *value,
    GParamSpec *pspec)
{
  YtstStatus *self = YTST_STATUS (object);
  YtstStatusPrivate *priv = self->priv;

  switch (property_id)
    {
      case PROP_SESSION:
        priv->session = g_value_dup_object (value);
        break;
      case PROP_CONNECTION:
        priv->connection = g_value_dup_object (value);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
update_contact_status (YtstStatus *self,
    const gchar *from,
    const gchar *capability,
    const gchar *service_name,
    const gchar *status_str)
{
  YtstStatusPrivate *priv = self->priv;
  const gchar *old_status;
  gboolean emit = FALSE;

  GHashTable *capability_service_map;
  GHashTable *service_status_map;

  capability_service_map = g_hash_table_lookup (priv->discovered_statuses, from);

  if (capability_service_map == NULL)
    {
      capability_service_map = g_hash_table_new_full (g_str_hash, g_str_equal,
          g_free, (GDestroyNotify) g_hash_table_unref);
      g_hash_table_insert (priv->discovered_statuses, g_strdup (from),
          capability_service_map);
    }

  service_status_map = g_hash_table_lookup (capability_service_map,
      capability);

  if (service_status_map == NULL)
    {
      service_status_map = g_hash_table_new_full (g_str_hash, g_str_equal,
          g_free, g_free);
      g_hash_table_insert (capability_service_map,
          g_strdup (capability), service_status_map);
    }

  old_status = g_hash_table_lookup (service_status_map, service_name);

  /* Save this value as old_status will be freed when we call
   * g_hash_table_insert next and the spec says we need to update the
   * property before emitting the signal. In reality this wouldn't be
   * a problem, but let's be nice. */
  emit = tp_strdiff (old_status, status_str);

  if (status_str != NULL)
    {
      g_hash_table_insert (service_status_map, g_strdup (service_name),
          g_strdup (status_str));
    }
  else
    {
      /* remove the service from the service status map */
      g_hash_table_remove (service_status_map, service_name);

      /* now run along up the hash table cleaning up */
      if (g_hash_table_size (service_status_map) == 0)
        g_hash_table_remove (capability_service_map, capability);

      if (g_hash_table_size (capability_service_map) == 0)
        g_hash_table_remove (priv->discovered_statuses, from);
    }

  if (emit)
    tp_yts_svc_status_emit_status_changed (self, from, capability,
        service_name, status_str);
}

static gchar *
get_node_body (WockyNode *node)
{
  WockyXmppWriter *writer;
  WockyNodeTree *tree;
  const guint8 *output;
  gsize length;
  gchar *result;

  writer = wocky_xmpp_writer_new_no_stream ();
  tree = wocky_node_tree_new_from_node (node);
  wocky_xmpp_writer_write_node_tree (writer, tree, &output, &length);
  result = g_strndup ((const gchar*) output, length);
  g_object_unref (writer);
  g_object_unref (tree);

  return result;
}

static gboolean
pep_event_cb (WockyPorter *porter,
    WockyStanza *stanza,
    gpointer user_data)
{
  YtstStatus *self = user_data;
  WockyNode *message, *event, *items, *item, *status;
  gchar *status_str = NULL;
  const gchar *from, *capability, *service_name;

  message = wocky_stanza_get_top_node (stanza);

  event = wocky_node_get_first_child (message);

  if (event == NULL || tp_strdiff (event->name, "event"))
    return FALSE;

  items = wocky_node_get_first_child (event);
  if (items == NULL || tp_strdiff (items->name, "items"))
    return TRUE;

  item = wocky_node_get_first_child (items);
  if (item == NULL || tp_strdiff (item->name, "item"))
    return FALSE;

  status = wocky_node_get_first_child (item);
  if (status == NULL || tp_strdiff (status->name, "status"))
    return FALSE;

  /* looks good */

  from = wocky_stanza_get_from (stanza);
  capability = wocky_node_get_attribute (items, "node");
  service_name = wocky_node_get_attribute (status, "from-service");

  if (wocky_node_get_attribute (status, "activity") != NULL)
    status_str = get_node_body (status);

  update_contact_status (self, from, capability, service_name, status_str);

  g_free (status_str);

  return TRUE;
}

static GHashTable *
get_name_map_from_strv (const gchar **strv)
{
  GHashTable *out = g_hash_table_new_full (g_str_hash, g_str_equal,
      g_free, g_free);
  const gchar **s;

  for (s = strv; s != NULL && *s != NULL; s++)
    {
      gchar **parts = g_strsplit (*s, "/", 2);

      g_hash_table_insert (out,
          g_strdup (parts[0]), g_strdup(parts[1]));

      g_strfreev (parts);
    }

  return out;
}

static void
contact_capabilities_changed (YtstStatus *self,
    gpointer contact,
    gboolean do_signal)
{
  YtstStatusPrivate *priv = self->priv;
  const GPtrArray *data_forms;
  guint i;
  GHashTable *old, *new;
  GHashTableIter iter;
  gpointer key, value;
  gchar *jid;

  data_forms = wocky_xep_0115_capabilities_get_data_forms (
      WOCKY_XEP_0115_CAPABILITIES (contact));

  /* A bit of a hack: both SalutContact and SalutSelf implement
   * WockyXep0115Capabilities, so contact could be either one. We only
   * need the contact jid, so let's just check whether it's a
   * WockyContact or not then. */
  if (WOCKY_IS_CONTACT (contact))
    jid = wocky_contact_dup_jid (WOCKY_CONTACT (contact));
  else
    jid = g_strdup (wocky_session_get_jid (priv->session));

  old = g_hash_table_lookup (priv->discovered_services, jid);

  new = g_hash_table_new_full (g_str_hash, g_str_equal,
      g_free, (GDestroyNotify) g_value_array_free);

  for (i = 0; i < data_forms->len; i++)
    {
      WockyDataForm *form = g_ptr_array_index (data_forms, i);
      WockyDataFormField *type, *tmp;
      const gchar *form_type;
      const gchar *service;
      GValueArray *details;

      gchar *yts_service_name;
      GHashTable *yts_name_map;
      gchar **yts_caps;

      type = g_hash_table_lookup (form->fields, "FORM_TYPE");
      form_type = g_value_get_string (type->default_value);

      if (type == NULL
          || !g_str_has_prefix (form_type, SERVICE_PREFIX))
        {
          continue;
        }

      service = form_type + strlen (SERVICE_PREFIX);

      /* service type */
      tmp = g_hash_table_lookup (form->fields, "type");
      if (tmp == NULL)
        continue;
      yts_service_name = g_value_dup_string (tmp->default_value);

      /* name map */
      tmp = g_hash_table_lookup (form->fields, "name");
      if (tmp != NULL && tmp->default_value != NULL)
        {
          yts_name_map = get_name_map_from_strv (
              g_value_get_boxed (tmp->default_value));
        }
      else
        {
          yts_name_map = g_hash_table_new (g_str_hash, g_str_equal);
        }

      /* caps */
      tmp = g_hash_table_lookup (form->fields, "capabilities");
      if (tmp != NULL && tmp->default_value != NULL)
        {
          yts_caps = g_strdupv (tmp->raw_value_contents);
        }
      else
        {
          gchar *caps_tmp[] = { NULL };
          yts_caps = g_strdupv (caps_tmp);
        }

      /* now build the value array and add it to the new hash table */
      details = tp_value_array_build (3,
          G_TYPE_STRING, yts_service_name,
          TP_HASH_TYPE_STRING_STRING_MAP, yts_name_map,
          G_TYPE_STRV, yts_caps,
          G_TYPE_INVALID);

      g_hash_table_insert (new, g_strdup (service), details);

      g_free (yts_service_name);
      g_hash_table_unref (yts_name_map);
      g_strfreev (yts_caps);
    }

  if (do_signal)
    {
      /* first check for services in old but not in new; they've been
       * removed. old can be NULL. */
      if (old != NULL)
        {
          g_hash_table_iter_init (&iter, old);
          while (g_hash_table_iter_next (&iter, &key, NULL))
            {
              if (g_hash_table_lookup (new, key) == NULL)
                tp_yts_svc_status_emit_service_removed (self, jid, key);
            }
        }

      /* next check for services in new but not in old; they've been
       * added */
      g_hash_table_iter_init (&iter, new);
      while (g_hash_table_iter_next (&iter, &key, &value))
        {
          if (old == NULL || g_hash_table_lookup (old, key) == NULL)
            tp_yts_svc_status_emit_service_added (self, jid, key, value);
        }
    }

  if (g_hash_table_size (new) > 0)
    {
      g_hash_table_replace (priv->discovered_services,
          g_strdup (jid), new);
    }
  else
    {
      g_hash_table_remove (priv->discovered_services, jid);
      g_hash_table_unref (new);
    }

  g_free (jid);
}

static gboolean
capabilities_changed_cb (GSignalInvocationHint *ihint,
    guint n_param_values,
    const GValue *param_values,
    gpointer user_data)
{
  YtstStatus *self = YTST_STATUS (user_data);

  contact_capabilities_changed (self,
      g_value_get_object (param_values), TRUE);

  return TRUE;
}

static gboolean
capabilities_idle_cb (gpointer data)
{
  YtstStatus *self = YTST_STATUS (data);
  YtstStatusPrivate *priv = self->priv;
  WockyContactFactory *factory;
  GList *contacts, *l;

  /* connect to all capabilities-changed signals */
  priv->capabilities_changed_id = g_signal_add_emission_hook (
      g_signal_lookup ("capabilities-changed", WOCKY_TYPE_XEP_0115_CAPABILITIES),
      0, capabilities_changed_cb, self, NULL);

  /* and now look through all the contacts that had caps before this
   * sidecar was ensured */
  factory = wocky_session_get_contact_factory (priv->session);
  contacts = wocky_contact_factory_get_ll_contacts (factory);

  for (l = contacts; l != NULL; l = l->next)
    {
      if (WOCKY_IS_XEP_0115_CAPABILITIES (l->data))
        contact_capabilities_changed (self, l->data, FALSE);
    }

  g_list_free (contacts);

  return FALSE;
}

static void
ytst_status_constructed (GObject *object)
{
  YtstStatus *self = YTST_STATUS (object);
  YtstStatusPrivate *priv = self->priv;
  WockyPorter *porter;

  priv->discovered_statuses = g_hash_table_new_full (g_str_hash, g_str_equal,
      g_free, (GDestroyNotify) g_hash_table_unref);

  priv->discovered_services = g_hash_table_new_full (g_str_hash, g_str_equal,
      g_free, (GDestroyNotify) g_hash_table_unref);

  porter = wocky_session_get_porter (priv->session);
  priv->handler_id = wocky_porter_register_handler_from_anyone (
      porter, WOCKY_STANZA_TYPE_MESSAGE, WOCKY_STANZA_SUB_TYPE_HEADLINE,
      WOCKY_PORTER_HANDLER_PRIORITY_MAX, pep_event_cb, self,
      '(', "event",
        ':', "http://jabber.org/protocol/pubsub#event",
      ')', NULL);

  /* we need an idle for this otherwise the g_signal_lookup fails
   * giving this (not entirely sure why):
   *
   *   unable to lookup signal "capabilities-changed" for non
   *   instantiatable type `WockyXep0115Capabilities'
   */
  g_idle_add_full (G_PRIORITY_HIGH_IDLE, capabilities_idle_cb,
      self, NULL);
}

static void
ytst_status_dispose (GObject *object)
{
  YtstStatus *self = YTST_STATUS (object);
  YtstStatusPrivate *priv = self->priv;
  WockyPorter *porter;

  if (priv->dispose_has_run)
    return;

  priv->dispose_has_run = TRUE;

  /* release any references held by the object here */

  porter = wocky_session_get_porter (priv->session);
  wocky_porter_unregister_handler (porter, priv->handler_id);
  priv->handler_id = 0;

  if (priv->capabilities_changed_id > 0)
    g_signal_remove_emission_hook (
        g_signal_lookup ("capabilities-changed", WOCKY_TYPE_XEP_0115_CAPABILITIES),
        priv->capabilities_changed_id);

  tp_clear_pointer (&priv->discovered_statuses, g_hash_table_unref);
  tp_clear_pointer (&priv->discovered_services, g_hash_table_unref);

  tp_clear_object (&priv->session);
  tp_clear_object (&priv->connection);

  if (G_OBJECT_CLASS (ytst_status_parent_class)->dispose)
    G_OBJECT_CLASS (ytst_status_parent_class)->dispose (object);
}

static void
ytst_status_finalize (GObject *object)
{
#if 0
  YtstStatus *self = YTST_STATUS (object);
  YtstStatusPrivate *priv = self->priv;
#endif

  /* free any data held directly by the object here */

  G_OBJECT_CLASS (ytst_status_parent_class)->finalize (object);
}

static void
ytst_status_class_init (YtstStatusClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  GParamSpec *param_spec;

  static TpDBusPropertiesMixinPropImpl ytstenut_props[] = {
      { "DiscoveredStatuses", "discovered-statuses", NULL },
      { "DiscoveredServices", "discovered-services", NULL },
      { NULL }
  };

  g_type_class_add_private (klass, sizeof (YtstStatusPrivate));

  object_class->dispose = ytst_status_dispose;
  object_class->finalize = ytst_status_finalize;
  object_class->constructed = ytst_status_constructed;
  object_class->get_property = ytst_status_get_property;
  object_class->set_property = ytst_status_set_property;

  param_spec = g_param_spec_object (
      "session",
      "Session object",
      "WockySession object",
      WOCKY_TYPE_SESSION,
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_SESSION,
      param_spec);

  param_spec = g_param_spec_object (
      "connection",
      "Salut connection",
      "SalutPluginConnection object",
      SALUT_TYPE_PLUGIN_CONNECTION,
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_CONNECTION,
      param_spec);

  param_spec = g_param_spec_boxed (
      "discovered-statuses",
      "Discovered Statuses",
      "Discovered Ytstenut statuses",
      TP_YTS_HASH_TYPE_CONTACT_CAPABILITY_MAP,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_DISCOVERED_STATUSES,
      param_spec);

  param_spec = g_param_spec_boxed (
      "discovered-services",
      "Discovered Services",
      "Discovered Ytstenut services",
      TP_YTS_HASH_TYPE_CONTACT_SERVICE_MAP,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_DISCOVERED_SERVICES,
      param_spec);

  tp_dbus_properties_mixin_class_init (object_class,
      G_STRUCT_OFFSET (YtstStatusClass, dbus_props_class));

  tp_dbus_properties_mixin_implement_interface (object_class,
      TP_YTS_IFACE_QUARK_STATUS,
      tp_dbus_properties_mixin_getter_gobject_properties, NULL,
      ytstenut_props);
}

static WockyNodeTree *
parse_status_body (const gchar *body,
    GError **error)
{
  WockyXmppReader *reader;
  WockyNodeTree *tree;
  GError *err = NULL;

  reader = wocky_xmpp_reader_new_no_stream ();
  wocky_xmpp_reader_push (reader, (guint8 *) body, strlen (body));
  tree = WOCKY_NODE_TREE (wocky_xmpp_reader_pop_stanza (reader));
  g_object_unref (reader);

  if (tree == NULL)
    {
      err = wocky_xmpp_reader_get_error (reader);
      g_set_error (error, TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
          "Invalid XML%s%s",
          err != NULL && err->message != NULL ? ": " : ".",
          err != NULL && err->message != NULL ? err->message : "");
      g_clear_error (&err);
    }

  return tree;
}

static void
ytst_status_advertise_status (TpYtsSvcStatus *svc,
    const gchar *capability,
    const gchar *service_name,
    const gchar *status,
    DBusGMethodInvocation *context)
{
  YtstStatus *self = YTST_STATUS (svc);
  YtstStatusPrivate *priv = self->priv;
  WockyNodeTree *status_tree = NULL;
  GError *error = NULL;
  WockyStanza *stanza;
  WockyNode *item, *status_node;

  if (tp_str_empty (capability))
    {
      g_set_error (&error, TP_ERRORS, TP_ERROR_INVALID_ARGUMENT,
          "Capability argument must be set");
      goto out;
    }

  if (tp_str_empty (service_name))
    {
      g_set_error (&error, TP_ERRORS, TP_ERROR_INVALID_ARGUMENT,
          "Service name argument must be set");
      goto out;
    }

  if (!tp_str_empty (status))
    {
      status_tree = parse_status_body (status, &error);
      if (status_tree == NULL)
        goto out;
    }
  else
    {
      status_tree = wocky_node_tree_new ("status", YTST_STATUS_NS, NULL);
    }

  status_node = wocky_node_tree_get_top_node (status_tree);

  wocky_node_set_attribute (status_node, "from-service",
      service_name);
  wocky_node_set_attribute (status_node, "capability",
      capability);

  stanza = wocky_pubsub_make_event_stanza (capability,
      salut_plugin_connection_get_name (priv->connection), &item);

  wocky_node_add_node_tree (item, status_tree);
  g_object_unref (status_tree);

  wocky_send_ll_pep_event (priv->session, stanza);
  g_object_unref (stanza);

out:
  if (error == NULL)
    {
      tp_yts_svc_status_return_from_advertise_status (context);
    }
  else
    {
      dbus_g_method_return_error (context, error);
      g_clear_error (&error);
    }
}

static void
ytst_status_iface_init (TpYtsSvcStatusClass *iface)
{
#define IMPLEMENT(x) tp_yts_svc_status_implement_##x (\
    iface, ytst_status_##x)
  IMPLEMENT(advertise_status);
#undef IMPLEMENT
}

static void
sidecar_iface_init (SalutSidecarInterface *iface)
{
  iface->interface = TP_YTS_IFACE_STATUS;
  iface->get_immutable_properties = NULL;
}

/* -----------------------------------------------------------------------------
 * PUBLIC METHODS
 */

YtstStatus *
ytst_status_new (WockySession *session,
    SalutPluginConnection *connection)
{
  return g_object_new (YTST_TYPE_STATUS,
      "session", session,
      "connection", connection,
      NULL);
}
