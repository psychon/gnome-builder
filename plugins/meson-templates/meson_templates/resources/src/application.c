{{include "license.c"}}

#include "{{module}}-application.h"
#include "{{module}}-window.h"

/**
 * SECTION:{{module}}-application
 * @title: {{Module}}Application
 * @short_description: The {{module}} application controller
 *
 * The #{{Module}}Application is responsible for managing the application
 * process. This includes showing windows, driving integration with the
 * desktop, and other global process management.
 */

struct _{{Module}}Application
{
  GtkApplication parent_instance;
};

G_DECLARE_TYPE ({{Module}}Application, {{module}}_application, GTK_TYPE_APPLICATION)

static void
{{module}}_application_activate (GApplication *app)
{
  GtkWindow *window;

  g_assert ({{MODULE}}_IS_APPLICATION (app));

  window = gtk_application_get_active_window (GTK_APPLICATION (app));
  if (window == NULL)
    window = g_object_new ({{MODULE}}_TYPE_WINDOW,
                           "application", app,
                           NULL);

  gtk_window_present (window);
}

static void
{{module}}_application_class_init ({{Module}}ApplicationClass *klass)
{
  GApplicationClass *app_class = G_APPLICATION_CLASS (klass);

  app_class->activate = {{module}}_application_activate;
}

static void
{{module}}_application_init ({{Module}}Application *self)
{
}

/**
 * {{module}}_application_new:
 *
 * Creates a new instance of the #{{Module}}Application.
 *
 * Returns: (transfer full): A #{{Module}}Application.
 */
{{Module}}Application *
{{module}}_application_new (void)
{
  return g_object_new ({{MODULE}}_TYPE_APPLICATION,
                       "application-id", "{{application_id}}",
                       NULL);
}
