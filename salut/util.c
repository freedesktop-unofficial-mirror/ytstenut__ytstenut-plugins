/*
 * util.c - Source for ytstenut utilities
 * Copyright (C) 2011 Collabora Ltd.
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

#include "util.h"

#include <telepathy-ytstenut-glib/telepathy-ytstenut-glib.h>

GQuark
ytst_message_error_quark (void)
{
  static GQuark quark = 0;

  if (quark == 0)
    quark = g_quark_from_static_string (YTST_MESSAGE_NS);

  return quark;
}

WockyXmppErrorDomain *
ytst_message_error_get_domain (void)
{
  static WockyXmppErrorSpecialization codes[] = {
      { "The recipient does not have sufficient privileges to carry out the "
        "command..",
        YTST_MESSAGE_ERROR_FORBIDDEN,
        TRUE,
        WOCKY_XMPP_ERROR_TYPE_MODIFY
      },
      { "The resource could not be located.",
        YTST_MESSAGE_ERROR_ITEM_NOT_FOUND,
        TRUE,
        WOCKY_XMPP_ERROR_TYPE_MODIFY
      }
  };
  static WockyXmppErrorDomain ytstenut_errors = { 0, };

  if (G_UNLIKELY (ytstenut_errors.domain == 0))
    {
      ytstenut_errors.domain = YTST_MESSAGE_ERROR;
      ytstenut_errors.enum_type = YTST_TYPE_MESSAGE_ERROR;
      ytstenut_errors.codes = codes;
    }

  return &ytstenut_errors;
}

GType
ytst_message_error_get_type (void)
{
  static GType etype = 0;

  if (g_once_init_enter (&etype))
    {
      static const GEnumValue values[] = {
        { YTST_MESSAGE_ERROR_FORBIDDEN,
            "YTST_MESSAGE_ERROR_FORBIDDEN", "forbidden" },
        { YTST_MESSAGE_ERROR_ITEM_NOT_FOUND,
            "YTST_MESSAGE_ERROR_ITEM_NOT_FOUND", "item-not-found" },
        { 0 }
      };

      g_once_init_leave (&etype, g_enum_register_static ("YtstMessageError",
          values));
    }

  return etype;
}

gint
ytst_message_error_type_to_wocky (guint ytstenut_type)
{
  switch (ytstenut_type)
    {
      case TP_YTS_ERROR_TYPE_CANCEL:
        return WOCKY_XMPP_ERROR_TYPE_CANCEL;
      case TP_YTS_ERROR_TYPE_CONTINUE:
        return WOCKY_XMPP_ERROR_TYPE_CONTINUE;
      case TP_YTS_ERROR_TYPE_MODIFY:
        return WOCKY_XMPP_ERROR_TYPE_MODIFY;
      case TP_YTS_ERROR_TYPE_AUTH:
        return WOCKY_XMPP_ERROR_TYPE_AUTH;
      case TP_YTS_ERROR_TYPE_WAIT:
        return WOCKY_XMPP_ERROR_TYPE_WAIT;
      default:
        return -1;
    }
}

guint
ytst_message_error_type_from_wocky (gint wocky_type)
{
  switch (wocky_type)
    {
      case WOCKY_XMPP_ERROR_TYPE_CANCEL:
        return TP_YTS_ERROR_TYPE_CANCEL;
      case WOCKY_XMPP_ERROR_TYPE_CONTINUE:
        return TP_YTS_ERROR_TYPE_CONTINUE;
      case WOCKY_XMPP_ERROR_TYPE_MODIFY:
        return TP_YTS_ERROR_TYPE_MODIFY;
      case WOCKY_XMPP_ERROR_TYPE_AUTH:
        return TP_YTS_ERROR_TYPE_AUTH;
      case WOCKY_XMPP_ERROR_TYPE_WAIT:
        return TP_YTS_ERROR_TYPE_WAIT;
      default:
        g_return_val_if_reached (0);
    }
}
