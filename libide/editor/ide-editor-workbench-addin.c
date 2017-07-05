/* ide-editor-workbench-addin.c
 *
 * Copyright (C) 2015 Christian Hergert <christian@hergert.me>
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

#define G_LOG_DOMAIN "ide-editor-workbench-addin"

#include <gtksourceview/gtksource.h>
#include <string.h>

#include "ide-context.h"
#include "ide-debug.h"

#include "buffers/ide-buffer-manager.h"
#include "buffers/ide-buffer.h"
#include "diagnostics/ide-source-location.h"
#include "editor/ide-editor-perspective.h"
#include "editor/ide-editor-workbench-addin.h"
#include "util/ide-gtk.h"
#include "util/ide-gtk.h"
#include "workbench/ide-workbench.h"
#include "workbench/ide-workbench-header-bar.h"

struct _IdeEditorWorkbenchAddin
{
  GObject               parent_instance;

  IdeWorkbench         *workbench;

  DzlDockManager       *manager;
  IdeEditorPerspective *perspective;

  GtkWidget            *new_document_button;
};

typedef struct
{
  IdeWorkbenchOpenFlags flags;
  IdeUri               *uri;
} OpenFileTaskData;

static void ide_workbench_addin_iface_init (IdeWorkbenchAddinInterface *iface);

G_DEFINE_TYPE_EXTENDED (IdeEditorWorkbenchAddin, ide_editor_workbench_addin, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (IDE_TYPE_WORKBENCH_ADDIN,
                                               ide_workbench_addin_iface_init))

static void
open_file_task_data_free (OpenFileTaskData *open_file_task_data)
{
  ide_uri_unref (open_file_task_data->uri);
  g_slice_free (OpenFileTaskData, open_file_task_data);
}

static void
ide_editor_workbench_addin_class_init (IdeEditorWorkbenchAddinClass *klass)
{
}

static void
ide_editor_workbench_addin_init (IdeEditorWorkbenchAddin *addin)
{
}

static void
ide_editor_workbench_addin_load (IdeWorkbenchAddin *addin,
                                 IdeWorkbench      *workbench)
{
  IdeEditorWorkbenchAddin *self = (IdeEditorWorkbenchAddin *)addin;
  IdeWorkbenchHeaderBar *header;

  g_assert (IDE_IS_EDITOR_WORKBENCH_ADDIN (self));
  g_assert (IDE_IS_WORKBENCH (workbench));

  self->workbench = workbench;

  self->manager = dzl_dock_manager_new ();

  header = ide_workbench_get_headerbar (workbench);

  self->new_document_button = g_object_new (GTK_TYPE_BUTTON,
                                            "action-name", "editor.new-document",
                                            "child", g_object_new (GTK_TYPE_IMAGE,
                                                                   "visible", TRUE,
                                                                   "icon-name", "document-new-symbolic",
                                                                   NULL),
                                            NULL);
  dzl_gtk_widget_add_style_class (self->new_document_button, "image-button");

  ide_workbench_header_bar_insert_left (header,
                                        self->new_document_button,
                                        GTK_PACK_START,
                                        0);

  self->perspective = g_object_new (IDE_TYPE_EDITOR_PERSPECTIVE,
                                    "manager", self->manager,
                                    "visible", TRUE,
                                    NULL);

  ide_workbench_add_perspective (workbench, IDE_PERSPECTIVE (self->perspective));
}

static void
ide_editor_workbench_addin_unload (IdeWorkbenchAddin *addin,
                                   IdeWorkbench      *workbench)
{
  IdeEditorWorkbenchAddin *self = (IdeEditorWorkbenchAddin *)addin;
  IdePerspective *perspective;

  g_assert (IDE_IS_EDITOR_WORKBENCH_ADDIN (self));
  g_assert (IDE_IS_WORKBENCH (workbench));

  gtk_widget_destroy (self->new_document_button);

  perspective = IDE_PERSPECTIVE (self->perspective);
  self->perspective = NULL;

  self->workbench = NULL;

  ide_workbench_remove_perspective (workbench, perspective);

  g_clear_object (&self->manager);
}

static gboolean
ide_editor_workbench_addin_can_open (IdeWorkbenchAddin *addin,
                                     IdeUri            *uri,
                                     const gchar       *content_type,
                                     gint              *priority)
{
  const gchar *path;

  g_assert (IDE_IS_EDITOR_WORKBENCH_ADDIN (addin));
  g_assert (uri != NULL);
  g_assert (priority != NULL);

  *priority = 0;

  path = ide_uri_get_path (uri);

  if ((path != NULL) || (content_type != NULL))
    {
      GtkSourceLanguageManager *manager;
      GtkSourceLanguage *language;

      manager = gtk_source_language_manager_get_default ();
      language = gtk_source_language_manager_guess_language (manager, path, content_type);

      if (language != NULL)
        return TRUE;
    }

  if (content_type != NULL)
    {
      gchar *text_type;
      gboolean ret;

      text_type = g_content_type_from_mime_type ("text/plain");
      ret = g_content_type_is_a (content_type, text_type);
      g_free (text_type);

      return ret;
    }

  return FALSE;
}

static void
ide_editor_workbench_addin_open_cb (GObject      *object,
                                    GAsyncResult *result,
                                    gpointer      user_data)
{
  IdeBufferManager *buffer_manager = (IdeBufferManager *)object;
  IdeEditorWorkbenchAddin *self;
  g_autoptr(IdeBuffer) buffer = NULL;
  g_autoptr(GTask) task = user_data;
  GError *error = NULL;
  const gchar *fragment;
  OpenFileTaskData *open_file_task_data;
  IdeUri *uri;

  g_assert (IDE_IS_BUFFER_MANAGER (buffer_manager));
  g_assert (G_IS_TASK (task));

  self = g_task_get_source_object (task);
  g_assert (IDE_IS_EDITOR_WORKBENCH_ADDIN (self));

  open_file_task_data = g_task_get_task_data (task);

  buffer = ide_buffer_manager_load_file_finish (buffer_manager, result, &error);

  if (buffer == NULL)
    {
      IDE_TRACE_MSG ("%s", error->message);
      g_task_return_error (task, error);
      return;
    }

  uri = open_file_task_data->uri;
  fragment = ide_uri_get_fragment (uri);

  if (fragment != NULL)
    {
      guint line = 0;
      guint column = 0;

      if (sscanf (fragment, "L%u_%u", &line, &column) >= 1)
        {
          g_autoptr(IdeSourceLocation) location = NULL;

          location = ide_source_location_new (ide_buffer_get_file (buffer), line, column, 0);
          ide_editor_perspective_focus_location (self->perspective, location);
        }
    }

  if (self->perspective != NULL && !(open_file_task_data->flags & IDE_WORKBENCH_OPEN_FLAGS_BACKGROUND))
    ide_editor_perspective_focus_buffer_in_current_stack (self->perspective, buffer);

  g_task_return_boolean (task, TRUE);
}

static void
ide_editor_workbench_addin_open_async (IdeWorkbenchAddin    *addin,
                                       IdeUri               *uri,
                                       const gchar          *content_type,
                                       IdeWorkbenchOpenFlags flags,
                                       GCancellable         *cancellable,
                                       GAsyncReadyCallback   callback,
                                       gpointer              user_data)
{
  IdeEditorWorkbenchAddin *self = (IdeEditorWorkbenchAddin *)addin;
  IdeBufferManager *buffer_manager;
  IdeContext *context;
  OpenFileTaskData *open_file_task_data;
  g_autoptr(GTask) task = NULL;
  g_autoptr(IdeFile) ifile = NULL;
  g_autoptr(GFile) gfile = NULL;

  g_assert (IDE_IS_EDITOR_WORKBENCH_ADDIN (self));
  g_assert (uri != NULL);
  g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));
  g_assert (IDE_IS_WORKBENCH (self->workbench));

  task = g_task_new (self, cancellable, callback, user_data);
  open_file_task_data = g_slice_new (OpenFileTaskData);
  open_file_task_data->flags = flags;
  open_file_task_data->uri = ide_uri_ref(uri);
  g_task_set_task_data (task, open_file_task_data, (GDestroyNotify)open_file_task_data_free);

  context = ide_workbench_get_context (self->workbench);
  buffer_manager = ide_context_get_buffer_manager (context);

  gfile = ide_uri_to_file (uri);

  if (gfile == NULL)
    {
      gchar *uristr;

      uristr = ide_uri_to_string (uri, IDE_URI_HIDE_AUTH_PARAMS);
      g_task_return_new_error (task,
                               G_IO_ERROR,
                               G_IO_ERROR_INVALID_FILENAME,
                               "Failed to create resource for \"%s\"",
                               uristr);
      g_free (uristr);
      return;
    }

  ifile = g_object_new (IDE_TYPE_FILE,
                        "context", context,
                        "file", gfile,
                        NULL);

  ide_buffer_manager_load_file_async (buffer_manager,
                                      ifile,
                                      FALSE,
                                      flags,
                                      NULL,
                                      cancellable,
                                      ide_editor_workbench_addin_open_cb,
                                      g_object_ref (task));
}

static gboolean
ide_editor_workbench_addin_open_finish (IdeWorkbenchAddin  *addin,
                                        GAsyncResult       *result,
                                        GError            **error)
{
  g_assert (IDE_IS_EDITOR_WORKBENCH_ADDIN (addin));
  g_assert (G_IS_TASK (result));

  return g_task_propagate_boolean (G_TASK (result), error);
}

static gchar *
ide_editor_workbench_addin_get_id (IdeWorkbenchAddin *addin)
{
  return g_strdup ("editor");
}

static void
ide_editor_workbench_addin_perspective_set (IdeWorkbenchAddin *addin,
                                            IdePerspective    *perspective)
{
  IdeEditorWorkbenchAddin *self = (IdeEditorWorkbenchAddin *)addin;

  g_assert (IDE_IS_EDITOR_WORKBENCH_ADDIN (self));

  gtk_widget_set_visible (self->new_document_button,
                          IDE_IS_EDITOR_PERSPECTIVE (perspective));
}

static void
ide_workbench_addin_iface_init (IdeWorkbenchAddinInterface *iface)
{
  iface->can_open = ide_editor_workbench_addin_can_open;
  iface->get_id = ide_editor_workbench_addin_get_id;
  iface->load = ide_editor_workbench_addin_load;
  iface->open_async = ide_editor_workbench_addin_open_async;
  iface->open_finish = ide_editor_workbench_addin_open_finish;
  iface->unload = ide_editor_workbench_addin_unload;
  iface->perspective_set = ide_editor_workbench_addin_perspective_set;
}
