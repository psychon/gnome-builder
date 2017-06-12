{{include "license.c"}}

#include "config.h"

{{if enable_i18n}}#include <glib/gi18n.h>{{end}}

#include "{{module}}-application.h"

int
main (int   argc,
      char *argv[])
{
  g_autoptr({{Module}}Application) app = NULL;
  gint ret;

{{if enable_i18n}}
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
{{end}}

  app = {{module}}_application_new ();
  ret = g_application_run (G_APPLICATION (app), argc, argv);

  return ret;
}
