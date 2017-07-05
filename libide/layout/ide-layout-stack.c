/* ide-layout-stack.c
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

#define G_LOG_DOMAIN "ide-layout-stack"

#include <dazzle.h>
#include <glib/gi18n.h>

#include "ide-layout-stack.h"
#include "ide-layout-stack-header.h"
#include "ide-layout-private.h"
#include "ide-shortcut-label.h"

#define TRANSITION_DURATION 300

/**
 * SECTION:ide-layout-stack
 * @title: IdeLayoutStack
 * @short_description: A stack of #IdeLayoutView
 *
 * This widget is used to represent a stack of #IdeLayoutView widgets.  it
 * includes an #IdeLayoutStackHeader at the top, and then a stack of views
 * below.
 *
 * If there are no #IdeLayoutView visibile, then an empty state widget is
 * displayed with some common information for the user.
 *
 * To simplify integration with other systems, #IdeLayoutStack implements
 * the #GListModel interface for each of the #IdeLayoutView.
 */

typedef struct
{
  DzlBindingGroup      *bindings;
  DzlSignalGroup       *signals;
  GPtrArray            *views;

  DzlBox               *empty_state;
  DzlEmptyState        *failed_state;
  IdeLayoutStackHeader *header;
  GtkStack             *stack;
  GtkStack             *top_stack;
} IdeLayoutStackPrivate;

typedef struct
{
  IdeLayoutStack *source;
  IdeLayoutStack *dest;
  IdeLayoutView  *view;
  DzlBoxTheatric *theatric;
} AnimationState;

enum {
  PROP_0,
  PROP_HAS_VIEW,
  PROP_VISIBLE_CHILD,
  N_PROPS
};

enum {
  CHNAGE_CURRENT_PAGE,
  N_SIGNALS
};

static void list_model_iface_init (GListModelInterface *iface);

G_DEFINE_TYPE_WITH_CODE (IdeLayoutStack, ide_layout_stack, GTK_TYPE_BOX,
                         G_ADD_PRIVATE (IdeLayoutStack)
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, list_model_iface_init))

static GParamSpec *properties [N_PROPS];
static guint signals [N_SIGNALS];

static void
ide_layout_stack_view_failed (IdeLayoutStack *self,
                              GParamSpec     *pspec,
                              IdeLayoutView  *view)
{
  IdeLayoutStackPrivate *priv = ide_layout_stack_get_instance_private (self);

  g_assert (IDE_IS_LAYOUT_STACK (self));
  g_assert (IDE_IS_LAYOUT_VIEW (view));

  if (ide_layout_view_get_failed (view))
    gtk_stack_set_visible_child (priv->top_stack, GTK_WIDGET (priv->failed_state));
  else
    gtk_stack_set_visible_child (priv->top_stack, GTK_WIDGET (priv->stack));
}

static void
ide_layout_stack_bindings_notify_source (IdeLayoutStack  *self,
                                         GParamSpec      *pspec,
                                         DzlBindingGroup *bindings)
{
  IdeLayoutStackPrivate *priv = ide_layout_stack_get_instance_private (self);
  GObject *source;

  g_assert (DZL_IS_BINDING_GROUP (bindings));
  g_assert (pspec != NULL);
  g_assert (IDE_IS_LAYOUT_STACK (self));

  source = dzl_binding_group_get_source (bindings);

  if (source == NULL)
    {
      _ide_layout_stack_header_set_title (priv->header, _("No Open Pages"));
      _ide_layout_stack_header_set_modified (priv->header, FALSE);
    }
}

static void
ide_layout_stack_notify_visible_child (IdeLayoutStack *self,
                                       GParamSpec     *pspec,
                                       GtkStack       *stack)
{
  IdeLayoutStackPrivate *priv = ide_layout_stack_get_instance_private (self);
  GtkWidget *visible_child;

  g_assert (IDE_IS_LAYOUT_STACK (self));
  g_assert (GTK_IS_STACK (stack));

  visible_child = gtk_stack_get_visible_child (priv->stack);

  /*
   * Mux/Proxy actions to our level so that they also be activated
   * from the header bar without any weirdness by the View.
   */
  dzl_gtk_widget_mux_action_groups (GTK_WIDGET (self), visible_child,
                                    "IDE_LAYOUT_STACK_MUXED_ACTION");

  /* Update our bindings targets */
  dzl_binding_group_set_source (priv->bindings, visible_child);
  dzl_signal_group_set_target (priv->signals, visible_child);

  /* Show either the empty state, failed state, or actual view */
  if (visible_child != NULL &&
      ide_layout_view_get_failed (IDE_LAYOUT_VIEW (visible_child)))
    gtk_stack_set_visible_child (priv->top_stack, GTK_WIDGET (priv->failed_state));
  else if (visible_child != NULL)
    gtk_stack_set_visible_child (priv->top_stack, GTK_WIDGET (priv->stack));
  else
    gtk_stack_set_visible_child (priv->top_stack, GTK_WIDGET (priv->empty_state));

  /* Allow the header to update settings */
  _ide_layout_stack_header_update (priv->header, IDE_LAYOUT_VIEW (visible_child));

  /* Ensure action state is up to date */
  _ide_layout_stack_update_actions (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_VISIBLE_CHILD]);
  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_HAS_VIEW]);
}

static void
collect_widgets (GtkWidget *widget,
                 gpointer   user_data)
{
  g_ptr_array_add (user_data, widget);
}

static void
ide_layout_stack_change_current_page (IdeLayoutStack *self,
                                      gint            direction)
{
  IdeLayoutStackPrivate *priv = ide_layout_stack_get_instance_private (self);
  g_autoptr(GPtrArray) ar = NULL;
  GtkWidget *visible_child;
  gint position = 0;

  g_assert (IDE_IS_LAYOUT_STACK (self));

  visible_child = gtk_stack_get_visible_child (priv->stack);

  if (visible_child == NULL)
    return;

  gtk_container_child_get (GTK_CONTAINER (priv->stack), visible_child,
                           "position", &position,
                           NULL);

  ar = g_ptr_array_new ();
  gtk_container_foreach (GTK_CONTAINER (priv->stack), collect_widgets, ar);
  if (ar->len == 0)
    g_return_if_reached ();

  visible_child = g_ptr_array_index (ar, (position + direction) % ar->len);
  gtk_stack_set_visible_child (priv->stack, visible_child);
}

static void
ide_layout_stack_add (GtkContainer *container,
                      GtkWidget    *widget)
{
  IdeLayoutStack *self = (IdeLayoutStack *)container;
  IdeLayoutStackPrivate *priv = ide_layout_stack_get_instance_private (self);

  g_return_if_fail (IDE_IS_LAYOUT_STACK (self));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (IDE_IS_LAYOUT_VIEW (widget))
    gtk_container_add (GTK_CONTAINER (priv->stack), widget);
  else
    GTK_CONTAINER_CLASS (ide_layout_stack_parent_class)->add (container, widget);

  gtk_widget_queue_resize (GTK_WIDGET (self));
}

static void
ide_layout_stack_view_added (IdeLayoutStack *self,
                             IdeLayoutView  *view)
{
  IdeLayoutStackPrivate *priv = ide_layout_stack_get_instance_private (self);
  gint position;

  g_assert (IDE_IS_LAYOUT_STACK (self));
  g_assert (IDE_IS_LAYOUT_VIEW (view));

  /*
   * Make sure that the header has dismissed all of the popovers immediately.
   * We don't want them lingering while we do other UI work which might want to
   * grab focus, etc.
   */
  _ide_layout_stack_header_popdown (priv->header);

  /* Notify GListModel consumers of the new view and it's position within
   * our stack of view widgets.
   */
  gtk_container_child_get (GTK_CONTAINER (priv->stack), GTK_WIDGET (view),
                           "position", &position,
                           NULL);
  g_ptr_array_insert (priv->views, position, view);
  g_list_model_items_changed (G_LIST_MODEL (self), position, 0, 1);

  /*
   * Now ensure that the view is displayed and focus the widget so the
   * user can immediately start typing.
   */
  ide_layout_stack_set_visible_child (self, view);
  gtk_widget_grab_focus (GTK_WIDGET (view));
}

static void
ide_layout_stack_view_removed (IdeLayoutStack *self,
                               IdeLayoutView  *view)
{
  IdeLayoutStackPrivate *priv = ide_layout_stack_get_instance_private (self);

  g_assert (IDE_IS_LAYOUT_STACK (self));
  g_assert (IDE_IS_LAYOUT_VIEW (view));

  if (priv->views != NULL)
    {
      /* If this is the last view, hide the popdown now.  We use our hide
       * variant instead of popdown so that we don't have jittery animations.
       */
      if (priv->views->len == 1)
        _ide_layout_stack_header_hide (priv->header);

      for (guint i = 0; i < priv->views->len; i++)
        {
          if (view == (IdeLayoutView *)g_ptr_array_index (priv->views, i))
            {
              g_ptr_array_remove_index (priv->views, i);
              g_list_model_items_changed (G_LIST_MODEL (self), i, 1, 0);
            }
        }
    }
}

static void
ide_layout_stack_real_agree_to_close_async (IdeLayoutStack      *self,
                                            GCancellable        *cancellable,
                                            GAsyncReadyCallback  callback,
                                            gpointer             user_data)
{
  g_autoptr(GTask) task = NULL;

  g_assert (IDE_IS_LAYOUT_STACK (self));
  g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_source_tag (task, ide_layout_stack_real_agree_to_close_async);
  g_task_set_priority (task, G_PRIORITY_LOW);
  g_task_return_boolean (task, TRUE);
}

static gboolean
ide_layout_stack_real_agree_to_close_finish (IdeLayoutStack *self,
                                             GAsyncResult   *result,
                                             GError        **error)
{
  g_assert (IDE_IS_LAYOUT_STACK (self));
  g_assert (G_IS_TASK (result));

  return g_task_propagate_boolean (G_TASK (result), error);
}

static void
ide_layout_stack_destroy (GtkWidget *widget)
{
  IdeLayoutStack *self = (IdeLayoutStack *)widget;
  IdeLayoutStackPrivate *priv = ide_layout_stack_get_instance_private (self);

  g_assert (IDE_IS_LAYOUT_STACK (self));

  if (priv->bindings != NULL)
    {
      dzl_binding_group_set_source (priv->bindings, NULL);
      g_clear_object (&priv->bindings);
    }

  if (priv->signals != NULL)
    {
      dzl_signal_group_set_target (priv->signals, NULL);
      g_clear_object (&priv->signals);
    }

  g_clear_pointer (&priv->views, g_ptr_array_unref);

  GTK_WIDGET_CLASS (ide_layout_stack_parent_class)->destroy (widget);
}

static void
ide_layout_stack_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  IdeLayoutStack *self = IDE_LAYOUT_STACK (object);

  switch (prop_id)
    {
    case PROP_HAS_VIEW:
      g_value_set_boolean (value, ide_layout_stack_get_has_view (self));
      break;

    case PROP_VISIBLE_CHILD:
      g_value_set_object (value, ide_layout_stack_get_visible_child (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ide_layout_stack_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  IdeLayoutStack *self = IDE_LAYOUT_STACK (object);

  switch (prop_id)
    {
    case PROP_VISIBLE_CHILD:
      ide_layout_stack_set_visible_child (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ide_layout_stack_class_init (IdeLayoutStackClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  object_class->get_property = ide_layout_stack_get_property;
  object_class->set_property = ide_layout_stack_set_property;

  widget_class->destroy = ide_layout_stack_destroy;

  container_class->add = ide_layout_stack_add;

  klass->agree_to_close_async = ide_layout_stack_real_agree_to_close_async;
  klass->agree_to_close_finish = ide_layout_stack_real_agree_to_close_finish;

  properties [PROP_HAS_VIEW] =
    g_param_spec_boolean ("has-view", NULL, NULL,
                          FALSE,
                          (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  properties [PROP_VISIBLE_CHILD] =
    g_param_spec_object ("visible-child",
                         "Visible Child",
                         "The current view to be displayed",
                         IDE_TYPE_LAYOUT_VIEW,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  signals [CHNAGE_CURRENT_PAGE] =
    g_signal_new_class_handler ("change-current-page",
                                G_TYPE_FROM_CLASS (klass),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                G_CALLBACK (ide_layout_stack_change_current_page),
                                NULL, NULL,
                                g_cclosure_marshal_VOID__INT,
                                G_TYPE_NONE, 1, G_TYPE_INT);

  gtk_widget_class_set_css_name (widget_class, "idelayoutstack");
  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/builder/ui/ide-layout-stack.ui");
  gtk_widget_class_bind_template_child_private (widget_class, IdeLayoutStack, empty_state);
  gtk_widget_class_bind_template_child_private (widget_class, IdeLayoutStack, failed_state);
  gtk_widget_class_bind_template_child_private (widget_class, IdeLayoutStack, header);
  gtk_widget_class_bind_template_child_private (widget_class, IdeLayoutStack, stack);
  gtk_widget_class_bind_template_child_private (widget_class, IdeLayoutStack, top_stack);

  g_type_ensure (IDE_TYPE_LAYOUT_STACK_HEADER);
  g_type_ensure (IDE_TYPE_SHORTCUT_LABEL);
}

static void
ide_layout_stack_init (IdeLayoutStack *self)
{
  IdeLayoutStackPrivate *priv = ide_layout_stack_get_instance_private (self);

  gtk_widget_init_template (GTK_WIDGET (self));

  _ide_layout_stack_init_actions (self);
  _ide_layout_stack_init_shortcuts (self);

  priv->views = g_ptr_array_new ();

  priv->signals = dzl_signal_group_new (IDE_TYPE_LAYOUT_VIEW);

  dzl_signal_group_connect_swapped (priv->signals,
                                    "notify::failed",
                                    G_CALLBACK (ide_layout_stack_view_failed),
                                    self);

  priv->bindings = dzl_binding_group_new ();

  g_signal_connect_object (priv->bindings,
                           "notify::source",
                           G_CALLBACK (ide_layout_stack_bindings_notify_source),
                           self,
                           G_CONNECT_SWAPPED);

  dzl_binding_group_bind (priv->bindings, "title",
                          priv->header, "title",
                          G_BINDING_SYNC_CREATE);

  dzl_binding_group_bind (priv->bindings, "modified",
                          priv->header, "modified",
                          G_BINDING_SYNC_CREATE);

  g_signal_connect_object (priv->stack,
                           "notify::visible-child",
                           G_CALLBACK (ide_layout_stack_notify_visible_child),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_object (priv->stack,
                           "add",
                           G_CALLBACK (ide_layout_stack_view_added),
                           self,
                           G_CONNECT_SWAPPED | G_CONNECT_AFTER);

  g_signal_connect_object (priv->stack,
                           "remove",
                           G_CALLBACK (ide_layout_stack_view_removed),
                           self,
                           G_CONNECT_SWAPPED);

  _ide_layout_stack_header_set_views (priv->header, G_LIST_MODEL (self));
  _ide_layout_stack_header_update (priv->header, NULL);
}

GtkWidget *
ide_layout_stack_new (void)
{
  return g_object_new (IDE_TYPE_LAYOUT_STACK, NULL);
}

/**
 * ide_layout_stack_set_visible_child:
 * @self: a #IdeLayoutStack
 *
 * Sets the current view for the stack.
 *
 * Since: 3.26
 */
void
ide_layout_stack_set_visible_child (IdeLayoutStack *self,
                                    IdeLayoutView  *view)
{
  IdeLayoutStackPrivate *priv = ide_layout_stack_get_instance_private (self);

  g_return_if_fail (IDE_IS_LAYOUT_STACK (self));
  g_return_if_fail (IDE_IS_LAYOUT_VIEW (view));
  g_return_if_fail (gtk_widget_get_parent (GTK_WIDGET (view)) == (GtkWidget *)priv->stack);

  gtk_stack_set_visible_child (priv->stack, GTK_WIDGET (view));
}

/**
 * ide_layout_stack_get_visible_child:
 * @self: a #IdeLayoutStack
 *
 * Gets the visible #IdeLayoutView if there is one; otherwise %NULL.
 *
 * Returns: (nullable) (transfer none): An #IdeLayoutView or %NULL
 *
 * Since: 3.26
 */
IdeLayoutView *
ide_layout_stack_get_visible_child (IdeLayoutStack *self)
{
  IdeLayoutStackPrivate *priv = ide_layout_stack_get_instance_private (self);

  g_return_val_if_fail (IDE_IS_LAYOUT_STACK (self), NULL);

  return IDE_LAYOUT_VIEW (gtk_stack_get_visible_child (priv->stack));
}

/**
 * ide_layout_stack_get_titlebar:
 * @self: a #IdeLayoutStack
 *
 * Gets the #IdeLayoutStackHeader header that is at the top of the stack.
 *
 * Returns: (transfer none) (type Ide.LayoutStackHeader): The layout stack header.
 *
 * Since: 3.26
 */
GtkWidget *
ide_layout_stack_get_titlebar (IdeLayoutStack *self)
{
  IdeLayoutStackPrivate *priv = ide_layout_stack_get_instance_private (self);

  g_return_val_if_fail (IDE_IS_LAYOUT_STACK (self), NULL);

  return GTK_WIDGET (priv->header);
}

/**
 * ide_layout_stack_get_has_view:
 * @self: A #IdeLayoutStack
 *
 * Gets the "has-view" property.
 *
 * This property is a convenience to allow widgets to easily bind
 * properties based on whether or not a view is visible in the stack.
 *
 * Returns: %TRUE if the stack has a view
 *
 * Since: 3.26
 */
gboolean
ide_layout_stack_get_has_view (IdeLayoutStack *self)
{
  IdeLayoutView *visible_child;

  g_return_val_if_fail (IDE_IS_LAYOUT_STACK (self), FALSE);

  visible_child = ide_layout_stack_get_visible_child (self);

  return visible_child != NULL;
}

static void
ide_layout_stack_close_view_cb (GObject      *object,
                                GAsyncResult *result,
                                gpointer      user_data)
{
  IdeLayoutView *view = (IdeLayoutView *)object;
  g_autoptr(IdeLayoutStack) self = user_data;
  g_autoptr(GError) error = NULL;
  GtkWidget *toplevel;
  GtkWidget *focus;
  gboolean had_focus = FALSE;

  g_assert (IDE_IS_LAYOUT_VIEW (view));
  g_assert (G_IS_ASYNC_RESULT (result));
  g_assert (IDE_IS_LAYOUT_STACK (self));

  if (!ide_layout_view_agree_to_close_finish (view, result, &error))
    {
      g_message ("%s", error->message);
      return;
    }

  /* Keep track of whether or not the widget had focus (which
   * would happen if we were activated from a keybinding.
   */
  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (view));
  if (GTK_IS_WINDOW (toplevel) &&
      NULL != (focus = gtk_window_get_focus (GTK_WINDOW (toplevel))) &&
      (focus == GTK_WIDGET (view) ||
       gtk_widget_is_ancestor (focus, GTK_WIDGET (view))))
    had_focus = TRUE;

  /* Now we can destroy the child */
  gtk_widget_destroy (GTK_WIDGET (view));

  /* We don't want to leave the widget focus in an indeterminate
   * state so we immediately focus the next child in the stack.
   * But only do so if we had focus previously.
   */
  if (had_focus)
    {
      IdeLayoutView *visible_child = ide_layout_stack_get_visible_child (self);

      if (visible_child != NULL)
        gtk_widget_grab_focus (GTK_WIDGET (visible_child));
    }
}

void
_ide_layout_stack_request_close (IdeLayoutStack *self,
                                 IdeLayoutView  *view)
{
  g_return_if_fail (IDE_IS_LAYOUT_STACK (self));
  g_return_if_fail (IDE_IS_LAYOUT_VIEW (view));

  ide_layout_view_agree_to_close_async (view,
                                        NULL,
                                        ide_layout_stack_close_view_cb,
                                        g_object_ref (self));
}

static GType
ide_layout_stack_get_item_type (GListModel *model)
{
  return IDE_TYPE_LAYOUT_VIEW;
}

static guint
ide_layout_stack_get_n_items (GListModel *model)
{
  IdeLayoutStack *self = (IdeLayoutStack *)model;
  IdeLayoutStackPrivate *priv = ide_layout_stack_get_instance_private (self);

  g_assert (IDE_IS_LAYOUT_STACK (self));

  return priv->views ? priv->views->len : 0;
}

static gpointer
ide_layout_stack_get_item (GListModel *model,
                           guint       position)
{
  IdeLayoutStack *self = (IdeLayoutStack *)model;
  IdeLayoutStackPrivate *priv = ide_layout_stack_get_instance_private (self);

  g_assert (IDE_IS_LAYOUT_STACK (self));
  g_assert (position < priv->views->len);

  return g_object_ref (g_ptr_array_index (priv->views, position));
}

static void
list_model_iface_init (GListModelInterface *iface)
{
  iface->get_n_items = ide_layout_stack_get_n_items;
  iface->get_item = ide_layout_stack_get_item;
  iface->get_item_type = ide_layout_stack_get_item_type;
}

void
ide_layout_stack_agree_to_close_async (IdeLayoutStack      *self,
                                       GCancellable        *cancellable,
                                       GAsyncReadyCallback  callback,
                                       gpointer             user_data)
{
  g_return_if_fail (IDE_IS_LAYOUT_STACK (self));
  g_return_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable));

  IDE_LAYOUT_STACK_GET_CLASS (self)->agree_to_close_async (self, cancellable, callback, user_data);
}

gboolean
ide_layout_stack_agree_to_close_finish (IdeLayoutStack *self,
                                        GAsyncResult   *result,
                                        GError        **error)
{
  g_return_val_if_fail (IDE_IS_LAYOUT_STACK (self), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (result), FALSE);

  return IDE_LAYOUT_STACK_GET_CLASS (self)->agree_to_close_finish (self, result, error);
}

static void
animation_state_complete (gpointer data)
{
  AnimationState *state = data;

  g_assert (state != NULL);

  gtk_container_add (GTK_CONTAINER (state->dest), GTK_WIDGET (state->view));

  g_clear_object (&state->source);
  g_clear_object (&state->dest);
  g_clear_object (&state->view);
  g_clear_object (&state->theatric);
  g_slice_free (AnimationState, state);
}

static inline gboolean
is_uninitialized (GtkAllocation *alloc)
{
  return (alloc->x == -1 && alloc->y == -1 &&
          alloc->width == 1 && alloc->height == 1);
}

void
_ide_layout_stack_transfer (IdeLayoutStack *self,
                            IdeLayoutStack *dest,
                            IdeLayoutView  *view)
{
  IdeLayoutStackPrivate *priv = ide_layout_stack_get_instance_private (self);
  IdeLayoutStackPrivate *dest_priv = ide_layout_stack_get_instance_private (dest);

  g_return_if_fail (IDE_IS_LAYOUT_STACK (self));
  g_return_if_fail (IDE_IS_LAYOUT_STACK (dest));
  g_return_if_fail (IDE_IS_LAYOUT_VIEW (view));
  g_return_if_fail (GTK_WIDGET (priv->stack) == gtk_widget_get_parent (GTK_WIDGET (view)));

  /*
   * If both the old and the new stacks are mapped, we can animate
   * between them using a snapshot of the view. Well, we also need
   * to be sure they have a valid allocation, but that check is done
   * slightly after this because it makes things easier.
   */
  if (gtk_widget_get_mapped (GTK_WIDGET (self)) &&
      gtk_widget_get_mapped (GTK_WIDGET (dest)) &&
      gtk_widget_get_mapped (GTK_WIDGET (view)))
    {
      GtkAllocation alloc, dest_alloc;
      cairo_surface_t *surface = NULL;
      GdkWindow *window;
      GtkWidget *grid;
      gboolean enable_animations;

      grid = gtk_widget_get_ancestor (GTK_WIDGET (self), IDE_TYPE_LAYOUT_GRID);

      gtk_widget_get_allocation (GTK_WIDGET (view), &alloc);
      gtk_widget_get_allocation (GTK_WIDGET (dest), &dest_alloc);

      g_object_get (gtk_settings_get_default (),
                    "gtk-enable-animations", &enable_animations,
                    NULL);

      if (enable_animations &&
          grid != NULL &&
          !is_uninitialized (&alloc) &&
          !is_uninitialized (&dest_alloc) &&
          dest_alloc.width > 0 && dest_alloc.height > 0 &&
          NULL != (window = gtk_widget_get_window (GTK_WIDGET (view))) &&
          NULL != (surface = gdk_window_create_similar_surface (window,
                                                                CAIRO_CONTENT_COLOR,
                                                                alloc.width,
                                                                alloc.height)))
        {
          DzlBoxTheatric *theatric = NULL;
          AnimationState *state;
          cairo_t *cr;

          cr = cairo_create (surface);
          gtk_widget_draw (GTK_WIDGET (view), cr);
          cairo_destroy (cr);

          gtk_widget_translate_coordinates (GTK_WIDGET (priv->stack), grid, 0, 0,
                                            &alloc.x, &alloc.y);
          gtk_widget_translate_coordinates (GTK_WIDGET (dest_priv->stack), grid, 0, 0,
                                            &dest_alloc.x, &dest_alloc.y);

          theatric = g_object_new (DZL_TYPE_BOX_THEATRIC,
                                   "surface", surface,
                                   "height", alloc.height,
                                   "target", grid,
                                   "width", alloc.width,
                                   "x", alloc.x,
                                   "y", alloc.y,
                                   NULL);

          state = g_slice_new0 (AnimationState);
          state->source = g_object_ref (self);
          state->dest = g_object_ref (dest);
          state->view = g_object_ref (view);
          state->theatric = theatric;

          dzl_object_animate_full (theatric,
                                   DZL_ANIMATION_EASE_IN_OUT_CUBIC,
                                   TRANSITION_DURATION,
                                   gtk_widget_get_frame_clock (GTK_WIDGET (self)),
                                   animation_state_complete,
                                   state,
                                   "x", dest_alloc.x,
                                   "width", dest_alloc.width,
                                   "y", dest_alloc.y,
                                   "height", dest_alloc.height,
                                   NULL);

          gtk_container_remove (GTK_CONTAINER (priv->stack), GTK_WIDGET (view));

          cairo_surface_destroy (surface);

          return;
        }
    }

  g_object_ref (view);
  gtk_container_remove (GTK_CONTAINER (priv->stack), GTK_WIDGET (view));
  gtk_container_add (GTK_CONTAINER (dest_priv->stack), GTK_WIDGET (view));
  g_object_unref (view);
}

/**
 * ide_layout_stack_foreach_view:
 * @self: a #IdeLayoutStack
 * @callback: (scope call) (closure user_data): A callback for each view
 * @user_data: user data for @callback
 *
 * This function will call @callback for every view found in @self.
 *
 * Since: 3.26
 */
void
ide_layout_stack_foreach_view (IdeLayoutStack *self,
                               GtkCallback     callback,
                               gpointer        user_data)
{
  IdeLayoutStackPrivate *priv = ide_layout_stack_get_instance_private (self);

  g_return_if_fail (IDE_IS_LAYOUT_STACK (self));
  g_return_if_fail (callback != NULL);

  gtk_container_foreach (GTK_CONTAINER (priv->stack), callback, user_data);
}
