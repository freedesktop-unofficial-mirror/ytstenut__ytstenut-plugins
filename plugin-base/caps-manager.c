/*
 * caps-manager.c - Source for YstCapsManager
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

#include "caps-manager.h"

#include <string.h>

#include <telepathy-glib/channel-manager.h>
#include <telepathy-glib/util.h>
#include <telepathy-glib/dbus.h>
#include <telepathy-glib/interfaces.h>

#include <telepathy-ytstenut-glib/telepathy-ytstenut-glib.h>

#include <wocky/wocky.h>

#ifdef SALUT
#include <salut/caps-channel-manager.h>
#else
#include <gabble/caps-channel-manager.h>
#endif

#include "utils.h"

#define DEBUG(msg, ...) \
  g_debug ("%s: " msg, G_STRFUNC, ##__VA_ARGS__)

#define CHANNEL_PREFIX "org.freedesktop.ytstenut.xpmn.Channel/"
#define UID CHANNEL_PREFIX "uid/"
#define TYPE CHANNEL_PREFIX "type/"
#define NAME CHANNEL_PREFIX "name/"
#define CAPS CHANNEL_PREFIX "caps/"
#define INTERESTED CHANNEL_PREFIX "interested/"

static void channel_manager_iface_init (gpointer g_iface, gpointer data);
static void caps_channel_manager_iface_init (gpointer g_iface, gpointer data);

G_DEFINE_TYPE_WITH_CODE (YtstCapsManager, ytst_caps_manager, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (TP_TYPE_CHANNEL_MANAGER, channel_manager_iface_init);
    G_IMPLEMENT_INTERFACE (GABBLE_TYPE_CAPS_CHANNEL_MANAGER, caps_channel_manager_iface_init);
    )

/* private structure */
struct _YtstCapsManagerPrivate
{
  GHashTable *cap_sets;
  GHashTable *services;
};

static void
ytst_caps_manager_init (YtstCapsManager *self)
{
  YtstCapsManagerPrivate *priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      YTST_TYPE_CAPS_MANAGER, YtstCapsManagerPrivate);
  self->priv = priv;

  priv->cap_sets = g_hash_table_new_full (g_str_hash, g_str_equal,
      g_free, (GDestroyNotify) gabble_capability_set_free);
  priv->services = g_hash_table_new_full (g_str_hash, g_str_equal,
      g_free, g_object_unref);
}

static void
ytst_caps_manager_dispose (GObject *object)
{
  YtstCapsManager *self = YTST_CAPS_MANAGER (object);

  tp_clear_pointer (&(self->priv->cap_sets), g_hash_table_unref);
  tp_clear_pointer (&(self->priv->services), g_hash_table_unref);

  if (G_OBJECT_CLASS (ytst_caps_manager_parent_class)->dispose)
    G_OBJECT_CLASS (ytst_caps_manager_parent_class)->dispose (object);
}

static void
ytst_caps_manager_class_init (YtstCapsManagerClass *klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);

  oclass->dispose = ytst_caps_manager_dispose;

  g_type_class_add_private (klass, sizeof (YtstCapsManagerPrivate));
}

static void
add_value_to_field (gpointer data,
    gpointer user_data)
{
  WockyNode *field = user_data;
  WockyNode *tmp;
  const gchar *content = data;

  tmp = wocky_node_add_child (field, "value");
  wocky_node_set_content (tmp, content);
}

static WockyDataForm *
make_new_data_form (const gchar *uid,
    const gchar *type,
    GPtrArray *names,
    GPtrArray *caps)
{
  WockyNode *node, *tmp;
  WockyDataForm *out;
  /* the default */
  const gchar *yts_type = type != NULL ? type : "application";
  gchar *form_type = g_strdup_printf ("%s%s", SERVICE_PREFIX, uid);

  node = wocky_node_new ("x", WOCKY_XMPP_NS_DATA);
  wocky_node_add_build (node,
      '@', "type", "result",

      '(', "field",
        '@', "var", "FORM_TYPE",
        '@', "type", "hidden",
        '(', "value",
          '$', form_type,
        ')',
      ')',

      '(', "field",
        '@', "var", "type",
        '@', "type", "text-single",
        '(', "value",
          '$', yts_type,
        ')',
      ')',

      NULL);

  g_free (form_type);

  /* do the next two by hand as we'll need to add the values
   * manually */
  if (names->len > 0)
    {
      tmp = wocky_node_add_child (node, "field");
      wocky_node_set_attribute (tmp, "var", "name");
      wocky_node_set_attribute (tmp, "type", "text-multi");
      g_ptr_array_foreach (names, add_value_to_field, tmp);
    }

  if (caps->len > 0)
    {
      tmp = wocky_node_add_child (node, "field");
      wocky_node_set_attribute (tmp, "var", "capabilities");
      wocky_node_set_attribute (tmp, "type", "text-multi");
      g_ptr_array_foreach (caps, add_value_to_field, tmp);
    }

  out = wocky_data_form_new_from_node (node, NULL);

  wocky_node_free (node);

  return out;
}

#ifdef SALUT
static void
add_to_set (gpointer key,
    gpointer value,
    gpointer user_data)
{
  gabble_capability_set_update (user_data, value);
}

static void
add_to_array (gpointer key,
    gpointer value,
    gpointer user_data)
{
  g_ptr_array_add (user_data, g_object_ref (value));
}
#endif

static void
ytst_caps_manager_represent_client (GabbleCapsChannelManager *manager,
    const gchar *client_name,
    const GPtrArray *filters,
    const gchar * const *cap_tokens,
    GabbleCapabilitySet *cap_set,
    GPtrArray *data_forms)
{
#ifdef SALUT
  YtstCapsManager *self = YTST_CAPS_MANAGER (manager);
  YtstCapsManagerPrivate *priv = self->priv;
#endif
  const gchar * const *t;

  const gchar *uid = NULL;
  const gchar *yts_type = NULL;
  GPtrArray *names = g_ptr_array_new ();
  GPtrArray *caps = g_ptr_array_new ();
  GabbleCapabilitySet *client_set;

#ifdef GABBLE
  guint i;
#endif

  client_set = gabble_capability_set_new ();

#ifdef GABBLE
  for (i = 0; i < filters->len; i++)
    {
      GHashTable *channel_class = g_ptr_array_index (filters, i);
      const gchar *service_name;
      gchar *cap;

      if (tp_strdiff (tp_asv_get_string (channel_class,
                  TP_IFACE_CHANNEL ".ChannelType"),
              TP_YTS_IFACE_CHANNEL))
        continue;

      service_name = tp_asv_get_string (channel_class,
          TP_YTS_IFACE_CHANNEL ".TargetService");

      if (service_name == NULL)
        continue;

      cap = g_strdup_printf ("%s#%s",
          YTST_SERVICE_NS, service_name);
      gabble_capability_set_add (cap_set, cap);
      g_free (cap);
    }
#endif

  for (t = cap_tokens; t != NULL && *t != NULL; t++)
    {
      const gchar *cap = *t;
      gchar *feature;

      if (g_str_has_prefix (*t, INTERESTED))
        {
          cap = *t + strlen (INTERESTED);
          feature = g_strdup_printf ("%s+notify", cap);
          gabble_capability_set_add (client_set, feature);
          g_free (feature);
        }
      else if (g_str_has_prefix (*t, UID))
        {
          uid = *t + strlen (UID);
        }
      else if (g_str_has_prefix (*t, TYPE))
        {
          yts_type = *t + strlen (TYPE);
        }
      else if (g_str_has_prefix (*t, NAME))
        {
          g_ptr_array_add (names, (gpointer) (*t + strlen (NAME)));
        }
      else if (g_str_has_prefix (*t, CAPS))
        {
          g_ptr_array_add (caps, (gpointer) (*t + strlen (CAPS)));
        }
    }

  /* So, gabble and salut have different ideas of how to save caps for
   * clients. salut is arguably wrong here as it relies on the caps
   * channel manager keeping a record of what clients can do. gabble
   * does not need this and is simpler, so doesn't need the
   * priv->cap_sets and priv->services hash tables at all.
   * We should fix salut. */

  if (uid != NULL)
    {
#ifdef SALUT
      g_hash_table_insert (priv->services,
          g_strdup (client_name),
          make_new_data_form (uid, yts_type, names, caps));
      g_hash_table_insert (priv->cap_sets,
          g_strdup (client_name), client_set);
#else
      g_ptr_array_add (data_forms,
          make_new_data_form (uid, yts_type, names, caps));
      gabble_capability_set_update (cap_set, client_set);
      gabble_capability_set_free (client_set);
#endif
    }
  else
    {
#ifdef SALUT
      g_hash_table_remove (priv->services, client_name);
      g_hash_table_remove (priv->cap_sets, client_name);
      gabble_capability_set_free (client_set);
#endif
    }

#ifdef SALUT
  g_hash_table_foreach (priv->cap_sets, add_to_set, cap_set);
  g_hash_table_foreach (priv->services, add_to_array, data_forms);
#endif

  g_ptr_array_unref (names);
  g_ptr_array_unref (caps);
}

static void
caps_channel_manager_iface_init (
    gpointer g_iface,
    gpointer data G_GNUC_UNUSED)
{
#ifdef SALUT /* sigh */
  GabbleCapsChannelManagerIface *iface = g_iface;
#else
  GabbleCapsChannelManagerInterface *iface = g_iface;
#endif

  iface->represent_client = ytst_caps_manager_represent_client;
}

static void
channel_manager_iface_init (
    gpointer g_iface,
    gpointer data G_GNUC_UNUSED)
{
}
