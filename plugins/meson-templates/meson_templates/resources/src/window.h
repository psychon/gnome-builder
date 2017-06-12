{{include "license.h"}}

#ifndef {{MODULE}}_WINDOW_H
#define {{MODULE}}_WINDOW_H

#define {{MODULE}}_TYPE_WINDOW ({{module}}_window_get_type())

G_DECLARE_FINAL_TYPE ({{Module}}Window, {{module}}_window, {{MODULE}}, WINDOW, GtkApplicationWindow)

#endif /* {{MODULE}}_WINDOW_H */
