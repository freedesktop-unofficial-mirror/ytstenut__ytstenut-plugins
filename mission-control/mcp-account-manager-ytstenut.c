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

#include <telepathy-glib/telepathy-glib.h>

#include <glib/gi18n-lib.h>

#define DEBUG g_debug
#define PLUGIN_NAME "ytstenut"
#define PLUGIN_PRIORITY (MCP_ACCOUNT_STORAGE_PLUGIN_PRIO_KEYRING + 20)
#define PLUGIN_DESCRIPTION "Provide Telepathy Accounts from ytstenut"
#define PLUGIN_PROVIDER "com.meego.xpmn.ytstenut"
#define YTSTENUT_ACCOUNT_NAME "salut/local-ytstenut/automatic_account"

typedef struct {
  char *key;
  char *value;
} Parameter;

static const Parameter account_parameters[] = {
      { "manager", "salut" },
      { "protocol", "local-ytstenut" },
      { "Icon", "im-facebook" },
      { "Service", "facebook" },
      { "ConnectAutomatically", "true" },
      { "Hidden", "true" },
      { "Enabled", "true" },
};

static const Parameter account_translated_parameters[] = {
      { "DisplayName", N_("Ytstenut Messaging") },
};

static void mcp_account_manager_ytstenut_account_storage_iface_init (
    McpAccountStorageIface *iface);

G_DEFINE_TYPE_WITH_CODE (McpAccountManagerYtstenut,
    mcp_account_manager_ytstenut, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (MCP_TYPE_ACCOUNT_STORAGE,
        mcp_account_manager_ytstenut_account_storage_iface_init));

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

/* -----------------------------------------------------------------------------
 * OBJECT
 */

static void
mcp_account_manager_ytstenut_init (McpAccountManagerYtstenut *self)
{
  DEBUG ("Plugin initialised");
}

static void
mcp_account_manager_ytstenut_class_init (McpAccountManagerYtstenutClass *klass)
{

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
    McpAccountStorageIface *iface)
{
  mcp_account_storage_iface_set_name (iface, PLUGIN_NAME);
  mcp_account_storage_iface_set_desc (iface, PLUGIN_DESCRIPTION);
  mcp_account_storage_iface_set_priority (iface, PLUGIN_PRIORITY);
  mcp_account_storage_iface_set_provider (iface, PLUGIN_PROVIDER);

  mcp_account_storage_iface_implement_get (iface,
      mcp_account_manager_ytstenut_get);
  mcp_account_storage_iface_implement_list (iface,
      mcp_account_manager_ytstenut_list);
  mcp_account_storage_iface_implement_set (iface,
      mcp_account_manager_ytstenut_set);
  mcp_account_storage_iface_implement_delete (iface,
      mcp_account_manager_ytstenut_delete);
  mcp_account_storage_iface_implement_commit (iface,
      mcp_account_manager_ytstenut_commit);
  mcp_account_storage_iface_implement_ready (iface,
      mcp_account_manager_ytstenut_ready);
  mcp_account_storage_iface_implement_get_restrictions (iface,
      mcp_account_manager_ytstenut_get_restrictions);
}
