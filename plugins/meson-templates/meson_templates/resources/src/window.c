{{include "license.c"}}

#include "{{module}}-window.h"

struct _{{Module}}Window
{
  GtkApplicationWindow parent_instance;

  GtkHeaderBar *header_bar;
};

static void
{{module}}_window_class_init ({{Module}}WindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "{{application_path}}");
  gtk_widget_class_bind_template_child (widget_class, {{Module}}Window, header_bar);
}

static void
{{module}}_window_init ({{Module}}Window *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
