/*
 * ytstenut.h - Header for YstPlugin
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

#include <glib-object.h>

#ifndef YTST_PLUGIN_H
#define YTST_PLUGIN_H

G_BEGIN_DECLS

typedef struct _YtstPluginClass YtstPluginClass;
typedef struct _YtstPlugin YtstPlugin;

struct _YtstPluginClass
{
  GObjectClass parent;
};

struct _YtstPlugin
{
  GObject parent;
};

GType ytst_plugin_get_type (void);

#define YTST_TYPE_PLUGIN \
  (ytst_plugin_get_type ())
#define YTST_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), YTST_TYPE_PLUGIN, YtstPlugin))
#define YTST_PLUGIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), YTST_TYPE_PLUGIN, \
                           YtstPluginClass))
#define YTST_IS_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), YTST_TYPE_PLUGIN))
#define YTST_IS_PLUGIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), YTST_TYPE_PLUGIN))
#define YTST_PLUGIN_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), YTST_TYPE_PLUGIN, \
                              YtstPluginClass))

G_END_DECLS

#endif /* ifndef YTST_PLUGIN_H */
