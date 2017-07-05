/* ide-layout-stack-header.c
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

#define G_LOG_DOMAIN "ide-layout-stack-header"

#include <glib/gi18n.h>

#include "ide-layout-private.h"
#include "ide-layout-stack-header.h"

struct _IdeLayoutStackHeader
{
  DzlPriorityBox  parent_instance;

  GtkButton      *close_button;
  GtkMenuButton  *document_button;
  GtkPopover     *document_popover;
  GtkMenuButton  *title_button;
  GtkPopover     *title_popover;
  GtkListBox     *title_list_box;
  DzlPriorityBox *title_box;
  GtkLabel       *title_label;
  GtkLabel       *title_modified;
  GtkBox         *title_views_box;

  DzlJoinedMenu  *menu;
};

enum {
  PROP_0,
  PROP_MODIFIED,
  PROP_SHOW_CLOSE_BUTTON,
  PROP_TITLE,
  N_PROPS
};

G_DEFINE_TYPE (IdeLayoutStackHeader, ide_layout_stack_header, DZL_TYPE_PRIORITY_BOX)

static GParamSpec *properties [N_PROPS];

void
_ide_layout_stack_header_hide (IdeLayoutStackHeader *self)
{
  g_return_if_fail (IDE_IS_LAYOUT_STACK_HEADER (self));

  /* This is like _ide_layout_stack_header_popdown() but we hide the
   * popovers immediately without performing the popdown animation.
   */

  gtk_widget_hide (GTK_WIDGET (self->document_popover));
  gtk_widget_hide (GTK_WIDGET (self->title_popover));
}

void
_ide_layout_stack_header_popdown (IdeLayoutStackHeader *self)
{
  g_return_if_fail (IDE_IS_LAYOUT_STACK_HEADER (self));

  gtk_popover_popdown (self->document_popover);
  gtk_popover_popdown (self->title_popover);
}

void
_ide_layout_stack_header_update (IdeLayoutStackHeader *self,
                                 IdeLayoutView        *view)
{
  const gchar *action = "layoutstack.close-view";

  g_assert (IDE_IS_LAYOUT_STACK_HEADER (self));
  g_assert (!view || IDE_IS_LAYOUT_VIEW (view));

  /*
   * Update our menus for the document to include the menu type needed for the
   * newly focused view. Make sure we keep the Frame section at the end which
   * is always the last section in the joined menus.
   */

  while (dzl_joined_menu_get_n_joined (self->menu) > 1)
    dzl_joined_menu_remove_index (self->menu, 0);

  if (view != NULL)
    {
      const gchar *menu_id = ide_layout_view_get_menu_id (view);

      if (menu_id != NULL)
        {
          GMenu *menu = dzl_application_get_menu_by_id (DZL_APPLICATION_DEFAULT, menu_id);

          dzl_joined_menu_prepend_menu (self->menu, G_MENU_MODEL (menu));
        }
    }

  /*
   * Hide the document selectors if there are no views to select (which is
   * indicated by us having a NULL view here.
   */
  gtk_widget_set_visible (GTK_WIDGET (self->title_views_box), view != NULL);

  /*
   * The close button acts differently depending on the grid stage.
   *
   *  - Last column, single stack => do nothing (action will be disabled)
   *  - No more views and more than one stack in column (close just the stack)
   *  - No more views and single stack in column and more than one column (close the column)
   */

  if (view == NULL)
    {
      GtkWidget *stack;
      GtkWidget *column;

      action = "layoutgridcolumn.close";
      stack = gtk_widget_get_ancestor (GTK_WIDGET (self), IDE_TYPE_LAYOUT_STACK);
      column = gtk_widget_get_ancestor (GTK_WIDGET (stack), IDE_TYPE_LAYOUT_GRID_COLUMN);

      if (stack != NULL && column != NULL)
        {
          if (dzl_multi_paned_get_n_children (DZL_MULTI_PANED (column)) > 1)
            action = "layoutstack.close-stack";
        }
    }

  gtk_actionable_set_action_name (GTK_ACTIONABLE (self->close_button), action);

  /*
   * Hide any popovers that we know about. If we got here from closing
   * documents, we should hide the popover after the last document is closed
   * (inidicated by NULL view).
   */
  if (view == NULL)
    _ide_layout_stack_header_popdown (self);
}

static void
close_view_cb (GtkButton            *button,
               IdeLayoutStackHeader *self)
{
  GtkWidget *stack;
  GtkWidget *row;
  GtkWidget *view;

  g_assert (GTK_IS_BUTTON (button));
  g_assert (IDE_IS_LAYOUT_STACK_HEADER (self));

  row = gtk_widget_get_ancestor (GTK_WIDGET (button), GTK_TYPE_LIST_BOX_ROW);
  if (row == NULL)
    return;

  view = g_object_get_data (G_OBJECT (row), "IDE_LAYOUT_VIEW");
  if (view == NULL)
    return;

  stack = gtk_widget_get_ancestor (GTK_WIDGET (self), IDE_TYPE_LAYOUT_STACK);
  if (stack == NULL)
    return;

  _ide_layout_stack_request_close (IDE_LAYOUT_STACK (stack), IDE_LAYOUT_VIEW (view));
}

static GtkWidget *
create_document_row (gpointer item,
                     gpointer user_data)
{
  IdeLayoutStackHeader *self = user_data;
  GtkListBoxRow *row;
  GtkButton *close_button;
  GtkLabel *label;
  GtkImage *image;
  GtkBox *box;

  g_assert (IDE_IS_LAYOUT_VIEW (item));
  g_assert (IDE_IS_LAYOUT_STACK_HEADER (self));

  row = g_object_new (GTK_TYPE_LIST_BOX_ROW,
                      "visible", TRUE,
                      NULL);
  box = g_object_new (GTK_TYPE_BOX,
                      "spacing", 6,
                      "visible", TRUE,
                      NULL);
  image = g_object_new (GTK_TYPE_IMAGE,
                        "icon-size", GTK_ICON_SIZE_MENU,
                        "visible", TRUE,
                        NULL);
  label = g_object_new (DZL_TYPE_BOLDING_LABEL,
                        "hexpand", TRUE,
                        "xalign", 0.0f,
                        "visible", TRUE,
                        NULL);
  close_button = g_object_new (GTK_TYPE_BUTTON,
                               "child", g_object_new (GTK_TYPE_IMAGE,
                                                      "icon-name", "window-close-symbolic",
                                                      "visible", TRUE,
                                                      NULL),
                               "visible", TRUE,
                               NULL);
  g_signal_connect (close_button,
                    "clicked",
                    G_CALLBACK (close_view_cb),
                    self);
  dzl_gtk_widget_add_style_class (GTK_WIDGET (close_button), "image-button");

  g_object_bind_property (item, "icon-name", image, "icon-name", G_BINDING_SYNC_CREATE);
  g_object_bind_property (item, "modified", label, "bold", G_BINDING_SYNC_CREATE);
  g_object_bind_property (item, "title", label, "label", G_BINDING_SYNC_CREATE);
  g_object_set_data (G_OBJECT (row), "IDE_LAYOUT_VIEW", item);

  gtk_container_add (GTK_CONTAINER (row), GTK_WIDGET (box));
  gtk_container_add (GTK_CONTAINER (box), GTK_WIDGET (image));
  gtk_container_add (GTK_CONTAINER (box), GTK_WIDGET (label));
  gtk_container_add (GTK_CONTAINER (box), GTK_WIDGET (close_button));

  return GTK_WIDGET (row);
}

void
_ide_layout_stack_header_set_views (IdeLayoutStackHeader *self,
                                    GListModel           *model)
{
  g_assert (IDE_IS_LAYOUT_STACK_HEADER (self));
  g_assert (!model || G_IS_LIST_MODEL (model));

  gtk_list_box_bind_model (self->title_list_box,
                           model,
                           create_document_row,
                           self, NULL);
}

static void
ide_layout_stack_header_view_row_activated (GtkListBox           *list_box,
                                            GtkListBoxRow        *row,
                                            IdeLayoutStackHeader *self)
{
  GtkWidget *stack;
  GtkWidget *view;

  g_assert (GTK_IS_LIST_BOX (list_box));
  g_assert (GTK_IS_LIST_BOX_ROW (row));
  g_assert (IDE_IS_LAYOUT_STACK_HEADER (self));

  stack = gtk_widget_get_ancestor (GTK_WIDGET (self), IDE_TYPE_LAYOUT_STACK);
  view = g_object_get_data (G_OBJECT (row), "IDE_LAYOUT_VIEW");

  if (stack != NULL && view != NULL)
    ide_layout_stack_set_visible_child (IDE_LAYOUT_STACK (stack),
                                        IDE_LAYOUT_VIEW (view));
}

static void
ide_layout_stack_header_destroy (GtkWidget *widget)
{
  IdeLayoutStackHeader *self = (IdeLayoutStackHeader *)widget;

  g_assert (IDE_IS_LAYOUT_STACK_HEADER (self));

  g_clear_object (&self->menu);

  GTK_WIDGET_CLASS (ide_layout_stack_header_parent_class)->destroy (widget);
}

static void
ide_layout_stack_header_get_property (GObject    *object,
                                      guint       prop_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  IdeLayoutStackHeader *self = IDE_LAYOUT_STACK_HEADER (object);

  switch (prop_id)
    {
    case PROP_MODIFIED:
      g_value_set_boolean (value, gtk_widget_get_visible (GTK_WIDGET (self->title_modified)));
      break;

    case PROP_SHOW_CLOSE_BUTTON:
      g_value_set_boolean (value, gtk_widget_get_visible (GTK_WIDGET (self->close_button)));
      break;

    case PROP_TITLE:
      g_value_set_string (value, gtk_label_get_label (GTK_LABEL (self->title_label)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ide_layout_stack_header_set_property (GObject      *object,
                                      guint         prop_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  IdeLayoutStackHeader *self = IDE_LAYOUT_STACK_HEADER (object);

  switch (prop_id)
    {
    case PROP_MODIFIED:
      gtk_widget_set_visible (GTK_WIDGET (self->title_modified), g_value_get_boolean (value));
      break;

    case PROP_SHOW_CLOSE_BUTTON:
      gtk_widget_set_visible (GTK_WIDGET (self->close_button), g_value_get_boolean (value));
      break;

    case PROP_TITLE:
      ide_layout_stack_header_set_title (self, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ide_layout_stack_header_class_init (IdeLayoutStackHeaderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = ide_layout_stack_header_get_property;
  object_class->set_property = ide_layout_stack_header_set_property;

  widget_class->destroy = ide_layout_stack_header_destroy;

  properties [PROP_SHOW_CLOSE_BUTTON] =
    g_param_spec_boolean ("show-close-button",
                          "Show Close Button",
                          "If the close button should be displayed",
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_MODIFIED] =
    g_param_spec_boolean ("modified",
                          "Modified",
                          "If the current document is modified",
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_TITLE] =
    g_param_spec_string ("title",
                         "Title",
                         "The title of the current document or view",
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_css_name (widget_class, "idelayoutstackheader");
  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/builder/ui/ide-layout-stack-header.ui");
  gtk_widget_class_bind_template_child (widget_class, IdeLayoutStackHeader, close_button);
  gtk_widget_class_bind_template_child (widget_class, IdeLayoutStackHeader, document_popover);
  gtk_widget_class_bind_template_child (widget_class, IdeLayoutStackHeader, document_button);
  gtk_widget_class_bind_template_child (widget_class, IdeLayoutStackHeader, title_box);
  gtk_widget_class_bind_template_child (widget_class, IdeLayoutStackHeader, title_button);
  gtk_widget_class_bind_template_child (widget_class, IdeLayoutStackHeader, title_label);
  gtk_widget_class_bind_template_child (widget_class, IdeLayoutStackHeader, title_list_box);
  gtk_widget_class_bind_template_child (widget_class, IdeLayoutStackHeader, title_modified);
  gtk_widget_class_bind_template_child (widget_class, IdeLayoutStackHeader, title_popover);
  gtk_widget_class_bind_template_child (widget_class, IdeLayoutStackHeader, title_views_box);
}

static void
ide_layout_stack_header_init (IdeLayoutStackHeader *self)
{
  GMenu *frame_section;

  gtk_widget_init_template (GTK_WIDGET (self));

  /*
   * Create our menu for the document controls popover. It has two sections.
   * The top section is based on the document and is updated whenever the
   * visible child is changed. The bottom, are the frame controls are and
   * static, but setup by us here.
   */

  self->menu = dzl_joined_menu_new ();
  gtk_popover_bind_model (self->document_popover, G_MENU_MODEL (self->menu), NULL);
  frame_section = dzl_application_get_menu_by_id (DZL_APPLICATION_DEFAULT, "ide-layout-stack-frame-menu");
  dzl_joined_menu_append_menu (self->menu, G_MENU_MODEL (frame_section));

  /*
   * When a row is selected, we want to change the current view and
   * hide the popover.
   */

  g_signal_connect (self->title_list_box,
                    "row-activated",
                    G_CALLBACK (ide_layout_stack_header_view_row_activated),
                    self);
}

GtkWidget *
ide_layout_stack_header_new (void)
{
  return g_object_new (IDE_TYPE_LAYOUT_STACK_HEADER, NULL);
}

/**
 * ide_layout_stack_header_add_custom_title:
 * @self: a #IdeLayoutStackHeader
 * @widget: A #GtkWidget
 * @priority: the sort priority
 *
 * This will add @widget to the title area with @priority determining the
 * sort order of the child.
 *
 * All "title" widgets in the #IdeLayoutStackHeader are expanded to the
 * same size. So if you don't need that, you should just use the normal
 * gtk_container_add_with_properties() API to specify your widget with
 * a given priority.
 */
void
ide_layout_stack_header_add_custom_title (IdeLayoutStackHeader *self,
                                          GtkWidget            *widget,
                                          gint                  priority)
{
  g_return_if_fail (IDE_IS_LAYOUT_STACK_HEADER (self));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  gtk_container_add_with_properties (GTK_CONTAINER (self->title_box), widget,
                                     "priority", priority,
                                     NULL);
}

void
ide_layout_stack_header_set_title (IdeLayoutStackHeader *self,
                                   const gchar          *title)
{
  g_return_if_fail (IDE_IS_LAYOUT_STACK_HEADER (self));

  gtk_label_set_label (GTK_LABEL (self->title_label), title);
  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_TITLE]);
}
