/* ide-debugger-perspective.c
 *
 * Copyright (C) 2017 Christian Hergert <chergert@redhat.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define G_LOG_DOMAIN "ide-debugger-perspective"

#include <dazzle.h>
#include <glib/gi18n.h>

#include "ide-debug.h"

#include "buffers/ide-buffer.h"
#include "debugger/ide-debugger.h"
#include "debugger/ide-debugger-breakpoints-view.h"
#include "debugger/ide-debugger-locals-view.h"
#include "debugger/ide-debugger-libraries-view.h"
#include "debugger/ide-debugger-perspective.h"
#include "debugger/ide-debugger-registers-view.h"
#include "debugger/ide-debugger-threads-view.h"
#include "debugger/ide-debugger-view.h"
#include "layout/ide-layout-grid.h"
#include "workbench/ide-perspective.h"

struct _IdeDebuggerPerspective
{
  IdeLayout       parent_instance;

  /* Owned references */
  IdeDebugger    *debugger;
  DzlSignalGroup *debugger_signals;
  GSettings      *terminal_settings;
  GtkCssProvider *log_css;

  /* Template references */
  GtkTextBuffer              *log_buffer;
  GtkTextView                *log_text_view;
  IdeLayoutGrid              *layout_grid;
  IdeDebuggerBreakpointsView *breakpoints_view;
  IdeDebuggerLibrariesView   *libraries_view;
  IdeDebuggerLocalsView      *locals_view;
  IdeDebuggerRegistersView   *registers_view;
  IdeDebuggerThreadsView     *threads_view;
};

enum {
  PROP_0,
  PROP_DEBUGGER,
  N_PROPS
};

static gchar *
ide_debugger_perspective_get_title (IdePerspective *perspective)
{
  return g_strdup (_("Debugger"));
}

static gchar *
ide_debugger_perspective_get_id (IdePerspective *perspective)
{
  return g_strdup ("debugger");
}

static gchar *
ide_debugger_perspective_get_icon_name (IdePerspective *perspective)
{
  return g_strdup ("builder-debugger-symbolic");
}

static gchar *
ide_debugger_perspective_get_accelerator (IdePerspective *perspective)
{
  return g_strdup ("<Alt>2");
}

static void
perspective_iface_init (IdePerspectiveInterface *iface)
{
  iface->get_accelerator = ide_debugger_perspective_get_accelerator;
  iface->get_icon_name = ide_debugger_perspective_get_icon_name;
  iface->get_id = ide_debugger_perspective_get_id;
  iface->get_title = ide_debugger_perspective_get_title;
}

G_DEFINE_TYPE_WITH_CODE  (IdeDebuggerPerspective, ide_debugger_perspective, IDE_TYPE_LAYOUT,
                          G_IMPLEMENT_INTERFACE (IDE_TYPE_PERSPECTIVE, perspective_iface_init))

static GParamSpec *properties [N_PROPS];

static void
on_debugger_log (IdeDebuggerPerspective *self,
                 IdeDebuggerStream       stream,
                 GBytes                 *content,
                 IdeDebugger            *debugger)
{
  g_assert (IDE_IS_DEBUGGER_PERSPECTIVE (self));
  g_assert (IDE_IS_DEBUGGER_STREAM (stream));
  g_assert (IDE_IS_DEBUGGER (debugger));

  if (stream == IDE_DEBUGGER_CONSOLE)
    {
      const gchar *str;
      GtkTextIter iter;
      gsize len;

      str = (gchar *)g_bytes_get_data (content, &len);

      gtk_text_buffer_get_end_iter (self->log_buffer, &iter);
      gtk_text_buffer_insert (self->log_buffer, &iter, str, len);
      gtk_text_buffer_select_range (self->log_buffer, &iter, &iter);
      gtk_text_view_scroll_to_iter (self->log_text_view, &iter, 0.0, FALSE, 1.0, 1.0);
    }
}

void
ide_debugger_perspective_set_debugger (IdeDebuggerPerspective *self,
                                       IdeDebugger            *debugger)
{
  IDE_ENTRY;

  g_return_if_fail (IDE_IS_DEBUGGER_PERSPECTIVE (self));
  g_return_if_fail (!debugger || IDE_IS_DEBUGGER (debugger));

  if (g_set_object (&self->debugger, debugger))
    {
      dzl_signal_group_set_target (self->debugger_signals, debugger);
      gtk_text_buffer_set_text (self->log_buffer, "", 0);
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_DEBUGGER]);
    }

  IDE_EXIT;
}

static void
log_panel_changed_font_name (IdeDebuggerPerspective *self,
                             const gchar            *key,
                             GSettings              *settings)
{
  gchar *font_name;
  PangoFontDescription *font_desc;

  g_assert (IDE_IS_DEBUGGER_PERSPECTIVE (self));
  g_assert (g_strcmp0 (key, "font-name") == 0);
  g_assert (G_IS_SETTINGS (settings));

  font_name = g_settings_get_string (settings, key);
  font_desc = pango_font_description_from_string (font_name);

  if (font_desc != NULL)
    {
      gchar *fragment;
      gchar *css;

      fragment = dzl_pango_font_description_to_css (font_desc);
      css = g_strdup_printf ("textview { %s }", fragment);

      gtk_css_provider_load_from_data (self->log_css, css, -1, NULL);

      pango_font_description_free (font_desc);
      g_free (fragment);
      g_free (css);
    }

  g_free (font_name);
}

static void
on_debugger_stopped (IdeDebuggerPerspective *self,
                     IdeDebuggerStopReason   reason,
                     IdeDebuggerBreakpoint  *breakpoint,
                     IdeDebugger            *debugger)
{
  IDE_ENTRY;

  g_assert (IDE_IS_DEBUGGER_PERSPECTIVE (self));
  g_assert (!breakpoint || IDE_IS_DEBUGGER_BREAKPOINT (breakpoint));
  g_assert (IDE_IS_DEBUGGER (debugger));

  if (breakpoint != NULL)
    ide_debugger_perspective_navigate_to_breakpoint (self, breakpoint);

  IDE_EXIT;
}

static void
ide_debugger_perspective_bind (IdeDebuggerPerspective *self,
                               IdeDebugger            *debugger,
                               DzlSignalGroup         *debugger_signals)
{
  IDE_ENTRY;

  g_assert (IDE_IS_DEBUGGER_PERSPECTIVE (self));
  g_assert (IDE_IS_DEBUGGER (debugger));
  g_assert (DZL_IS_SIGNAL_GROUP (debugger_signals));

  ide_debugger_breakpoints_view_set_debugger (self->breakpoints_view, debugger);
  ide_debugger_libraries_view_set_debugger (self->libraries_view, debugger);
  ide_debugger_locals_view_set_debugger (self->locals_view, debugger);
  ide_debugger_registers_view_set_debugger (self->registers_view, debugger);
  ide_debugger_threads_view_set_debugger (self->threads_view, debugger);

  IDE_EXIT;
}

static void
ide_debugger_perspective_unbind (IdeDebuggerPerspective *self,
                                 DzlSignalGroup         *debugger_signals)
{
  IDE_ENTRY;

  g_assert (IDE_IS_DEBUGGER_PERSPECTIVE (self));
  g_assert (DZL_IS_SIGNAL_GROUP (debugger_signals));

  ide_debugger_breakpoints_view_set_debugger (self->breakpoints_view, NULL);
  ide_debugger_libraries_view_set_debugger (self->libraries_view, NULL);
  ide_debugger_locals_view_set_debugger (self->locals_view, NULL);
  ide_debugger_registers_view_set_debugger (self->registers_view, NULL);
  ide_debugger_threads_view_set_debugger (self->threads_view, NULL);

  IDE_EXIT;
}

static void
ide_debugger_perspective_finalize (GObject *object)
{
  IdeDebuggerPerspective *self = (IdeDebuggerPerspective *)object;

  g_clear_object (&self->debugger);
  g_clear_object (&self->debugger_signals);
  g_clear_object (&self->terminal_settings);
  g_clear_object (&self->log_css);

  G_OBJECT_CLASS (ide_debugger_perspective_parent_class)->finalize (object);
}

static void
ide_debugger_perspective_get_property (GObject    *object,
                                       guint       prop_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  IdeDebuggerPerspective *self = IDE_DEBUGGER_PERSPECTIVE (object);

  switch (prop_id)
    {
    case PROP_DEBUGGER:
      g_value_set_object (value, self->debugger);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ide_debugger_perspective_set_property (GObject      *object,
                                       guint         prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  IdeDebuggerPerspective *self = IDE_DEBUGGER_PERSPECTIVE (object);

  switch (prop_id)
    {
    case PROP_DEBUGGER:
      ide_debugger_perspective_set_debugger (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ide_debugger_perspective_class_init (IdeDebuggerPerspectiveClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = ide_debugger_perspective_finalize;
  object_class->get_property = ide_debugger_perspective_get_property;
  object_class->set_property = ide_debugger_perspective_set_property;

  properties [PROP_DEBUGGER] =
    g_param_spec_object ("debugger",
                         "Debugger",
                         "The current debugger instance",
                         IDE_TYPE_DEBUGGER,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/builder/ui/ide-debugger-perspective.ui");
  gtk_widget_class_bind_template_child (widget_class, IdeDebuggerPerspective, layout_grid);
  gtk_widget_class_bind_template_child (widget_class, IdeDebuggerPerspective, log_text_view);
  gtk_widget_class_bind_template_child (widget_class, IdeDebuggerPerspective, log_buffer);
  gtk_widget_class_bind_template_child (widget_class, IdeDebuggerPerspective, breakpoints_view);
  gtk_widget_class_bind_template_child (widget_class, IdeDebuggerPerspective, libraries_view);
  gtk_widget_class_bind_template_child (widget_class, IdeDebuggerPerspective, locals_view);
  gtk_widget_class_bind_template_child (widget_class, IdeDebuggerPerspective, registers_view);
  gtk_widget_class_bind_template_child (widget_class, IdeDebuggerPerspective, threads_view);

  g_type_ensure (IDE_TYPE_DEBUGGER_BREAKPOINTS_VIEW);
  g_type_ensure (IDE_TYPE_DEBUGGER_LOCALS_VIEW);
  g_type_ensure (IDE_TYPE_DEBUGGER_LIBRARIES_VIEW);
  g_type_ensure (IDE_TYPE_DEBUGGER_REGISTERS_VIEW);
  g_type_ensure (IDE_TYPE_DEBUGGER_THREADS_VIEW);
}

static void
ide_debugger_perspective_init (IdeDebuggerPerspective *self)
{
  GtkStyleContext *context;

  gtk_widget_init_template (GTK_WIDGET (self));

  self->debugger_signals = dzl_signal_group_new (IDE_TYPE_DEBUGGER);

  g_signal_connect_swapped (self->debugger_signals,
                            "bind",
                            G_CALLBACK (ide_debugger_perspective_bind),
                            self);

  g_signal_connect_swapped (self->debugger_signals,
                            "unbind",
                            G_CALLBACK (ide_debugger_perspective_unbind),
                            self);

  dzl_signal_group_connect_object (self->debugger_signals,
                                   "log",
                                   G_CALLBACK (on_debugger_log),
                                   self,
                                   G_CONNECT_SWAPPED);

  dzl_signal_group_connect_object (self->debugger_signals,
                                   "stopped",
                                   G_CALLBACK (on_debugger_stopped),
                                   self,
                                   G_CONNECT_SWAPPED);

  self->log_css = gtk_css_provider_new ();
  context = gtk_widget_get_style_context (GTK_WIDGET (self->log_text_view));
  gtk_style_context_add_provider (context,
                                  GTK_STYLE_PROVIDER (self->log_css),
                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  self->terminal_settings = g_settings_new ("org.gnome.builder.terminal");
  g_signal_connect_object (self->terminal_settings,
                           "changed::font-name",
                           G_CALLBACK (log_panel_changed_font_name),
                           self,
                           G_CONNECT_SWAPPED);
  log_panel_changed_font_name (self, "font-name", self->terminal_settings);
}

static void
ide_debugger_perspective_locate_by_file (GtkWidget *widget,
                                         gpointer   user_data)
{
  struct {
    const gchar     *file;
    IdeDebuggerView *view;
  } *lookup = user_data;

  if (lookup->view != NULL)
    return;

  if (IDE_IS_DEBUGGER_VIEW (widget))
    {
    }
}

void
ide_debugger_perspective_navigate_to_breakpoint (IdeDebuggerPerspective *self,
                                                 IdeDebuggerBreakpoint  *breakpoint)
{
  struct {
    const gchar     *file;
    IdeDebuggerView *view;
  } lookup = { 0 };

  IDE_ENTRY;

  g_return_if_fail (IDE_IS_DEBUGGER_PERSPECTIVE (self));
  g_return_if_fail (IDE_IS_DEBUGGER_BREAKPOINT (breakpoint));
  g_return_if_fail (IDE_IS_DEBUGGER (self->debugger));

  /*
   * To display the source for the breakpoint, first we need to discover what
   * file contains the source. If there is no file, then we need to ask the
   * IdeDebugger to retrieve the disassembly for us so that we can show
   * something "useful" to the developer.
   *
   * If we also fail to get the disassembly for the current breakpoint, we
   * need to load some dummy text into a buffer to denote to the developer
   * that technically they can click forward, but the behavior is rather
   * undefined.
   *
   * If the file on disk is out of date (due to changes behind the scenes) we
   * will likely catch that with a CRC check. We will show the file, but the
   * user will have an infobar displayed that denotes that the file is not
   * longer in sync with the debugged executable.
   */

  lookup.file = ide_debugger_breakpoint_get_file (breakpoint);
  g_return_if_fail (lookup.file != NULL);

  ide_layout_grid_foreach_view (self->layout_grid,
                                ide_debugger_perspective_locate_by_file,
                                &lookup);

  if (lookup.view != NULL)
    {
      //ide_debugger_view_scroll_to_line (lookup.view,
                                        //ide_debugger_breakpoint_get_line (breakpoint));
      gtk_widget_grab_focus (GTK_WIDGET (lookup.view));
      return;
    }

  g_print ("Need to load source\n");

  IDE_EXIT;
}
