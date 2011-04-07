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

#include <salut/plugin.h>

#include <telepathy-glib/svc-generic.h>

#include <telepathy-ytstenut-glib/telepathy-ytstenut-glib.h>

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
  PROP_DISCOVERED_STATUSES = 1,
  PROP_DISCOVERED_SERVICES,
  LAST_PROPERTY
};

/* private structure */
struct _YtstStatusPrivate
{
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
#if 0
  YtstStatus *self = YTST_STATUS (object);
  YtstStatusPrivate *priv = self->priv;
#endif

  switch (property_id)
    {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
ytst_status_constructed (GObject *object)
{
  YtstStatus *self = YTST_STATUS (object);
  YtstStatusPrivate *priv = self->priv;

  priv->discovered_statuses = g_hash_table_new_full (g_str_hash, g_str_equal,
      g_free, (GDestroyNotify) g_hash_table_unref);

  priv->discovered_services = g_hash_table_new_full (g_str_hash, g_str_equal,
      g_free, (GDestroyNotify) g_hash_table_unref);
}

static void
ytst_status_dispose (GObject *object)
{
  YtstStatus *self = YTST_STATUS (object);
  YtstStatusPrivate *priv = self->priv;

  if (priv->dispose_has_run)
    return;

  priv->dispose_has_run = TRUE;

  /* release any references held by the object here */

  tp_clear_pointer (&priv->discovered_statuses, g_hash_table_unref);
  tp_clear_pointer (&priv->discovered_services, g_hash_table_unref);

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

static void
ytst_status_advertise_status (TpYtsSvcStatus *self,
    const gchar *capability,
    const gchar *service_name,
    const gchar *status,
    DBusGMethodInvocation *context)
{
  /* TODO: Implement */

  tp_yts_svc_status_return_from_advertise_status (context);
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
ytst_status_new (void)
{
  return g_object_new (YTST_TYPE_STATUS, NULL);
}
