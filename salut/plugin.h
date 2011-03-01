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

