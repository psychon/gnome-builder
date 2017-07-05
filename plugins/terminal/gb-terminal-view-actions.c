/* gb-editor-view-actions.c
 *
 * Copyright (C) 2015 Sebastien Lafargue <slafargue@gnome.org>
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

#define G_LOG_DOMAIN "gb-terminal-view"

#include <glib/gi18n.h>
#include <ide.h>
#include <string.h>

#include "gb-terminal-view-actions.h"
#include "gb-terminal-view-private.h"

typedef struct
{
  VteTerminal    *terminal;
  GFile          *file;
  GOutputStream  *stream;
} SaveTask;

static void
savetask_free (gpointer data)
{
  SaveTask *savetask = (SaveTask *)data;

  if (savetask != NULL)
    {
      if (savetask->file)
        g_object_unref (savetask->file);

      g_object_unref (savetask->stream);
      g_object_unref (savetask->terminal);
    }
}

gboolean
gb_terminal_view_actions_save_finish (GbTerminalView  *view,
                                      GAsyncResult    *result,
                                      GError         **error)
{
  GTask *task = (GTask *)result;

  g_return_val_if_fail (g_task_is_valid (result, view), FALSE);

  g_return_val_if_fail (GB_IS_TERMINAL_VIEW (view), FALSE);
  g_return_val_if_fail (G_IS_TASK (result), FALSE);
  g_return_val_if_fail (G_IS_TASK (task), FALSE);

  return g_task_propagate_boolean (task, error);
}

static void
save_async (GTask        *task,
            gpointer      source_object,
            gpointer      task_data,
            GCancellable *cancellable)
{
  GbTerminalView *view = source_object;
  SaveTask *savetask = (SaveTask *)task_data;
  GError *error = NULL;
  gboolean ret;

  g_assert (G_IS_TASK (task));
  g_assert (GB_IS_TERMINAL_VIEW (view));
  g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));

  if (view->selection_buffer != NULL)
    {
      g_autoptr(GInputStream) input_stream = NULL;

      input_stream = g_memory_input_stream_new_from_data (view->selection_buffer, -1, NULL);
      ret = g_output_stream_splice (G_OUTPUT_STREAM (savetask->stream),
                                    G_INPUT_STREAM (input_stream),
                                    G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
                                    cancellable,
                                    &error);

      g_clear_pointer (&view->selection_buffer, g_free);
    }
  else
    {
      ret = vte_terminal_write_contents_sync (savetask->terminal,
                                              G_OUTPUT_STREAM (savetask->stream),
                                              VTE_WRITE_DEFAULT,
                                              cancellable,
                                              &error);
    }

  if (ret)
    g_task_return_boolean (task, TRUE);
  else
    g_task_return_error (task, error);
}

static void
gb_terminal_view_actions_save_async (GbTerminalView       *view,
                                     VteTerminal          *terminal,
                                     GFile                *file,
                                     GAsyncReadyCallback   callback,
                                     GCancellable         *cancellable,
                                     gpointer              user_data)
{
  g_autoptr(GTask) task = NULL;
  g_autoptr(GFileOutputStream) output_stream = NULL;
  SaveTask *savetask;
  GError *error = NULL;

  g_return_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable));

  task = g_task_new (view, cancellable, callback, user_data);

  output_stream = g_file_replace (file, NULL, FALSE, G_FILE_CREATE_REPLACE_DESTINATION, cancellable, &error);
  if (output_stream)
    {
      savetask = g_slice_new0 (SaveTask);
      savetask->file = g_object_ref (file);
      savetask->stream = g_object_ref (output_stream);
      savetask->terminal = g_object_ref (terminal);

      g_task_set_task_data (task, savetask, savetask_free);
      g_task_run_in_thread (task, save_async);
    }
  else
    g_task_return_error (task, error);
}

static void
save_as_cb (GObject      *object,
            GAsyncResult *result,
            gpointer      user_data)
{
  GTask *task = (GTask *)result;
  GbTerminalView *view = user_data;
  SaveTask *savetask;
  GFile *file;
  GError *error = NULL;

  savetask = g_task_get_task_data (task);
  file = g_object_ref (savetask->file);

  if (!gb_terminal_view_actions_save_finish (view, result, &error))
    {
      g_object_unref (file);
      g_warning ("%s", error->message);
      g_clear_error (&error);
    }
  else
    {
      g_clear_object (&view->save_as_file_top);
      view->save_as_file_top = file;
    }
}

static void
save_cb (GObject      *object,
         GAsyncResult *result,
         gpointer      user_data)
{
  GbTerminalView *view = user_data;
  GError *error = NULL;

  if (!gb_terminal_view_actions_save_finish (view, result, &error))
    {
      g_warning ("%s", error->message);
      g_clear_error (&error);
    }
}

static GFile *
get_last_focused_terminal_file (GbTerminalView *view)
{
  GFile *file = NULL;

  if (G_IS_FILE (view->save_as_file_top))
    file = view->save_as_file_top;

  return file;
}

static VteTerminal *
get_last_focused_terminal (GbTerminalView *view)
{
  return view->terminal_top;
}

static gchar *
gb_terminal_get_selected_text (GbTerminalView  *view,
                               VteTerminal    **terminal_p)
{
  VteTerminal *terminal;
  gchar *buf = NULL;

  terminal = get_last_focused_terminal (view);
  if (terminal_p != NULL)
    *terminal_p = terminal;

  if (vte_terminal_get_has_selection (terminal))
    {
      vte_terminal_copy_primary (terminal);
      buf = gtk_clipboard_wait_for_text (gtk_clipboard_get (GDK_SELECTION_PRIMARY));
    }

  return buf;
}

static void
save_as_response (GtkWidget *widget,
                  gint       response,
                  gpointer   user_data)
{
  g_autoptr(GbTerminalView) view = user_data;
  g_autoptr(GFile) file = NULL;
  GtkFileChooser *chooser = (GtkFileChooser *)widget;
  VteTerminal *terminal;

  g_assert (GTK_IS_FILE_CHOOSER (chooser));
  g_assert (GB_IS_TERMINAL_VIEW (view));

  switch (response)
    {
    case GTK_RESPONSE_OK:
      file = gtk_file_chooser_get_file (chooser);
      terminal = get_last_focused_terminal (view);
      gb_terminal_view_actions_save_async (view, terminal, file, save_as_cb, NULL, view);
      break;

    case GTK_RESPONSE_CANCEL:
      g_free (view->selection_buffer);

    default:
      break;
    }

  gtk_widget_destroy (widget);
}

static void
gb_terminal_view_actions_save_as (GSimpleAction *action,
                                  GVariant      *param,
                                  gpointer       user_data)
{
  GbTerminalView *view = user_data;
  GtkWidget *suggested;
  GtkWidget *toplevel;
  GtkWidget *dialog;
  GFile *file = NULL;

  g_assert (GB_IS_TERMINAL_VIEW (view));

  /* We can't get this later because the dialog makes the terminal
   * unfocused and thus resets the selection
   */
  view->selection_buffer = gb_terminal_get_selected_text (view, NULL);

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (view));
  dialog = g_object_new (GTK_TYPE_FILE_CHOOSER_DIALOG,
                         "action", GTK_FILE_CHOOSER_ACTION_SAVE,
                         "do-overwrite-confirmation", TRUE,
                         "local-only", FALSE,
                         "modal", TRUE,
                         "select-multiple", FALSE,
                         "show-hidden", FALSE,
                         "transient-for", toplevel,
                         "title", _("Save Terminal Content As"),
                         NULL);

  file = get_last_focused_terminal_file (view);
  if (file != NULL)
    gtk_file_chooser_set_file (GTK_FILE_CHOOSER (dialog), file, NULL);

  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                          _("Cancel"), GTK_RESPONSE_CANCEL,
                          _("Save"), GTK_RESPONSE_OK,
                          NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

  suggested = gtk_dialog_get_widget_for_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
  gtk_style_context_add_class (gtk_widget_get_style_context (suggested),
                               GTK_STYLE_CLASS_SUGGESTED_ACTION);

  g_signal_connect (dialog, "response", G_CALLBACK (save_as_response), g_object_ref (view));

  gtk_window_present (GTK_WINDOW (dialog));
}

static void
gb_terminal_view_actions_save (GSimpleAction *action,
                               GVariant      *param,
                               gpointer       user_data)
{
  GbTerminalView *view = user_data;
  VteTerminal *terminal;
  GFile *file = NULL;

  g_assert (GB_IS_TERMINAL_VIEW (view));

  file = get_last_focused_terminal_file (view);
  if (file != NULL)
    {
      /* We can't get this later because the dialog make the terminal
       * unfocused and thus reset the selection
       */
      view->selection_buffer = gb_terminal_get_selected_text (view, &terminal);
      gb_terminal_view_actions_save_async (view, terminal, file, save_cb, NULL, view);
    }
  else
    {
      gb_terminal_view_actions_save_as (action, param, user_data);
    }
}

static GActionEntry GbTerminalViewActions[] = {
  { "save", gb_terminal_view_actions_save },
  { "save-as", gb_terminal_view_actions_save_as },
};

void
gb_terminal_view_actions_init (GbTerminalView *self)
{
  g_autoptr(GSimpleActionGroup) group = NULL;

  group = g_simple_action_group_new ();
  g_action_map_add_action_entries (G_ACTION_MAP (group), GbTerminalViewActions,
                                   G_N_ELEMENTS (GbTerminalViewActions), self);
  gtk_widget_insert_action_group (GTK_WIDGET (self), "view", G_ACTION_GROUP (group));
}
