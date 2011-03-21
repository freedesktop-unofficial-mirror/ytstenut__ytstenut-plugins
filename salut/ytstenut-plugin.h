/*
 * plugin.h -- The salut plugin
 * Copyright (C) 2011 Collabora Ltd.
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

#ifndef __YTSTENUT_PLUGIN_H__
#define __YTSTENUT_PLUGIN_H__

#include <glib-object.h>

typedef struct _YtstenutPluginClass YtstenutPluginClass;
typedef struct _YtstenutPlugin YtstenutPlugin;

struct _YtstenutPluginClass
{
  GObjectClass parent;
};

struct _YtstenutPlugin
{
  GObject parent;
};

GType ytstenut_plugin_get_type (void);

#define YTSTENUT_TYPE_PLUGIN \
  (ytstenut_plugin_get_type ())
#define YTSTENUT_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), YTSTENUT_TYPE_PLUGIN, YtstenutPlugin))
#define YTSTENUT_PLUGIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), YTSTENUT_TYPE_PLUGIN, \
                           YtstenutPluginClass))
#define YTSTENUT_IS_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), YTSTENUT_TYPE_PLUGIN))
#define YTSTENUT_IS_PLUGIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), YTSTENUT_TYPE_PLUGIN))
#define YTSTENUT_PLUGIN_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), YTSTENUT_TYPE_PLUGIN, \
                              YtstenutPluginClass))

#endif /* __YTSTENUT_PLUGIN_H__ */
