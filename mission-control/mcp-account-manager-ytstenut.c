/*
 * mcp-account-manager-ytstenut.c
 *
 * McpAccountManagerYtstenut - a Mission Control plugin to create ytstenut
 * account to Mission Control.
 *
 * Copyright (C) 2010-2011 Collabora Ltd.
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *    Danielle Madeley <danielle.madeley@collabora.co.uk>
 *    Stef Walter <stefw@collabora.co.uk>
 */

#include "config.h"

#include "mcp-account-manager-ytstenut.h"

#include <dbus/dbus-glib.h>

#include <glib/gi18n-lib.h>

#include <telepathy-glib/telepathy-glib.h>

#include <telepathy-ytstenut-glib/telepathy-ytstenut-glib.h>

#define DEBUG g_debug
#define PLUGIN_NAME "ytstenut"
#define PLUGIN_PRIORITY (MCP_ACCOUNT_STORAGE_PLUGIN_PRIO_KEYRING + 20)
#define PLUGIN_DESCRIPTION "Provide Telepathy Accounts from ytstenut"
#define PLUGIN_PROVIDER "com.meego.xpmn.ytstenut"
#define YTSTENUT_ACCOUNT_NAME "salut/local-ytstenut/automatic_account"
#define YTSTENUT_ACCOUNT_PATH \
  TP_ACCOUNT_OBJECT_PATH_BASE YTSTENUT_ACCOUNT_NAME
#define ACCOUNT_MANAGER_PATH "/com/meego/xpmn/ytstenut/AccountManager"

/* Timeout after last release before going offline, in ms */
#define RELEASE_TIMEOUT 5000

/* properties */
enum
{
  PROP_ACCOUNT = 1,
  LAST_PROPERTY
};

/* private structure */
struct _McpAccountManagerYtstenutPrivate {
  GHashTable *hold_requests;
  TpDBusDaemon *dbus_daemon;
  TpAccount *account_proxy;
  guint timeout_id;
};

typedef struct {
  char *key;
  char *value;
} Parameter;

static const Parameter account_parameters[] = {
    { "manager", "salut" },
    { "protocol", "local-ytstenut" },
    { "ConnectAutomatically", "true" },
    { "Hidden", "true" },
    { "Enabled", "true" },
};

static const Parameter account_translated_parameters[] = {
      { "DisplayName", N_("Ytstenut Messaging") },
};

static void mcp_account_manager_ytstenut_account_storage_iface_init (
    McpAccountStorageIface *iface, gpointer iface_data);

static void mcp_account_manager_ytstenut_account_manager_iface_init (
    YtstenutSvcAccountManagerClass *iface, gpointer iface_data);

static void account_manager_hold (McpAccountManagerYtstenut *self,
    const gchar *client);

static void account_manager_release (McpAccountManagerYtstenut *self,
    const gchar *client);

G_DEFINE_TYPE_WITH_CODE (McpAccountManagerYtstenut,
    mcp_account_manager_ytstenut, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (MCP_TYPE_ACCOUNT_STORAGE,
        mcp_account_manager_ytstenut_account_storage_iface_init);
    G_IMPLEMENT_INTERFACE (YTSTENUT_TYPE_SVC_ACCOUNT_MANAGER,
        mcp_account_manager_ytstenut_account_manager_iface_init);
);

/* -----------------------------------------------------------------------------
 * INTERNAL
 */

static void
account_manager_get_all_keys (const McpAccountManager *am,
                              const gchar *account)
{
  const Parameter *param;
  guint i;

  for (i = 0; i < G_N_ELEMENTS (account_parameters); i++)
    {
      param = &account_parameters[i];
      DEBUG ("Loading %s = %s", param->key, param->value);
      mcp_account_manager_set_value (am, account, param->key, param->value);
    }

  for (i = 0; i < G_N_ELEMENTS (account_translated_parameters); i++)
    {
      const gchar *tvalue = gettext (param->value);
      param = &account_translated_parameters[i];
      DEBUG ("Loading %s = %s", param->key, tvalue);
      mcp_account_manager_set_value (am, account, param->key, tvalue);
    }
}

static gboolean
account_manager_get_key (const McpAccountManager *am,
                         const gchar *account,
                         const gchar *key)
{
  const Parameter *param;
  guint i;

  for (i = 0; i < G_N_ELEMENTS (account_parameters); i++)
    {
      param = &account_parameters[i];
      if (!tp_strdiff (param->key, key))
        {
          DEBUG ("Loading %s = %s", param->key, param->value);
          mcp_account_manager_set_value (am, account, param->key, param->value);
          return TRUE;
        }
    }

  for (i = 0; i < G_N_ELEMENTS (account_translated_parameters); i++)
    {
      param = &account_translated_parameters[i];
      if (!tp_strdiff (param->key, key))
        {
          const gchar *tvalue = gettext (param->value);
          DEBUG ("Loading %s = %s", param->key, tvalue);
          mcp_account_manager_set_value (am, account, param->key, tvalue);
          return TRUE;
        }
    }

  return TRUE;
}

static void
on_account_request_presence_ready (GObject *source, GAsyncResult *res,
                                   gpointer user_data)
{
  McpAccountManagerYtstenut *self = MCP_ACCOUNT_MANAGER_YTSTENUT (user_data);
  McpAccountManagerYtstenutPrivate *priv = self->priv;
  GError *error = NULL;

  if (!tp_account_request_presence_finish (priv->account_proxy, res, &error))
    {
      g_warning ("couldn't request change for account presence: %s",
          error->message);
      g_clear_error (&error);
    }

  /* Matches in account_manager_set_presence */
  g_object_unref (self);
}

static void
account_manager_set_presence (McpAccountManagerYtstenut *self,
                              TpConnectionPresenceType presence)
{
  McpAccountManagerYtstenutPrivate *priv = self->priv;
  GError *error = NULL;

  if (!priv->account_proxy)
    {
      priv->account_proxy = tp_account_new (priv->dbus_daemon,
          YTSTENUT_ACCOUNT_PATH, &error);
      if (error)
        {
          g_warning ("couldn't create account proxy: %s", error->message);
          g_clear_error (&error);
          return;
        }
    }

  tp_account_request_presence_async (priv->account_proxy, presence, "", "",
      on_account_request_presence_ready, g_object_ref (self));
}

static gboolean
on_release_timeout (gpointer user_data)
{
  McpAccountManagerYtstenut *self = MCP_ACCOUNT_MANAGER_YTSTENUT (user_data);
  McpAccountManagerYtstenutPrivate *priv = self->priv;

  priv->timeout_id = 0;

  if (g_hash_table_size (priv->hold_requests) == 0)
    account_manager_set_presence (self, TP_CONNECTION_PRESENCE_TYPE_OFFLINE);

  /* Remove this source */
  return FALSE;
}

static void
on_name_owner_changed (TpDBusDaemon *bus_daemon,
                       const gchar *name,
                       const gchar *new_owner,
                       gpointer user_data)
{
  McpAccountManagerYtstenut *self = MCP_ACCOUNT_MANAGER_YTSTENUT (user_data);

  /* if they fell of the bus, cancel their request for them */
  if (new_owner == NULL || new_owner[0] == '\0')
      account_manager_release (self, name);
}

static void
account_manager_hold (McpAccountManagerYtstenut *self, const gchar *client)
{
  McpAccountManagerYtstenutPrivate *priv = self->priv;
  guint count;

  count = GPOINTER_TO_UINT (g_hash_table_lookup (priv->hold_requests, client));
  g_hash_table_replace (priv->hold_requests, g_strdup (client),
      GUINT_TO_POINTER (++count));
  tp_dbus_daemon_watch_name_owner (priv->dbus_daemon, client,
      on_name_owner_changed, self, NULL);

  account_manager_set_presence (self, TP_CONNECTION_PRESENCE_TYPE_AVAILABLE);
  if (priv->timeout_id != 0)
    g_source_remove (priv->timeout_id);
}

static void
account_manager_release (McpAccountManagerYtstenut *self, const gchar *client)
{
  McpAccountManagerYtstenutPrivate *priv = self->priv;
  guint count;

  count = GPOINTER_TO_UINT (g_hash_table_lookup (priv->hold_requests, client));

  if (count > 1)
    {
      g_hash_table_replace (priv->hold_requests, g_strdup (client),
          GUINT_TO_POINTER (--count));
      return;
    }

  g_hash_table_remove (priv->hold_requests, client);
  tp_dbus_daemon_cancel_name_owner_watch (priv->dbus_daemon, client,
       on_name_owner_changed, self);

  if (g_hash_table_size (priv->hold_requests) == 0 && priv->timeout_id == 0)
    {
      priv->timeout_id = g_timeout_add_seconds (RELEASE_TIMEOUT,
          on_release_timeout, self);
    }
}

/* -----------------------------------------------------------------------------
 * OBJECT
 */

static void
mcp_account_manager_ytstenut_init (McpAccountManagerYtstenut *self)
{
  McpAccountManagerYtstenutPrivate *priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      MCP_TYPE_ACCOUNT_MANAGER_YTSTENUT, McpAccountManagerYtstenutPrivate);
  GError *error = NULL;

  self->priv = priv;
  DEBUG ("Plugin initialised");

  priv->hold_requests = g_hash_table_new_full (g_str_hash, g_str_equal,
      g_free, NULL);

  priv->dbus_daemon = tp_dbus_daemon_dup (&error);
  if (!priv->dbus_daemon)
    {
      g_warning ("can't get Tp DBus daemon wrapper: %s", error->message);
      g_error_free (error);
  }
}

static void
mcp_account_manager_ytstenut_constructed (GObject *object)
{
  McpAccountManagerYtstenut *self = MCP_ACCOUNT_MANAGER_YTSTENUT (object);
  McpAccountManagerYtstenutPrivate *priv = self->priv;

  tp_dbus_daemon_register_object (priv->dbus_daemon, ACCOUNT_MANAGER_PATH,
                                  self);
}

static void
mcp_account_manager_ytstenut_get_property (GObject *object,
                                           guint property_id,
                                           GValue *value,
                                           GParamSpec *pspec)
{
  switch (property_id)
    {
      case PROP_ACCOUNT:
        g_value_set_static_boxed (value, YTSTENUT_ACCOUNT_PATH);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
mcp_account_manager_ytstenut_dispose (GObject *object)
{
  McpAccountManagerYtstenut *self = MCP_ACCOUNT_MANAGER_YTSTENUT (object);
  McpAccountManagerYtstenutPrivate *priv = self->priv;

  g_hash_table_remove_all (priv->hold_requests);
  tp_clear_object (&priv->dbus_daemon);
  tp_clear_object (&priv->account_proxy);

  if (G_OBJECT_CLASS (mcp_account_manager_ytstenut_parent_class)->dispose)
    G_OBJECT_CLASS (mcp_account_manager_ytstenut_parent_class)->dispose (object);
}

static void
mcp_account_manager_ytstenut_finalize (GObject *object)
{
  McpAccountManagerYtstenut *self = MCP_ACCOUNT_MANAGER_YTSTENUT (object);
  McpAccountManagerYtstenutPrivate *priv = self->priv;

  g_hash_table_destroy (priv->hold_requests);

  if (G_OBJECT_CLASS (mcp_account_manager_ytstenut_parent_class)->finalize)
    G_OBJECT_CLASS (mcp_account_manager_ytstenut_parent_class)->finalize (object);
}

static void
mcp_account_manager_ytstenut_class_init (McpAccountManagerYtstenutClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  static TpDBusPropertiesMixinPropImpl account_manager_props[] = {
      { "Account", "account", NULL },
      { NULL }
  };

  static TpDBusPropertiesMixinIfaceImpl prop_interfaces[] = {
      { YTSTENUT_IFACE_ACCOUNT_MANAGER,
        tp_dbus_properties_mixin_getter_gobject_properties,
        NULL,
        account_manager_props,
      },
      { NULL }
  };

  object_class->constructed = mcp_account_manager_ytstenut_constructed;
  object_class->get_property = mcp_account_manager_ytstenut_get_property;
  object_class->dispose = mcp_account_manager_ytstenut_dispose;
  object_class->finalize = mcp_account_manager_ytstenut_finalize;

  g_object_class_install_property (object_class, PROP_ACCOUNT,
       g_param_spec_boxed ("account", "Account",
           "Object path to automatically created account",
           DBUS_TYPE_G_OBJECT_PATH,
           G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  klass->dbus_props_class.interfaces = prop_interfaces;
  tp_dbus_properties_mixin_class_init (object_class,
      G_STRUCT_OFFSET (McpAccountManagerYtstenutClass, dbus_props_class));

  g_type_class_add_private (klass, sizeof (McpAccountManagerYtstenutPrivate));
}

static gboolean
mcp_account_manager_ytstenut_get (const McpAccountStorage *storage,
    const McpAccountManager *manager,
    const gchar *account,
    const gchar *key)
{
  DEBUG ("%s: %s, %s", G_STRFUNC, account, key);

  if (tp_strdiff (account, YTSTENUT_ACCOUNT_NAME))
    return FALSE;

  if (key == NULL)
    {
      account_manager_get_all_keys (manager, account);
      return TRUE;
    }
  else
    {
      return account_manager_get_key (manager, account, key);
    }
}

static gboolean
mcp_account_manager_ytstenut_set (const McpAccountStorage *storage,
    const McpAccountManager * manager,
    const gchar *account,
    const gchar *key,
    const gchar *val)
{
  DEBUG ("%s: (%s, %s, %s)", G_STRFUNC, account, key, val);

  if (tp_strdiff (account, YTSTENUT_ACCOUNT_NAME))
    return FALSE;

  /*
   * TODO: Just pretend we saved. Is this really what we want to do here?
   * I copied the behavior from meego-facebook-plugins.
   */
  return TRUE;
}

static GList *
mcp_account_manager_ytstenut_list (const McpAccountStorage *storage,
    const McpAccountManager *manager)
{
  DEBUG (G_STRFUNC);
  return g_list_prepend (NULL, g_strdup (YTSTENUT_ACCOUNT_NAME));
}

static gboolean
mcp_account_manager_ytstenut_delete (const McpAccountStorage *storage,
    const McpAccountManager *manager,
    const gchar *account,
    const gchar *key)
{
  DEBUG ("%s: (%s, %s)", G_STRFUNC, account, key);

  if (tp_strdiff (account, YTSTENUT_ACCOUNT_NAME))
    return FALSE;

  /*
   * TODO: Just pretend we deleted. Is this really what we want to do here?
   * I copied the behavior from meego-facebook-plugins.
   */
  return TRUE;
}

static gboolean
mcp_account_manager_ytstenut_commit (const McpAccountStorage *storage,
    const McpAccountManager *manager)
{
  DEBUG ("%s", G_STRFUNC);
  return TRUE;
}

static void
mcp_account_manager_ytstenut_ready (const McpAccountStorage *storage,
    const McpAccountManager *manager)
{
  DEBUG (G_STRFUNC);
}

static guint
mcp_account_manager_ytstenut_get_restrictions (const McpAccountStorage *storage,
    const gchar *account)
{
  if (tp_strdiff (account, YTSTENUT_ACCOUNT_NAME))
    return 0;

  return TP_STORAGE_RESTRICTION_FLAG_CANNOT_SET_PARAMETERS |
         TP_STORAGE_RESTRICTION_FLAG_CANNOT_SET_SERVICE;
}

static void
mcp_account_manager_ytstenut_account_storage_iface_init (
    McpAccountStorageIface *iface, gpointer iface_data)
{
  mcp_account_storage_iface_set_name (iface, PLUGIN_NAME);
  mcp_account_storage_iface_set_desc (iface, PLUGIN_DESCRIPTION);
  mcp_account_storage_iface_set_priority (iface, PLUGIN_PRIORITY);
  mcp_account_storage_iface_set_provider (iface, PLUGIN_PROVIDER);

#define IMPLEMENT(x) mcp_account_storage_iface_implement_##x (\
    iface, mcp_account_manager_ytstenut_##x)
  IMPLEMENT(get);
  IMPLEMENT(list);
  IMPLEMENT(set);
  IMPLEMENT(delete);
  IMPLEMENT(commit);
  IMPLEMENT(ready);
  IMPLEMENT(get_restrictions);
#undef IMPLEMENT
}

static void
mcp_account_manager_ytstenut_hold (YtstenutSvcAccountManager *manager,
    DBusGMethodInvocation *context)
{
  McpAccountManagerYtstenut *self = MCP_ACCOUNT_MANAGER_YTSTENUT (manager);
  const char *client;

  client = dbus_g_method_get_sender (context);
  g_return_if_fail (client);

  account_manager_hold (self, client);
  ytstenut_svc_account_manager_return_from_hold (context);
}

static void
mcp_account_manager_ytstenut_release (YtstenutSvcAccountManager *manager,
    DBusGMethodInvocation *context)
{
  McpAccountManagerYtstenut *self = MCP_ACCOUNT_MANAGER_YTSTENUT (manager);
  McpAccountManagerYtstenutPrivate *priv = self->priv;
  const char *client;
  GError *error;

  client = dbus_g_method_get_sender (context);
  g_return_if_fail (client);

  if (g_hash_table_lookup (priv->hold_requests, client))
    {
      account_manager_release (self, client);
    }
  else
    {
      error = g_error_new_literal (TP_ERRORS, TP_ERROR_NOT_AVAILABLE,
          "The Hold() method must be called successfully by this caller before "
          "calling Release().");
      dbus_g_method_return_error (context, error);
      g_error_free (error);
    }

  ytstenut_svc_account_manager_return_from_release (context);
}

static void
mcp_account_manager_ytstenut_account_manager_iface_init (
    YtstenutSvcAccountManagerClass *iface, gpointer iface_data)
{
#define IMPLEMENT(x) ytstenut_svc_account_manager_implement_##x (\
    iface, mcp_account_manager_ytstenut_##x)
  IMPLEMENT(hold);
  IMPLEMENT(release);
#undef IMPLEMENT
}
