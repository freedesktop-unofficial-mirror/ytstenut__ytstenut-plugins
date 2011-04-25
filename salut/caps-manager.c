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

#include <wocky/wocky-data-form.h>
#include <wocky/wocky-namespaces.h>

#include <salut/caps-channel-manager.h>

#include "utils.h"

#define DEBUG(msg, ...) \
  g_debug ("%s: " msg, G_STRFUNC, ##__VA_ARGS__)

#define CHANNEL_PREFIX "com.meego.xpmn.ytstenut.Channel/"
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

static void
ytst_caps_manager_init (YtstCapsManager *object)
{
}

static void
ytst_caps_manager_class_init (YtstCapsManagerClass *klass)
{
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
        '(', "value",
          '$', yts_type,
        ')',
      ')',

      NULL);

  g_free (form_type);

  /* do the next two by hand as we'll need to add the values
   * manually */
  tmp = wocky_node_add_child (node, "field");
  wocky_node_set_attribute (tmp, "var", "name");
  g_ptr_array_foreach (names, add_value_to_field, tmp);

  tmp = wocky_node_add_child (node, "field");
  wocky_node_set_attribute (tmp, "var", "capabilities");
  g_ptr_array_foreach (caps, add_value_to_field, tmp);

  out = wocky_data_form_new_from_node (node, NULL);

  wocky_node_free (node);

  return out;
}

static void
ytst_caps_manager_represent_client (GabbleCapsChannelManager *manager,
    const gchar *client_name,
    const GPtrArray *filters,
    const gchar * const *cap_tokens,
    GabbleCapabilitySet *cap_set,
    GPtrArray *data_forms)
{
  const gchar * const *t;

  WockyDataForm *form;
  const gchar *uid = NULL;
  const gchar *yts_type = NULL;
  GPtrArray *names = g_ptr_array_new ();
  GPtrArray *caps = g_ptr_array_new ();

  for (t = cap_tokens; t != NULL && *t != NULL; t++)
    {
      const gchar *cap = *t;
      gchar *feature;

      if (g_str_has_prefix (*t, INTERESTED))
        {
          cap = *t + strlen (INTERESTED);
          feature = g_strdup_printf ("%s%s+notify", CAPS_FEATURE_PREFIX, cap);
          gabble_capability_set_add (cap_set, feature);
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

  if (uid != NULL)
    {
      form = make_new_data_form (uid, yts_type, names, caps);
      g_ptr_array_add (data_forms, form);
    }

  g_ptr_array_unref (names);
  g_ptr_array_unref (caps);
}

static void
caps_channel_manager_iface_init (
    gpointer g_iface,
    gpointer data G_GNUC_UNUSED)
{
  GabbleCapsChannelManagerIface *iface = g_iface;

  iface->represent_client = ytst_caps_manager_represent_client;
}

static void
channel_manager_iface_init (
    gpointer g_iface,
    gpointer data G_GNUC_UNUSED)
{
}
