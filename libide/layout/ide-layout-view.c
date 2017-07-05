/* ide-layout-view.c
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

#define G_LOG_DOMAIN "ide-layout-view"

#include "ide-layout-view.h"

typedef struct
{
  const gchar *menu_id;
  const gchar *icon_name;
  gchar       *title;
  guint        failed : 1;
  guint        modified : 1;
  guint        can_split : 1;
} IdeLayoutViewPrivate;

enum {
  PROP_0,
  PROP_CAN_SPLIT,
  PROP_ICON_NAME,
  PROP_MENU_ID,
  PROP_MODIFIED,
  PROP_FAILED,
  PROP_TITLE,
  N_PROPS
};

enum {
  CREATE_SPLIT_VIEW,
  N_SIGNALS
};

G_DEFINE_TYPE_WITH_PRIVATE (IdeLayoutView, ide_layout_view, GTK_TYPE_BOX)

static GParamSpec *properties [N_PROPS];
static guint signals [N_SIGNALS];

static void
ide_layout_view_real_agree_to_close_async (IdeLayoutView       *self,
                                           GCancellable        *cancellable,
                                           GAsyncReadyCallback  callback,
                                           gpointer             user_data)
{
  g_autoptr(GTask) task = NULL;

  g_assert (IDE_IS_LAYOUT_VIEW (self));
  g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_priority (task, G_PRIORITY_LOW);
  g_task_set_source_tag (task, ide_layout_view_agree_to_close_async);
  g_task_return_boolean (task, TRUE);
}

static gboolean
ide_layout_view_real_agree_to_close_finish (IdeLayoutView  *self,
                                            GAsyncResult   *result,
                                            GError        **error)
{
  g_assert (IDE_IS_LAYOUT_VIEW (self));
  g_assert (G_IS_TASK (result));

  return g_task_propagate_boolean (G_TASK (result), error);
}

static void
find_focus_child (GtkWidget *widget,
                  gboolean  *handled)
{
  if (!*handled)
    *handled = gtk_widget_child_focus (widget, GTK_DIR_TAB_FORWARD);
}

static void
ide_layout_view_grab_focus (GtkWidget *widget)
{
  gboolean handled = FALSE;

  g_assert (IDE_IS_LAYOUT_VIEW (widget));

  /*
   * This default grab_focus override just looks for the first child (generally
   * something like a scrolled window) and tries to move forward on focusing
   * the child widget. In most cases, this should work without intervention
   * from the child subclass.
   */

  gtk_container_foreach (GTK_CONTAINER (widget), (GtkCallback) find_focus_child, &handled);
}

static void
ide_layout_view_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  IdeLayoutView *self = IDE_LAYOUT_VIEW (object);

  switch (prop_id)
    {
    case PROP_CAN_SPLIT:
      g_value_set_boolean (value, ide_layout_view_get_can_split (self));
      break;

    case PROP_FAILED:
      g_value_set_boolean (value, ide_layout_view_get_failed (self));
      break;

    case PROP_ICON_NAME:
      g_value_set_static_string (value, ide_layout_view_get_icon_name (self));
      break;

    case PROP_MENU_ID:
      g_value_set_static_string (value, ide_layout_view_get_menu_id (self));
      break;

    case PROP_MODIFIED:
      g_value_set_boolean (value, ide_layout_view_get_modified (self));
      break;

    case PROP_TITLE:
      g_value_set_string (value, ide_layout_view_get_title (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ide_layout_view_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  IdeLayoutView *self = IDE_LAYOUT_VIEW (object);

  switch (prop_id)
    {
    case PROP_CAN_SPLIT:
      ide_layout_view_set_can_split (self, g_value_get_boolean (value));
      break;

    case PROP_FAILED:
      ide_layout_view_set_failed (self, g_value_get_boolean (value));
      break;

    case PROP_ICON_NAME:
      ide_layout_view_set_icon_name (self, g_value_get_string (value));
      break;

    case PROP_MENU_ID:
      ide_layout_view_set_menu_id (self, g_value_get_string (value));
      break;

    case PROP_MODIFIED:
      ide_layout_view_set_modified (self, g_value_get_boolean (value));
      break;

    case PROP_TITLE:
      ide_layout_view_set_title (self, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ide_layout_view_class_init (IdeLayoutViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->get_property = ide_layout_view_get_property;
  object_class->set_property = ide_layout_view_set_property;

  widget_class->grab_focus = ide_layout_view_grab_focus;

  klass->agree_to_close_async = ide_layout_view_real_agree_to_close_async;
  klass->agree_to_close_finish = ide_layout_view_real_agree_to_close_finish;

  properties [PROP_CAN_SPLIT] =
    g_param_spec_boolean ("can-split",
                          "Can Split",
                          "If the view can be split into a second view",
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_FAILED] =
    g_param_spec_boolean ("failed",
                          "Failed",
                          "If the view has failed or crashed",
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_ICON_NAME] =
    g_param_spec_string ("icon-name",
                         "Icon Name",
                         "The icon-name describing the view content",
                         "text-x-generic-symbolic",
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_MENU_ID] =
    g_param_spec_string ("menu-id",
                         "Menu ID",
                         "The identifier of the GMenu to use in the document popover",
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_MODIFIED] =
    g_param_spec_boolean ("modified",
                          "Modified",
                          "If the view has been modified from the saved content",
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_TITLE] =
    g_param_spec_string ("title",
                         "Title",
                         "The title of the document or view",
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  /**
   * IdeLayoutView::create-split-view:
   *
   * This signal is emitted when the view is requested to make a split
   * version of itself. This happens when the user requests that a second
   * version of the file to be displayed, often side-by-side.
   *
   * This signal will only be emitted when #IdeLayoutView:can-split is
   * set to %TRUE. The default is %FALSE.
   *
   * Returns: (transfer full): A newly created #IdeLayoutView
   */
  signals [CREATE_SPLIT_VIEW] =
    g_signal_new (g_intern_static_string ("create-split-view"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (IdeLayoutViewClass, create_split_view),
                  g_signal_accumulator_first_wins, NULL,
                  NULL, IDE_TYPE_LAYOUT_VIEW, 0);
}

static void
ide_layout_view_init (IdeLayoutView *self)
{
  IdeLayoutViewPrivate *priv = ide_layout_view_get_instance_private (self);

  gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_VERTICAL);

  priv->icon_name = g_intern_string ("text-x-generic-symbolic");
}

GtkWidget *
ide_layout_view_new (void)
{
  return g_object_new (IDE_TYPE_LAYOUT_VIEW, NULL);
}

const gchar *
ide_layout_view_get_title (IdeLayoutView *self)
{
  IdeLayoutViewPrivate *priv = ide_layout_view_get_instance_private (self);

  g_return_val_if_fail (IDE_IS_LAYOUT_VIEW (self), NULL);

  return priv->title;
}

void
ide_layout_view_set_title (IdeLayoutView *self,
                           const gchar   *title)
{
  IdeLayoutViewPrivate *priv = ide_layout_view_get_instance_private (self);

  g_return_if_fail (IDE_IS_LAYOUT_VIEW (self));

  if (g_strcmp0 (title, priv->title) != 0)
    {
      g_free (priv->title);
      priv->title = g_strdup (title);
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_TITLE]);
    }
}

const gchar *
ide_layout_view_get_menu_id (IdeLayoutView *self)
{
  IdeLayoutViewPrivate *priv = ide_layout_view_get_instance_private (self);

  g_return_val_if_fail (IDE_IS_LAYOUT_VIEW (self), NULL);

  return priv->menu_id;
}

void
ide_layout_view_set_menu_id (IdeLayoutView *self,
                             const gchar   *menu_id)
{
  IdeLayoutViewPrivate *priv = ide_layout_view_get_instance_private (self);

  g_return_if_fail (IDE_IS_LAYOUT_VIEW (self));

  menu_id = g_intern_string (menu_id);

  if (menu_id != priv->menu_id)
    {
      priv->menu_id = menu_id;
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_MENU_ID]);
    }
}

void
ide_layout_view_agree_to_close_async (IdeLayoutView       *self,
                                      GCancellable        *cancellable,
                                      GAsyncReadyCallback  callback,
                                      gpointer             user_data)
{
  g_return_if_fail (IDE_IS_LAYOUT_VIEW (self));
  g_return_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable));

  IDE_LAYOUT_VIEW_GET_CLASS (self)->agree_to_close_async (self, cancellable, callback, user_data);
}

gboolean
ide_layout_view_agree_to_close_finish (IdeLayoutView  *self,
                                       GAsyncResult   *result,
                                       GError        **error)
{
  g_return_val_if_fail (IDE_IS_LAYOUT_VIEW (self), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (result), FALSE);

  return IDE_LAYOUT_VIEW_GET_CLASS (self)->agree_to_close_finish (self, result, error);
}

gboolean
ide_layout_view_get_failed (IdeLayoutView *self)
{
  IdeLayoutViewPrivate *priv = ide_layout_view_get_instance_private (self);

  g_return_val_if_fail (IDE_IS_LAYOUT_VIEW (self), FALSE);

  return priv->failed;
}

void
ide_layout_view_set_failed (IdeLayoutView *self,
                            gboolean       failed)
{
  IdeLayoutViewPrivate *priv = ide_layout_view_get_instance_private (self);

  g_return_if_fail (IDE_IS_LAYOUT_VIEW (self));

  failed = !!failed;

  if (failed != priv->failed)
    {
      priv->failed = failed;
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_FAILED]);
    }
}

gboolean
ide_layout_view_get_modified (IdeLayoutView *self)
{
  IdeLayoutViewPrivate *priv = ide_layout_view_get_instance_private (self);

  g_return_val_if_fail (IDE_IS_LAYOUT_VIEW (self), FALSE);

  return priv->modified;
}

void
ide_layout_view_set_modified (IdeLayoutView *self,
                              gboolean       modified)
{
  IdeLayoutViewPrivate *priv = ide_layout_view_get_instance_private (self);

  g_return_if_fail (IDE_IS_LAYOUT_VIEW (self));

  modified = !!modified;

  if (priv->modified != modified)
    {
      priv->modified = modified;
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_MODIFIED]);
    }
}

const gchar *
ide_layout_view_get_icon_name (IdeLayoutView *self)
{
  IdeLayoutViewPrivate *priv = ide_layout_view_get_instance_private (self);

  g_return_val_if_fail (IDE_IS_LAYOUT_VIEW (self), NULL);

  return priv->icon_name;
}

void
ide_layout_view_set_icon_name (IdeLayoutView *self,
                               const gchar   *icon_name)
{
  IdeLayoutViewPrivate *priv = ide_layout_view_get_instance_private (self);

  g_return_if_fail (IDE_IS_LAYOUT_VIEW (self));

  icon_name = g_intern_string (icon_name);

  if (icon_name != priv->icon_name)
    {
      priv->icon_name = icon_name;
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_ICON_NAME]);
    }
}

gboolean
ide_layout_view_get_can_split (IdeLayoutView *self)
{
  IdeLayoutViewPrivate *priv = ide_layout_view_get_instance_private (self);

  g_return_val_if_fail (IDE_IS_LAYOUT_VIEW (self), FALSE);

  return priv->can_split;
}

void
ide_layout_view_set_can_split (IdeLayoutView *self,
                               gboolean       can_split)
{
  IdeLayoutViewPrivate *priv = ide_layout_view_get_instance_private (self);

  g_return_if_fail (IDE_IS_LAYOUT_VIEW (self));

  can_split = !!can_split;

  if (priv->can_split != can_split)
    {
      priv->can_split = can_split;
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_CAN_SPLIT]);
    }
}

/**
 * ide_layout_view_create_split_view:
 * @self: an #IdeLayoutView
 *
 * This function requests that the #IdeLayoutView create a split version
 * of itself so that the user may view the document in multiple views.
 *
 * The view should be added to an #IdeLayoutStack where appropriate.
 *
 * Returns: (nullable) (transfer full): A newly created #IdeLayoutView or %NULL.
 *
 * Since: 3.26
 */
IdeLayoutView *
ide_layout_view_create_split_view (IdeLayoutView *self)
{
  IdeLayoutViewPrivate *priv = ide_layout_view_get_instance_private (self);
  IdeLayoutView *ret = NULL;

  g_return_val_if_fail (IDE_IS_LAYOUT_VIEW (self), NULL);

  if (priv->can_split)
    {
      g_signal_emit (self, signals [CREATE_SPLIT_VIEW], 0, &ret);
      g_return_val_if_fail (!ret || IDE_IS_LAYOUT_VIEW (ret), NULL);
    }

  return ret;
}
