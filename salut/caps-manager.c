#include "config.h"

#include "caps-manager.h"

#include <string.h>

#include <telepathy-glib/channel-manager.h>

#include <salut/caps-channel-manager.h>

#define DEBUG(msg, ...) \
  g_debug ("%s: " msg, G_STRFUNC, ##__VA_ARGS__)

#define FEATURE_PREFIX "urn:ytstenut:capabilities:"
#define INTERESTED "com.meego.xpmn.ytstenut.Channel/interested/"

static void channel_manager_iface_init (gpointer g_iface, gpointer data);
static void caps_channel_manager_iface_init (gpointer g_iface, gpointer data);

G_DEFINE_TYPE_WITH_CODE (YtstCapsManager, ytst_caps_manager, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (TP_TYPE_CHANNEL_MANAGER, channel_manager_iface_init);
    G_IMPLEMENT_INTERFACE (GABBLE_TYPE_CAPS_CHANNEL_MANAGER, caps_channel_manager_iface_init);
    )

static void
ytst_caps_manager_init (YtstCapsManager *object)
{
}

static void
ytst_caps_manager_class_init (YtstCapsManagerClass *klass)
{
}

static void
ytst_caps_manager_represent_client (GabbleCapsChannelManager *manager,
    const gchar *client_name,
    const GPtrArray *filters,
    const gchar * const *cap_tokens,
    GabbleCapabilitySet *cap_set)
{
  const gchar * const *t;

  for (t = cap_tokens; t != NULL && *t != NULL; t++)
    {
      const gchar *cap = *t;
      gchar *feature;

      if (!g_str_has_prefix (*t, INTERESTED))
        continue;

      cap = *t + strlen (INTERESTED);

      feature = g_strdup_printf ("%s%s+notify", FEATURE_PREFIX, cap);

      gabble_capability_set_add (cap_set, feature);

      g_free (feature);
    }
}

static void
caps_channel_manager_iface_init (
    gpointer g_iface,
    gpointer data G_GNUC_UNUSED)
{
  GabbleCapsChannelManagerIface *iface = g_iface;

  iface->represent_client = ytst_caps_manager_represent_client;
}

static void
channel_manager_iface_init (
    gpointer g_iface,
    gpointer data G_GNUC_UNUSED)
{
}
