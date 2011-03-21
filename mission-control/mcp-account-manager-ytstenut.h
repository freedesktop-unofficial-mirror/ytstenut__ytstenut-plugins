/*
 * mcp-account-manager-ytstenut.h
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

#include <mission-control-plugins/mission-control-plugins.h>

#include <telepathy-glib/dbus-properties-mixin.h>

#ifndef __MCP_ACCOUNT_MANAGER_YTSTENUT_H__
#define __MCP_ACCOUNT_MANAGER_YTSTENUT_H__

G_BEGIN_DECLS

#define MCP_TYPE_ACCOUNT_MANAGER_YTSTENUT \
  (mcp_account_manager_ytstenut_get_type ())
#define MCP_ACCOUNT_MANAGER_YTSTENUT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MCP_TYPE_ACCOUNT_MANAGER_YTSTENUT, \
      McpAccountManagerYtstenut))
#define MCP_ACCOUNT_MANAGER_YTSTENUT_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_CAST ((obj), MCP_TYPE_ACCOUNT_MANAGER_YTSTENUT, \
      McpAccountManagerYtstenutClass))
#define MCP_IS_ACCOUNT_MANAGER_YTSTENUT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MCP_TYPE_ACCOUNT_MANAGER_YTSTENUT))
#define MCP_IS_ACCOUNT_MANAGER_YTSTENUT_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE ((obj), MCP_TYPE_ACCOUNT_MANAGER_YTSTENUT))
#define MCP_ACCOUNT_MANAGER_YTSTENUT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MCP_TYPE_ACCOUNT_MANAGER_YTSTENUT, \
      McpAccountManagerYtstenutClass))

typedef struct _McpAccountManagerYtstenut McpAccountManagerYtstenut;
typedef struct _McpAccountManagerYtstenutClass McpAccountManagerYtstenutClass;
typedef struct _McpAccountManagerYtstenutPrivate
    McpAccountManagerYtstenutPrivate;

struct _McpAccountManagerYtstenut {
  GObject parent;
  McpAccountManagerYtstenutPrivate *priv;
};

struct _McpAccountManagerYtstenutClass {
  GObjectClass parent_class;
  TpDBusPropertiesMixinClass dbus_props_class;
};

GType mcp_account_manager_ytstenut_get_type (void);

G_END_DECLS

#endif
