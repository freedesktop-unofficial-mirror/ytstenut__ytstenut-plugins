#include <glib-object.h>

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

