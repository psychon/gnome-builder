/* gbp-todo-panel.c
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

#define G_LOG_DOMAIN "gbp-todo-panel"

#include <ide.h>

#include "gbp-todo-item.h"
#include "gbp-todo-panel.h"

struct _GbpTodoPanel
{
  GtkBin        parent_instance;

  GtkTreeView  *tree_view;
  GbpTodoModel *model;

  guint         last_width;
  guint         relayout_source;
};

G_DEFINE_TYPE (GbpTodoPanel, gbp_todo_panel, GTK_TYPE_BIN)

enum {
  PROP_0,
  PROP_MODEL,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

static void
gbp_todo_panel_cell_data_func (GtkCellLayout   *cell_layout,
                               GtkCellRenderer *cell,
                               GtkTreeModel    *tree_model,
                               GtkTreeIter     *iter,
                               gpointer         data)
{
  g_autoptr(GbpTodoItem) item = NULL;
  g_autofree gchar *markup = NULL;
  const gchar *message;

  gtk_tree_model_get (tree_model, iter, 0, &item, -1);

  message = gbp_todo_item_get_line (item, 0);

  if (message != NULL)
    {
      g_autofree gchar *title = NULL;
      const gchar *path;
      guint lineno;

      /*
       * We don't trim the whitespace from lines so that we can keep
       * them in tact when showing tooltips. So we need to truncate
       * here for display in the pane.
       */
      while (g_ascii_isspace (*message))
        message++;

      path = gbp_todo_item_get_path (item);
      lineno = gbp_todo_item_get_lineno (item);
      title = g_strdup_printf ("%s:%u", path, lineno);
      ide_cell_renderer_fancy_take_title (IDE_CELL_RENDERER_FANCY (cell),
                                          g_steal_pointer (&title));
      ide_cell_renderer_fancy_set_body (IDE_CELL_RENDERER_FANCY (cell), message);
    }
  else
    {
      ide_cell_renderer_fancy_set_body (IDE_CELL_RENDERER_FANCY (cell), NULL);
      ide_cell_renderer_fancy_set_title (IDE_CELL_RENDERER_FANCY (cell), NULL);
    }
}

static void
gbp_todo_panel_row_activated (GbpTodoPanel      *self,
                              GtkTreePath       *tree_path,
                              GtkTreeViewColumn *column,
                              GtkTreeView       *tree_view)
{
  g_autoptr(GbpTodoItem) item = NULL;
  g_autoptr(GFile) file = NULL;
  g_autoptr(IdeUri) uri = NULL;
  g_autofree gchar *fragment = NULL;
  IdeWorkbench *workbench;
  GtkTreeModel *model;
  const gchar *path;
  GtkTreeIter iter;
  guint lineno;

  g_assert (GBP_IS_TODO_PANEL (self));
  g_assert (tree_path != NULL);
  g_assert (GTK_IS_TREE_VIEW (tree_view));

  model = gtk_tree_view_get_model (tree_view);
  gtk_tree_model_get_iter (model, &iter, tree_path);
  gtk_tree_model_get (model, &iter, 0, &item, -1);
  g_assert (GBP_IS_TODO_ITEM (item));

  workbench = ide_widget_get_workbench (GTK_WIDGET (self));
  g_assert (IDE_IS_WORKBENCH (workbench));

  path = gbp_todo_item_get_path (item);
  g_assert (path != NULL);

  if (g_path_is_absolute (path))
    {
      file = g_file_new_for_path (path);
    }
  else
    {
      IdeContext *context;
      IdeVcs *vcs;
      GFile *workdir;

      context = ide_workbench_get_context (workbench);
      vcs = ide_context_get_vcs (context);
      workdir = ide_vcs_get_working_directory (vcs);
      file = g_file_get_child (workdir, path);
    }

  uri = ide_uri_new_from_file (file);

  /* Set lineno info so that the editor can jump
   * to the location of the TODO item.
   */
  lineno = gbp_todo_item_get_lineno (item);
  fragment = g_strdup_printf ("L%u", lineno);
  ide_uri_set_fragment (uri, fragment);

  ide_workbench_open_uri_async (workbench, uri, "editor", 0, NULL, NULL, NULL);
}

static gboolean
gbp_todo_panel_query_tooltip (GbpTodoPanel *self,
                              gint          x,
                              gint          y,
                              gboolean      keyboard_mode,
                              GtkTooltip   *tooltip,
                              GtkTreeView  *tree_view)
{
  GtkTreePath *path = NULL;
  GtkTreeModel *model;

  g_assert (GBP_IS_TODO_PANEL (self));
  g_assert (GTK_IS_TOOLTIP (tooltip));
  g_assert (GTK_IS_TREE_VIEW (tree_view));

  if (NULL == (model = gtk_tree_view_get_model (tree_view)))
    return FALSE;

  if (gtk_tree_view_get_path_at_pos (tree_view, x, y, &path, NULL, NULL, NULL))
    {
      GtkTreeIter iter;

      if (gtk_tree_model_get_iter (model, &iter, path))
        {
          g_autoptr(GbpTodoItem) item = NULL;
          g_autoptr(GString) str = g_string_new ("<tt>");

          gtk_tree_model_get (model, &iter, 0, &item, -1);
          g_assert (GBP_IS_TODO_ITEM (item));

          /* only 5 lines stashed */
          for (guint i = 0; i < 5; i++)
            {
              const gchar *line = gbp_todo_item_get_line (item, i);
              g_autofree gchar *escaped = NULL;

              if (!line)
                break;

              escaped = g_markup_escape_text (line, -1);
              g_string_append (str, escaped);
              g_string_append_c (str, '\n');
            }

          g_string_append (str, "</tt>");
          gtk_tooltip_set_markup (tooltip, str->str);
        }

      gtk_tree_path_free (path);

      return TRUE;
    }

  return FALSE;
}

static gboolean
queue_relayout_in_idle (gpointer user_data)
{
  GbpTodoPanel *self = user_data;
  GtkAllocation alloc;
  guint n_columns;

  g_assert (GBP_IS_TODO_PANEL (self));

  gtk_widget_get_allocation (GTK_WIDGET (self), &alloc);

  if (alloc.width == self->last_width)
    goto cleanup;

  self->last_width = alloc.width;

  n_columns = gtk_tree_view_get_n_columns (self->tree_view);

  for (guint i = 0; i < n_columns; i++)
    {
      GtkTreeViewColumn *column;

      column = gtk_tree_view_get_column (self->tree_view, i);
      gtk_tree_view_column_queue_resize (column);
    }

cleanup:
  self->relayout_source = 0;

  return G_SOURCE_REMOVE;
}

static void
gbp_todo_panel_size_allocate (GtkWidget     *widget,
                              GtkAllocation *alloc)
{
  GbpTodoPanel *self = (GbpTodoPanel *)widget;

  g_assert (GBP_IS_TODO_PANEL (self));
  g_assert (alloc != NULL);

  GTK_WIDGET_CLASS (gbp_todo_panel_parent_class)->size_allocate (widget, alloc);

  if (self->last_width != alloc->width)
    {
      /*
       * We must perform our queued relayout from an idle callback
       * so that we don't affect this draw cycle. If we do that, we
       * will get empty content flashes for the current frame. This
       * allows us to draw the current frame slightly incorrect but
       * fixup on the next frame (which looks much nicer from a user
       * point of view).
       */
      if (self->relayout_source == 0)
        self->relayout_source =
          gdk_threads_add_idle_full (G_PRIORITY_LOW + 100,
                                     queue_relayout_in_idle,
                                     g_object_ref (self),
                                     g_object_unref);
    }
}

static void
gbp_todo_panel_destroy (GtkWidget *widget)
{
  GbpTodoPanel *self = (GbpTodoPanel *)widget;

  g_assert (GBP_IS_TODO_PANEL (self));

  if (self->tree_view != NULL)
    gtk_tree_view_set_model (self->tree_view, NULL);

  ide_clear_source (&self->relayout_source);
  g_clear_object (&self->model);

  GTK_WIDGET_CLASS (gbp_todo_panel_parent_class)->destroy (widget);
}

static void
gbp_todo_panel_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  GbpTodoPanel *self = GBP_TODO_PANEL (object);

  switch (prop_id)
    {
    case PROP_MODEL:
      g_value_set_object (value, gbp_todo_panel_get_model (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gbp_todo_panel_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  GbpTodoPanel *self = GBP_TODO_PANEL (object);

  switch (prop_id)
    {
    case PROP_MODEL:
      gbp_todo_panel_set_model (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gbp_todo_panel_class_init (GbpTodoPanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = gbp_todo_panel_get_property;
  object_class->set_property = gbp_todo_panel_set_property;

  widget_class->destroy = gbp_todo_panel_destroy;
  widget_class->size_allocate = gbp_todo_panel_size_allocate;

  properties [PROP_MODEL] =
    g_param_spec_object ("model",
                         "Model",
                         "The model for the TODO list",
                         GBP_TYPE_TODO_MODEL,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
gbp_todo_panel_init (GbpTodoPanel *self)
{
  GtkWidget *scroller;
  GtkTreeViewColumn *column;
  GtkCellRenderer *cell;

  scroller = g_object_new (GTK_TYPE_SCROLLED_WINDOW,
                           "visible", TRUE,
                           "vexpand", TRUE,
                           NULL);
  gtk_container_add (GTK_CONTAINER (self), scroller);

  self->tree_view = g_object_new (GTK_TYPE_TREE_VIEW,
                                  "activate-on-single-click", TRUE,
                                  "has-tooltip", TRUE,
                                  "headers-visible", FALSE,
                                  "visible", TRUE,
                                  NULL);
  g_signal_connect (self->tree_view,
                    "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    &self->tree_view);
  g_signal_connect_swapped (self->tree_view,
                            "row-activated",
                            G_CALLBACK (gbp_todo_panel_row_activated),
                            self);
  g_signal_connect_swapped (self->tree_view,
                            "query-tooltip",
                            G_CALLBACK (gbp_todo_panel_query_tooltip),
                            self);
  gtk_container_add (GTK_CONTAINER (scroller), GTK_WIDGET (self->tree_view));

  column = g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
                         "expand", TRUE,
                         "visible", TRUE,
                         NULL);
  gtk_tree_view_append_column (self->tree_view, column);

  cell = g_object_new (IDE_TYPE_CELL_RENDERER_FANCY,
                       "visible", TRUE,
                       "xalign", 0.0f,
                       "xpad", 4,
                       "ypad", 6,
                       NULL);
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (column), cell, TRUE);

  gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (column),
                                      cell,
                                      gbp_todo_panel_cell_data_func,
                                      NULL, NULL);
}

/**
 * gbp_todo_panel_get_model:
 * @self: a #GbpTodoPanel
 *
 * Gets the model being displayed by the treeview.
 *
 * Returns: (transfer none) (nullable): A #GbpTodoModel.
 */
GbpTodoModel *
gbp_todo_panel_get_model (GbpTodoPanel *self)
{
  g_return_val_if_fail (GBP_IS_TODO_PANEL (self), NULL);

  return self->model;
}

void
gbp_todo_panel_set_model (GbpTodoPanel *self,
                          GbpTodoModel *model)
{
  g_return_if_fail (GBP_IS_TODO_PANEL (self));
  g_return_if_fail (!model || GBP_IS_TODO_MODEL (model));

  if (g_set_object (&self->model, model))
    {
      if (self->model != NULL)
        gtk_tree_view_set_model (self->tree_view, GTK_TREE_MODEL (self->model));
      else
        gtk_tree_view_set_model (self->tree_view, NULL);

      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_MODEL]);
    }
}
