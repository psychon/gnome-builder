{{include "license.c"}}

#include <gtk/gtk.h>
#include "{{prefix}}-config.h"
#include "{{prefix}}-window.h"

struct _{{Prefix}}Window
{
  GtkWindow     parent_instance;
  GtkHeaderBar *header_bar;
  GtkLabel     *label;
};

G_DEFINE_TYPE ({{Prefix}}Window, {{prefix}}_window, GTK_TYPE_WINDOW)

static void
{{prefix}}_window_class_init ({{Prefix}}WindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "{{appid_path}}/{{prefix}}-window.ui");
  gtk_widget_class_bind_template_child (widget_class, {{Prefix}}Window, label);
}

static void
{{prefix}}_window_init ({{Prefix}}Window *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
