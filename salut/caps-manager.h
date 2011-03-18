#include <glib-object.h>

#ifndef YTST_CAPS_MANAGER_H
#define YTST_CAPS_MANAGER_H

G_BEGIN_DECLS

typedef struct _YtstCapsManagerClass YtstCapsManagerClass;
typedef struct _YtstCapsManager YtstCapsManager;

struct _YtstCapsManagerClass
{
  GObjectClass parent;
};

struct _YtstCapsManager
{
  GObject parent;
};

GType ytst_caps_manager_get_type (void);

#define YTST_TYPE_CAPS_MANAGER \
  (ytst_caps_manager_get_type ())
#define YTST_CAPS_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), YTST_TYPE_CAPS_MANAGER, YtstCapsManager))
#define YTST_CAPS_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), YTST_TYPE_CAPS_MANAGER, \
                           YtstCapsManagerClass))
#define YTST_IS_CAPS_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), YTST_TYPE_CAPS_MANAGER))
#define YTST_IS_CAPS_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), YTST_TYPE_CAPS_MANAGER))
#define YTST_CAPS_MANAGER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), YTST_TYPE_CAPS_MANAGER, \
                              YtstCapsManagerClass))

G_END_DECLS

#endif /* ifndef YTST_CAPS_MANAGER_H */
