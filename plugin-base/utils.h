/*
 * util.h - Header for ytstenut utilities
 * Copyright (C) 2011 Intel, Corp.
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

#ifndef __YTST_UTIL_H__
#define __YTST_UTIL_H__

#include <glib-object.h>

#include <wocky/wocky.h>

G_BEGIN_DECLS

#define YTST_MESSAGE_NS "urn:ytstenut:message"
#define YTST_STATUS_NS "urn:ytstenut:status"
#define YTST_CAPABILITIES_NS "urn:ytstenut:capabilities"
#define YTST_SERVICE_NS "urn:ytstenut:service"

GQuark ytst_message_error_quark (void);
#define YTST_MESSAGE_ERROR (ytst_message_error_quark ())

WockyXmppErrorDomain * ytst_message_error_get_domain (void);

GType ytst_message_error_get_type (void);
#define YTST_TYPE_MESSAGE_ERROR (ytst_message_error_get_type ())

/*< prefix=YTST_MESSAGE_ERROR >*/
typedef enum {
  YTST_MESSAGE_ERROR_FORBIDDEN = 1,
  YTST_MESSAGE_ERROR_ITEM_NOT_FOUND,
} YtstMessageError;

#define SERVICE_PREFIX YTST_CAPABILITIES_NS "#"

gint ytst_message_error_type_to_wocky (guint ytstenut_type);

guint ytst_message_error_type_from_wocky (gint wocky_type);

G_END_DECLS

#endif /* #ifndef __YTST_MESSAGE_CHANNEL_H__*/
