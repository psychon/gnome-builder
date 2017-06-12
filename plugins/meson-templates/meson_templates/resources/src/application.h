{{include "license.h"}}

#ifndef {{MODULE}}_APPLICATION_H
#define {{MODULE}}_APPLICATION_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define {{MODULE}}_TYPE_APPLICATION ({{module}}_application_get_type())

G_DECLARE_FINAL_TYPE ({{Module}}Application, {{module}}_application, {{MODULE}}, APPLICATION, GtkApplication)

{{Module}}Application *{{module}}_application_new (void);

G_END_DECLS

#endif /* {{MODULE}}_APPLICATION_H */
