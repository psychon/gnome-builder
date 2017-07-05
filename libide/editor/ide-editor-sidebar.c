/* ide-editor-sidebar.c
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

#define G_LOG_DOMAIN "ide-editor-sidebar"

#include <dazzle.h>

#include "editor/ide-editor-sidebar.h"
#include "layout/ide-layout-private.h"
#include "layout/ide-layout-stack.h"
#include "layout/ide-layout-view.h"

/**
 * SECTION:ide-editor-sidebar
 * @title: IdeEditorSidebar
 * @short_description: The left sidebar for the editor
 *
 * The #IdeEditorSidebar is the widget displayed on the left of the
 * #IdeEditorPerspective.  It contains an open document list, and then the
 * various sections that have been added to the sidebar.
 *
 * Use ide_editor_sidebar_add_section() to add a section to the sidebar.
 */

struct _IdeEditorSidebar
{
  GtkBox             parent_instance;

  /* Template widgets */
  GtkStackSwitcher  *stack_switcher;
  GtkListBox        *open_pages_list_box;
  GtkBox            *open_pages_section;
  GtkLabel          *section_title;
  GtkImage          *section_menu_button;
  GtkStack          *stack;
};

G_DEFINE_TYPE (IdeEditorSidebar, ide_editor_sidebar, GTK_TYPE_BOX)

static void
ide_editor_sidebar_update_title (IdeEditorSidebar *self)
{
  g_autofree gchar *title = NULL;
  GtkWidget *visible_child;

  g_assert (IDE_IS_EDITOR_SIDEBAR (self));

  if (NULL != (visible_child = gtk_stack_get_visible_child (self->stack)))
    gtk_container_child_get (GTK_CONTAINER (self->stack), visible_child,
                             "title", &title,
                             NULL);

  gtk_label_set_label (self->section_title, title);
}

static void
ide_editor_sidebar_stack_notify_visible_child (IdeEditorSidebar *self,
                                               GParamSpec       *pspec,
                                               GtkStack         *stack)
{
  g_assert (IDE_IS_EDITOR_SIDEBAR (self));
  g_assert (G_IS_PARAM_SPEC_OBJECT (pspec));
  g_assert (GTK_IS_STACK (stack));

  ide_editor_sidebar_update_title (self);
}

static void
ide_editor_sidebar_open_pages_row_activated (IdeEditorSidebar *self,
                                             GtkListBoxRow    *row,
                                             GtkListBox       *list_box)
{
  IdeLayoutView *view;
  GtkWidget *stack;

  g_assert (IDE_IS_EDITOR_SIDEBAR (self));
  g_assert (GTK_IS_LIST_BOX_ROW (row));
  g_assert (GTK_IS_LIST_BOX (list_box));

  view = g_object_get_data (G_OBJECT (row), "IDE_LAYOUT_VIEW");
  g_assert (IDE_IS_LAYOUT_VIEW (view));

  stack = gtk_widget_get_ancestor (GTK_WIDGET (view), IDE_TYPE_LAYOUT_STACK);
  g_assert (IDE_IS_LAYOUT_STACK (stack));

  ide_layout_stack_set_visible_child (IDE_LAYOUT_STACK (stack), view);

  gtk_widget_grab_focus (GTK_WIDGET (view));
}

static void
ide_editor_sidebar_destroy (GtkWidget *widget)
{
  IdeEditorSidebar *self = (IdeEditorSidebar *)widget;

  if (self->open_pages_list_box != NULL)
    gtk_list_box_bind_model (self->open_pages_list_box, NULL, NULL, NULL, NULL);

  GTK_WIDGET_CLASS (ide_editor_sidebar_parent_class)->destroy (widget);
}

static void
ide_editor_sidebar_class_init (IdeEditorSidebarClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->destroy = ide_editor_sidebar_destroy;

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/builder/ui/ide-editor-sidebar.ui");
  gtk_widget_class_bind_template_child (widget_class, IdeEditorSidebar, open_pages_list_box);
  gtk_widget_class_bind_template_child (widget_class, IdeEditorSidebar, open_pages_section);
  gtk_widget_class_bind_template_child (widget_class, IdeEditorSidebar, section_menu_button);
  gtk_widget_class_bind_template_child (widget_class, IdeEditorSidebar, section_title);
  gtk_widget_class_bind_template_child (widget_class, IdeEditorSidebar, stack);
  gtk_widget_class_bind_template_child (widget_class, IdeEditorSidebar, stack_switcher);
  gtk_widget_class_set_css_name (widget_class, "ideeditorsidebar");
}

static void
ide_editor_sidebar_init (IdeEditorSidebar *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_VERTICAL);

  g_signal_connect_swapped (self->open_pages_list_box,
                            "row-activated",
                            G_CALLBACK (ide_editor_sidebar_open_pages_row_activated),
                            self);

  g_signal_connect_swapped (self->stack,
                            "notify::visible-child",
                            G_CALLBACK (ide_editor_sidebar_stack_notify_visible_child),
                            self);
}

/**
 * ide_editor_sidebar_new:
 *
 * Creates a new #IdeEditorSidebar instance.
 *
 * Returns: (transfer full): A new #IdeEditorSidebar
 *
 * Since: 3.26
 */
GtkWidget *
ide_editor_sidebar_new (void)
{
  return g_object_new (IDE_TYPE_EDITOR_SIDEBAR, NULL);
}

static void
fixup_stack_switcher_button (GtkWidget *widget,
                             gpointer   user_data)
{
  g_assert (GTK_IS_RADIO_BUTTON (widget));

  /*
   * We need to set hexpand on each of the radiobuttons inside the stack
   * switcher to match our designs.
   */
  gtk_widget_set_hexpand (widget, TRUE);
}

static gint
find_position (IdeEditorSidebar *self,
               gint              priority)
{
  GList *children;
  gint position = 0;

  children = gtk_container_get_children (GTK_CONTAINER (self->stack));

  for (const GList *iter = children; iter != NULL; iter = iter->next)
    {
      GtkWidget *widget = iter->data;
      gint widget_prio = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                             "IDE_EDITOR_SIDEBAR_PRIORITY"));

      if (widget_prio > priority)
        break;

      position++;
    }

  g_list_free (children);

  return position;
}

/**
 * ide_editor_sidebar_add_section:
 * @self: a #IdeEditorSidebar
 * @id: (nullable): an optional id for the section
 * @title: the title of the section
 * @icon_name: the icon name for the section's icon
 * @menu_id: (nullable): an optional menu-id to display
 * @menu_icon_name: (nullable): an optional icon-name for displaying the menu
 * @section: the widget to display in the sidebar
 *
 * Adds a new section to the #IdeEditorSidebar.  @icon_name will be used to
 * display an icon for the section.  @title should contain the title to display
 * above the section.
 *
 * If you want to be able to switch to the section manually, you should set @id
 * so that ide_editor_sidebar_set_section_id() will allow you to use id.
 *
 * To remove your section, call gtk_widget_destroy() on @section.
 *
 * Since: 3.26
 */
void
ide_editor_sidebar_add_section (IdeEditorSidebar *self,
                                const gchar      *id,
                                const gchar      *title,
                                const gchar      *icon_name,
                                const gchar      *menu_id,
                                const gchar      *menu_icon_name,
                                GtkWidget        *section,
                                gint              priority)
{
  gint position;

  g_return_if_fail (IDE_IS_EDITOR_SIDEBAR (self));
  g_return_if_fail (title != NULL);
  g_return_if_fail (icon_name != NULL);
  g_return_if_fail (GTK_IS_WIDGET (section));

  g_object_set_data (G_OBJECT (section),
                     "IDE_EDITOR_SIDEBAR_PRIORITY",
                     GINT_TO_POINTER (priority));

  g_object_set_data (G_OBJECT (section),
                     "IDE_EDITOR_SIDEBAR_MENU_ID",
                     (gpointer) g_intern_string (menu_id));

  g_object_set_data (G_OBJECT (section),
                     "IDE_EDITOR_SIDEBAR_MENU_ICON_NAME",
                     (gpointer) g_intern_string (menu_icon_name));

  position = find_position (self, priority);

  gtk_container_add_with_properties (GTK_CONTAINER (self->stack), section,
                                     "icon-name", icon_name,
                                     "name", id,
                                     "position", position,
                                     "title", title,
                                     NULL);

  gtk_container_foreach (GTK_CONTAINER (self->stack_switcher),
                         fixup_stack_switcher_button,
                         NULL);

  ide_editor_sidebar_update_title (self);

  /* Whenever we add a position 0, select it. We don't
   * have an otherwise good hueristic to ensure that our
   * first panel is selected at startup.
   */
  if (position == 0)
    gtk_stack_set_visible_child (self->stack, section);
}

/**
 * ide_editor_sidebar_get_section_id:
 * @self: a #IdeEditorSidebar
 *
 * Gets the id of the current section.
 *
 * Returns: (nullable): The id of the current section if it registered one.
 *
 * Since: 3.26
 */
const gchar *
ide_editor_sidebar_get_section_id (IdeEditorSidebar *self)
{
  g_return_val_if_fail (IDE_IS_EDITOR_SIDEBAR (self), NULL);

  return gtk_stack_get_visible_child_name (self->stack);
}

/**
 * ide_editor_sidebar_set_section_id:
 * @self: a #IdeEditorSidebar
 * @section_id: a section id to switch to
 *
 * Changes the current section to @section_id.
 *
 * Since: 3.26
 */
void
ide_editor_sidebar_set_section_id (IdeEditorSidebar *self,
                                   const gchar      *section_id)
{
  g_return_if_fail (IDE_IS_EDITOR_SIDEBAR (self));
  g_return_if_fail (section_id != NULL);

  gtk_stack_set_visible_child_name (self->stack, section_id);
}

static void
ide_editor_sidebar_close_view (GtkButton     *button,
                               IdeLayoutView *view)
{
  GtkWidget *stack;

  g_assert (GTK_IS_BUTTON (button));
  g_assert (IDE_IS_LAYOUT_VIEW (view));

  stack = gtk_widget_get_ancestor (GTK_WIDGET (view), IDE_TYPE_LAYOUT_STACK);

  if (stack != NULL)
    _ide_layout_stack_request_close (IDE_LAYOUT_STACK (stack), view);
}

static GtkWidget *
create_open_page_row (gpointer item,
                      gpointer user_data)
{
  IdeLayoutView *view = item;
  GtkListBoxRow *row;
  GtkButton *button;
  GtkImage *image;
  GtkLabel *label;
  GtkBox *box;

  g_assert (IDE_IS_LAYOUT_VIEW (view));
  g_assert (IDE_IS_EDITOR_SIDEBAR (user_data));

  row = g_object_new (GTK_TYPE_LIST_BOX_ROW,
                      "visible", TRUE,
                      NULL);
  g_object_set_data (G_OBJECT (row), "IDE_LAYOUT_VIEW", view);

  box = g_object_new (GTK_TYPE_BOX,
                      "orientation", GTK_ORIENTATION_HORIZONTAL,
                      "visible", TRUE,
                      NULL);
  gtk_container_add (GTK_CONTAINER (row), GTK_WIDGET (box));

  image = g_object_new (GTK_TYPE_IMAGE,
                        "icon-size", GTK_ICON_SIZE_MENU,
                        "hexpand", FALSE,
                        "visible", TRUE,
                        NULL);
  g_object_bind_property (view, "icon-name", image, "icon-name",
                          G_BINDING_SYNC_CREATE);
  gtk_container_add (GTK_CONTAINER (box), GTK_WIDGET (image));

  label = g_object_new (DZL_TYPE_BOLDING_LABEL,
                        "ellipsize", PANGO_ELLIPSIZE_START,
                        "visible", TRUE,
                        "hexpand", TRUE,
                        "xalign", 0.0f,
                        NULL);
  g_object_bind_property (view, "title", label, "label", G_BINDING_SYNC_CREATE);
  g_object_bind_property (view, "modified", label, "bold", G_BINDING_SYNC_CREATE);
  gtk_container_add (GTK_CONTAINER (box), GTK_WIDGET (label));

  button = g_object_new (GTK_TYPE_BUTTON,
                        "visible", TRUE,
                        "hexpand", FALSE,
                        NULL);
  g_signal_connect_object (button,
                           "clicked",
                           G_CALLBACK (ide_editor_sidebar_close_view),
                           view, 0);
  dzl_gtk_widget_add_style_class (GTK_WIDGET (button), "flat");
  gtk_container_add (GTK_CONTAINER (box), GTK_WIDGET (button));

  image = g_object_new (GTK_TYPE_IMAGE,
                        "icon-size", GTK_ICON_SIZE_MENU,
                        "icon-name", "window-close-symbolic",
                        "visible", TRUE,
                        NULL);
  gtk_container_add (GTK_CONTAINER (button), GTK_WIDGET (image));

  return GTK_WIDGET (row);
}

static void
ide_editor_sidebar_open_pages_items_changed (IdeEditorSidebar *self,
                                             guint             position,
                                             guint             added,
                                             guint             removed,
                                             GListModel       *model)
{
  g_assert (IDE_IS_EDITOR_SIDEBAR (self));
  g_assert (G_IS_LIST_MODEL (model));

  /*
   * Sets the visibility of our page list widgets only when the listmodel has
   * views within it. We try to be careful about being safe when the widget is
   * in destruction and an items-changed signal arrives.
   */

  if (self->open_pages_section != NULL)
    {
      gboolean has_items = g_list_model_get_n_items (model) > 0;
      gtk_widget_set_visible (GTK_WIDGET (self->open_pages_section), has_items);
    }
}

/**
 * _ide_editor_sidebar_set_open_pages:
 * @self: a #IdeEditorSidebar
 * @open_pages: A #GListModel describing the open pages
 *
 * This private function is used to set the GListModel to use for the list
 * of open pages in the sidebar. It should contain a list of IdeLayoutView
 * which we will use to keep the rows up to date.
 */
void
_ide_editor_sidebar_set_open_pages (IdeEditorSidebar *self,
                                    GListModel       *open_pages)
{
  g_return_if_fail (IDE_IS_EDITOR_SIDEBAR (self));
  g_return_if_fail (!open_pages || G_IS_LIST_MODEL (open_pages));
  g_return_if_fail (!open_pages ||
                    g_list_model_get_item_type (open_pages) == IDE_TYPE_LAYOUT_VIEW);

  if (open_pages != NULL)
    g_signal_connect_object (open_pages,
                             "items-changed",
                             G_CALLBACK (ide_editor_sidebar_open_pages_items_changed),
                             self,
                             G_CONNECT_SWAPPED);

  gtk_list_box_bind_model (self->open_pages_list_box,
                           open_pages,
                           create_open_page_row,
                           self, NULL);
}
