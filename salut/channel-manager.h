/*
 * channel-manager.h - Header for YtstChannelManager
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

#ifndef __YTST_CHANNEL_MANAGER_H__
#define __YTST_CHANNEL_MANAGER_H__

#include <glib-object.h>

#include <salut/connection.h>

G_BEGIN_DECLS

typedef struct _YtstChannelManager YtstChannelManager;
typedef struct _YtstChannelManagerClass YtstChannelManagerClass;
typedef struct _YtstChannelManagerPrivate YtstChannelManagerPrivate;

struct _YtstChannelManagerClass {
  GObjectClass parent_class;
};

struct _YtstChannelManager {
  GObject parent;
  YtstChannelManagerPrivate *priv;
};


GType ytst_channel_manager_get_type (void);

/* TYPE MACROS */
#define YTST_TYPE_CHANNEL_MANAGER \
  (ytst_channel_manager_get_type ())
#define YTST_CHANNEL_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), YTST_TYPE_CHANNEL_MANAGER, YtstChannelManager))
#define YTST_CHANNEL_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), YTST_TYPE_CHANNEL_MANAGER, \
                           YtstChannelManagerClass))
#define YTST_IS_CHANNEL_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), YTST_TYPE_CHANNEL_MANAGER))
#define YTST_IS_CHANNEL_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), YTST_TYPE_CHANNEL_MANAGER))
#define YTST_CHANNEL_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), YTST_TYPE_CHANNEL_MANAGER, \
                              YtstChannelManagerClass))

YtstChannelManager *
ytst_channel_manager_new (SalutConnection *connection);

#endif /* #ifndef __YTST_CHANNEL_MANAGER_H__*/
