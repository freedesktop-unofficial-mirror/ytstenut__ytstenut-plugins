/*
 * message-channel.h - Header for YtstMessageChannel
 * Copyright (C) 2011 Intel, Corp.
 * Copyright (C) 2005, 2011 Collabora Ltd.
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

#ifndef __YTST_MESSAGE_CHANNEL_H__
#define __YTST_MESSAGE_CHANNEL_H__

#include <glib-object.h>

#include <telepathy-glib/base-channel.h>

#include <wocky/wocky.h>

#include <salut/plugin-connection.h>

G_BEGIN_DECLS

typedef struct _YtstMessageChannel YtstMessageChannel;
typedef struct _YtstMessageChannelClass YtstMessageChannelClass;
typedef struct _YtstMessageChannelPrivate YtstMessageChannelPrivate;

struct _YtstMessageChannelClass {
  TpBaseChannelClass parent_class;
};

struct _YtstMessageChannel {
  TpBaseChannel parent;
  YtstMessageChannelPrivate *priv;
};

GType ytst_message_channel_get_type (void);

/* TYPE MACROS */
#define YTST_TYPE_MESSAGE_CHANNEL \
  (ytst_message_channel_get_type ())
#define YTST_MESSAGE_CHANNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), YTST_TYPE_MESSAGE_CHANNEL, YtstMessageChannel))
#define YTST_MESSAGE_CHANNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), YTST_TYPE_MESSAGE_CHANNEL, \
                           YtstMessageChannelClass))
#define YTST_IS_MESSAGE_CHANNEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), YTST_TYPE_MESSAGE_CHANNEL))
#define YTST_IS_MESSAGE_CHANNEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), YTST_TYPE_MESSAGE_CHANNEL))
#define YTST_MESSAGE_CHANNEL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), YTST_TYPE_MESSAGE_CHANNEL, \
                              YtstMessageChannelClass))

YtstMessageChannel* ytst_message_channel_new (SalutPluginConnection *connection,
    WockyLLContact *contact,
    WockyStanza *request,
    TpHandle handle,
    TpHandle initiator,
    gboolean requested);

gboolean ytst_message_channel_is_ytstenut_request_with_id (
    WockyStanza *stanza, gchar **id);

WockyStanza * ytst_message_channel_build_request (GHashTable *request_props,
    const gchar *from,
    WockyLLContact *to,
    GError **error);

G_END_DECLS

#endif /* #ifndef __YTST_MESSAGE_CHANNEL_H__*/
