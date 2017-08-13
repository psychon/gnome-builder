{{include "license.h"}}

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define {{PREFIX}}_TYPE_WINDOW ({{prefix}}_window_get_type())
G_DECLARE_FINAL_TYPE ({{Prefix}}Window, {{prefix}}_window, {{PREFIX}}, WINDOW, GtkWindow)

G_END_DECLS
