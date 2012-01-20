/*
 * status.h - Header for YtstStatus
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

#ifndef YTST_STATUS_H
#define YTST_STATUS_H

#include <glib-object.h>

#include <telepathy-glib/base-channel.h>

#include <wocky/wocky-session.h>

#include <gabble/plugin-connection.h>

G_BEGIN_DECLS

typedef struct _YtstStatus YtstStatus;
typedef struct _YtstStatusClass YtstStatusClass;
typedef struct _YtstStatusPrivate YtstStatusPrivate;

struct _YtstStatusClass {
  GObjectClass parent_class;

  TpDBusPropertiesMixinClass dbus_props_class;
};

struct _YtstStatus {
  GObject parent;
  YtstStatusPrivate *priv;
};

GType ytst_status_get_type (void);

/* TYPE MACROS */
#define YTST_TYPE_STATUS \
  (ytst_status_get_type ())
#define YTST_STATUS(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), YTST_TYPE_STATUS, YtstStatus))
#define YTST_STATUS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), YTST_TYPE_STATUS, YtstStatusClass))
#define YTST_IS_STATUS(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), YTST_TYPE_STATUS))
#define YTST_IS_STATUS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), YTST_TYPE_STATUS))
#define YTST_STATUS_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), YTST_TYPE_STATUS, YtstStatusClass))

YtstStatus * ytst_status_new (WockySession *session,
    GabblePluginConnection *connection);

G_END_DECLS

#endif /* #ifndef YTST_STATUS_H*/
