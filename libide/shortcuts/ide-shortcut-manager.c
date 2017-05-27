/* ide-shortcut-manager.c
 *
 * Copyright (C) 2016 Christian Hergert <chergert@redhat.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
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

#define G_LOG_DOMAIN "ide-shortcut-manager.h"

#include "ide-shortcut-private.h"

#include "ide-shortcut-controller.h"
#include "ide-shortcut-label.h"
#include "ide-shortcut-manager.h"
#include "ide-shortcuts-group.h"
#include "ide-shortcuts-section.h"
#include "ide-shortcuts-shortcut.h"

typedef struct
{
  IdeShortcutTheme *theme;
  GPtrArray        *themes;
  gchar            *user_dir;
  GNode            *root;
  GQueue            search_path;
} IdeShortcutManagerPrivate;

enum {
  PROP_0,
  PROP_THEME,
  PROP_THEME_NAME,
  PROP_USER_DIR,
  N_PROPS
};

enum {
  CHANGED,
  N_SIGNALS
};

static void list_model_iface_init (GListModelInterface *iface);
static void initable_iface_init   (GInitableIface      *iface);

G_DEFINE_TYPE_WITH_CODE (IdeShortcutManager, ide_shortcut_manager, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (IdeShortcutManager)
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, initable_iface_init)
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, list_model_iface_init))

static GParamSpec *properties [N_PROPS];
static guint signals [N_SIGNALS];

static gboolean
free_node_data (GNode    *node,
                gpointer  user_data)
{
  IdeShortcutNodeData *data = node->data;

  g_slice_free (IdeShortcutNodeData, data);

  return FALSE;
}

static void
ide_shortcut_manager_finalize (GObject *object)
{
  IdeShortcutManager *self = (IdeShortcutManager *)object;
  IdeShortcutManagerPrivate *priv = ide_shortcut_manager_get_instance_private (self);

  if (priv->root != NULL)
    {
      g_node_traverse (priv->root, G_IN_ORDER, G_TRAVERSE_ALL, -1, free_node_data, NULL);
      g_node_destroy (priv->root);
      priv->root = NULL;
    }

  g_clear_pointer (&priv->themes, g_ptr_array_unref);
  g_clear_pointer (&priv->user_dir, g_free);
  g_clear_object (&priv->theme);

  G_OBJECT_CLASS (ide_shortcut_manager_parent_class)->finalize (object);
}

static void
ide_shortcut_manager_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  IdeShortcutManager *self = (IdeShortcutManager *)object;

  switch (prop_id)
    {
    case PROP_THEME:
      g_value_set_object (value, ide_shortcut_manager_get_theme (self));
      break;

    case PROP_THEME_NAME:
      g_value_set_string (value, ide_shortcut_manager_get_theme_name (self));
      break;

    case PROP_USER_DIR:
      g_value_set_string (value, ide_shortcut_manager_get_user_dir (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ide_shortcut_manager_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  IdeShortcutManager *self = (IdeShortcutManager *)object;

  switch (prop_id)
    {
    case PROP_THEME:
      ide_shortcut_manager_set_theme (self, g_value_get_object (value));
      break;

    case PROP_THEME_NAME:
      ide_shortcut_manager_set_theme_name (self, g_value_get_string (value));
      break;

    case PROP_USER_DIR:
      ide_shortcut_manager_set_user_dir (self, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ide_shortcut_manager_class_init (IdeShortcutManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = ide_shortcut_manager_finalize;
  object_class->get_property = ide_shortcut_manager_get_property;
  object_class->set_property = ide_shortcut_manager_set_property;

  properties [PROP_THEME] =
    g_param_spec_object ("theme",
                         "Theme",
                         "The current key theme.",
                         IDE_TYPE_SHORTCUT_THEME,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_THEME_NAME] =
    g_param_spec_string ("theme-name",
                         "Theme Name",
                         "The name of the current theme",
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_USER_DIR] =
    g_param_spec_string ("user-dir",
                         "User Dir",
                         "The directory for saved user modifications",
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  signals [CHANGED] =
    g_signal_new ("changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}

static void
ide_shortcut_manager_init (IdeShortcutManager *self)
{
  IdeShortcutManagerPrivate *priv = ide_shortcut_manager_get_instance_private (self);

  priv->themes = g_ptr_array_new_with_free_func (g_object_unref);
  priv->root = g_node_new (NULL);
}

static void
ide_shortcut_manager_load_directory (IdeShortcutManager  *self,
                                     const gchar         *directory,
                                     GCancellable        *cancellable)
{
  g_autoptr(GDir) dir = NULL;
  const gchar *name;

  g_assert (IDE_IS_SHORTCUT_MANAGER (self));
  g_assert (directory != NULL);
  g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));

  if (!g_file_test (directory, G_FILE_TEST_IS_DIR))
    return;

  if (NULL == (dir = g_dir_open (directory, 0, NULL)))
    return;

  while (NULL != (name = g_dir_read_name (dir)))
    {
      g_autofree gchar *path = g_build_filename (directory, name, NULL);
      g_autoptr(IdeShortcutTheme) theme = NULL;
      g_autoptr(GError) local_error = NULL;

      theme = ide_shortcut_theme_new (NULL);

      if (ide_shortcut_theme_load_from_path (theme, path, cancellable, &local_error))
        ide_shortcut_manager_add_theme (self, theme);
      else
        g_warning ("%s", local_error->message);
    }
}

static void
ide_shortcut_manager_load_resources (IdeShortcutManager *self,
                                     const gchar        *resource_dir,
                                     GCancellable       *cancellable)
{
  g_auto(GStrv) children = NULL;

  g_assert (IDE_IS_SHORTCUT_MANAGER (self));
  g_assert (resource_dir != NULL);
  g_assert (g_str_has_prefix (resource_dir, "resource://"));
  g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));

  children = g_resources_enumerate_children (resource_dir, 0, NULL);

  if (children != NULL)
    {
      for (guint i = 0; children[i] != NULL; i++)
        {
          g_autofree gchar *path = g_build_filename (resource_dir, children[i], NULL);
          g_autoptr(IdeShortcutTheme) theme = NULL;
          g_autoptr(GError) local_error = NULL;
          g_autoptr(GBytes) bytes = NULL;
          const gchar *data;
          gsize len = 0;

          if (NULL == (bytes = g_resources_lookup_data (path, 0, NULL)))
            continue;

          data = g_bytes_get_data (bytes, &len);
          theme = ide_shortcut_theme_new (NULL);

          if (ide_shortcut_theme_load_from_data (theme, data, len, &local_error))
            ide_shortcut_manager_add_theme (self, theme);
          else
            g_warning ("%s", local_error->message);
        }
    }
}

static gboolean
ide_shortcut_manager_initiable_init (GInitable     *initable,
                                     GCancellable  *cancellable,
                                     GError       **error)
{
  IdeShortcutManager *self = (IdeShortcutManager *)initable;
  IdeShortcutManagerPrivate *priv = ide_shortcut_manager_get_instance_private (self);

  g_assert (IDE_IS_SHORTCUT_MANAGER (self));
  g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));

  for (const GList *iter = priv->search_path.tail; iter != NULL; iter = iter->prev)
    {
      const gchar *directory = iter->data;

      if (g_str_has_prefix (directory, "resource://"))
        ide_shortcut_manager_load_resources (self, directory, cancellable);
      else
        ide_shortcut_manager_load_directory (self, directory, cancellable);
    }

  return TRUE;
}

static void
initable_iface_init (GInitableIface *iface)
{
  iface->init = ide_shortcut_manager_initiable_init;
}


/**
 * ide_shortcut_manager_get_default:
 *
 * Gets the singleton #IdeShortcutManager for the process.
 *
 * Returns: (transfer none) (not nullable): An #IdeShortcutManager.
 */
IdeShortcutManager *
ide_shortcut_manager_get_default (void)
{
  static IdeShortcutManager *instance;

  if (instance == NULL)
    {
      instance = g_object_new (IDE_TYPE_SHORTCUT_MANAGER, NULL);
      g_object_add_weak_pointer (G_OBJECT (instance), (gpointer *)&instance);
    }

  return instance;
}

/**
 * ide_shortcut_manager_get_theme:
 * @self: (nullable): A #IdeShortcutManager or %NULL
 *
 * Gets the "theme" property.
 *
 * Returns: (transfer none) (not nullable): An #IdeShortcutTheme.
 */
IdeShortcutTheme *
ide_shortcut_manager_get_theme (IdeShortcutManager *self)
{
  IdeShortcutManagerPrivate *priv;

  g_return_val_if_fail (!self || IDE_IS_SHORTCUT_MANAGER (self), NULL);

  if (self == NULL)
    self = ide_shortcut_manager_get_default ();

  priv = ide_shortcut_manager_get_instance_private (self);

  if (priv->theme == NULL)
    priv->theme = g_object_new (IDE_TYPE_SHORTCUT_THEME,
                                "name", "default",
                                NULL);

  return priv->theme;
}

/**
 * ide_shortcut_manager_set_theme:
 * @self: An #IdeShortcutManager
 * @theme: (not nullable): An #IdeShortcutTheme
 *
 * Sets the theme for the shortcut manager.
 */
void
ide_shortcut_manager_set_theme (IdeShortcutManager *self,
                                IdeShortcutTheme   *theme)
{
  IdeShortcutManagerPrivate *priv = ide_shortcut_manager_get_instance_private (self);

  g_return_if_fail (IDE_IS_SHORTCUT_MANAGER (self));
  g_return_if_fail (IDE_IS_SHORTCUT_THEME (theme));

  /*
   * It is important that IdeShortcutController instances watch for
   * notify::theme so that they can reset their state. Otherwise, we
   * could be transitioning between incorrect contexts.
   */

  if (g_set_object (&priv->theme, theme))
    {
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_THEME]);
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_THEME_NAME]);
    }
}

/**
 * ide_shortcut_manager_handle_event:
 * @self: (nullable): An #IdeShortcutManager
 * @toplevel: A #GtkWidget or %NULL.
 * @event: A #GdkEventKey event to handle.
 *
 * This function will try to dispatch @event to the proper widget and
 * #IdeShortcutContext. If the event is handled, then %TRUE is returned.
 *
 * You should call this from #GtkWidget::key-press-event handler in your
 * #GtkWindow toplevel.
 *
 * Returns: %TRUE if the event was handled.
 */
gboolean
ide_shortcut_manager_handle_event (IdeShortcutManager *self,
                                   const GdkEventKey  *event,
                                   GtkWidget          *toplevel)
{
  GtkWidget *widget;
  GtkWidget *focus;
  GdkModifierType modifier;

  g_return_val_if_fail (!self || IDE_IS_SHORTCUT_MANAGER (self), FALSE);
  g_return_val_if_fail (!toplevel || GTK_IS_WINDOW (toplevel), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (self == NULL)
    self = ide_shortcut_manager_get_default ();

  if (toplevel == NULL)
    {
      gpointer user_data;

      gdk_window_get_user_data (event->window, &user_data);
      g_return_val_if_fail (GTK_IS_WIDGET (user_data), FALSE);

      toplevel = gtk_widget_get_toplevel (user_data);
      g_return_val_if_fail (GTK_IS_WINDOW (toplevel), FALSE);
    }

  if (event->type != GDK_KEY_PRESS)
    return GDK_EVENT_PROPAGATE;

  g_assert (IDE_IS_SHORTCUT_MANAGER (self));
  g_assert (GTK_IS_WINDOW (toplevel));
  g_assert (event != NULL);

  modifier = event->state & gtk_accelerator_get_default_mod_mask ();
  widget = focus = gtk_window_get_focus (GTK_WINDOW (toplevel));

  while (widget != NULL)
    {
      g_autoptr(GtkWidget) widget_hold = g_object_ref (widget);
      IdeShortcutController *controller;
      gboolean use_binding_sets = TRUE;

      if (NULL != (controller = ide_shortcut_controller_find (widget)))
        {
          IdeShortcutContext *context = ide_shortcut_controller_get_context (controller);

          /*
           * Fetch this property first as the controller context could change
           * during activation of the handle_event().
           */
          if (context != NULL)
            g_object_get (context,
                          "use-binding-sets", &use_binding_sets,
                          NULL);

          /*
           * Now try to activate the event using the controller.
           */
          if (ide_shortcut_controller_handle_event (controller, event))
            return GDK_EVENT_STOP;
        }

      /*
       * If the current context at activation indicates that we can
       * dispatch using the default binding sets for the widget, go
       * ahead and try to do that.
       */
      if (use_binding_sets)
        {
          GtkStyleContext *style_context;
          g_autoptr(GPtrArray) sets = NULL;

          style_context = gtk_widget_get_style_context (widget);
          gtk_style_context_get (style_context,
                                 gtk_style_context_get_state (style_context),
                                 "-gtk-key-bindings", &sets,
                                 NULL);

          if (sets != NULL)
            {
              for (guint i = 0; i < sets->len; i++)
                {
                  GtkBindingSet *set = g_ptr_array_index (sets, i);

                  if (gtk_binding_set_activate (set, event->keyval, modifier, G_OBJECT (widget)))
                    return GDK_EVENT_STOP;
                }
            }

          /*
           * Only if this widget is also our focus, try to activate the default
           * keybindings for the widget.
           */
          if (widget == focus)
            {
              GtkBindingSet *set = gtk_binding_set_by_class (G_OBJECT_GET_CLASS (widget));

              if (gtk_binding_set_activate (set, event->keyval, modifier, G_OBJECT (widget)))
                return GDK_EVENT_STOP;
            }
        }

      widget = gtk_widget_get_parent (widget);
    }

  return GDK_EVENT_PROPAGATE;
}

const gchar *
ide_shortcut_manager_get_theme_name (IdeShortcutManager *self)
{
  IdeShortcutManagerPrivate *priv = ide_shortcut_manager_get_instance_private (self);
  const gchar *ret = NULL;

  g_return_val_if_fail (IDE_IS_SHORTCUT_MANAGER (self), NULL);

  if (priv->theme != NULL)
    ret = ide_shortcut_theme_get_name (priv->theme);

  return ret;
}

void
ide_shortcut_manager_set_theme_name (IdeShortcutManager *self,
                                     const gchar        *name)
{
  IdeShortcutManagerPrivate *priv = ide_shortcut_manager_get_instance_private (self);

  g_return_if_fail (IDE_IS_SHORTCUT_MANAGER (self));

  if (name == NULL)
    name = "default";

  for (guint i = 0; i < priv->themes->len; i++)
    {
      IdeShortcutTheme *theme = g_ptr_array_index (priv->themes, i);
      const gchar *theme_name = ide_shortcut_theme_get_name (theme);

      if (g_strcmp0 (name, theme_name) == 0)
        {
          ide_shortcut_manager_set_theme (self, theme);
          return;
        }
    }

  g_warning ("No such shortcut theme “%s”", name);
}

static guint
ide_shortcut_manager_get_n_items (GListModel *model)
{
  IdeShortcutManager *self = (IdeShortcutManager *)model;
  IdeShortcutManagerPrivate *priv = ide_shortcut_manager_get_instance_private (self);

  g_return_val_if_fail (IDE_IS_SHORTCUT_MANAGER (self), 0);

  return priv->themes->len;
}

static GType
ide_shortcut_manager_get_item_type (GListModel *model)
{
  return IDE_TYPE_SHORTCUT_THEME;
}

static gpointer
ide_shortcut_manager_get_item (GListModel *model,
                               guint       position)
{
  IdeShortcutManager *self = (IdeShortcutManager *)model;
  IdeShortcutManagerPrivate *priv = ide_shortcut_manager_get_instance_private (self);

  g_return_val_if_fail (IDE_IS_SHORTCUT_MANAGER (self), NULL);
  g_return_val_if_fail (position < priv->themes->len, NULL);

  return g_object_ref (g_ptr_array_index (priv->themes, position));
}

static void
list_model_iface_init (GListModelInterface *iface)
{
  iface->get_n_items = ide_shortcut_manager_get_n_items;
  iface->get_item_type = ide_shortcut_manager_get_item_type;
  iface->get_item = ide_shortcut_manager_get_item;
}

void
ide_shortcut_manager_add_theme (IdeShortcutManager *self,
                                IdeShortcutTheme   *theme)
{
  IdeShortcutManagerPrivate *priv = ide_shortcut_manager_get_instance_private (self);
  guint position;

  g_return_if_fail (IDE_IS_SHORTCUT_MANAGER (self));
  g_return_if_fail (IDE_IS_SHORTCUT_THEME (theme));

  for (guint i = 0; i < priv->themes->len; i++)
    {
      if (g_ptr_array_index (priv->themes, i) == theme)
        {
          g_warning ("%s named %s has already been added",
                     G_OBJECT_TYPE_NAME (theme),
                     ide_shortcut_theme_get_name (theme));
          return;
        }
    }

  position = priv->themes->len;

  g_ptr_array_add (priv->themes, g_object_ref (theme));

  g_list_model_items_changed (G_LIST_MODEL (self), position, 0, 1);
}

void
ide_shortcut_manager_remove_theme (IdeShortcutManager *self,
                                   IdeShortcutTheme   *theme)
{
  IdeShortcutManagerPrivate *priv = ide_shortcut_manager_get_instance_private (self);

  g_return_if_fail (IDE_IS_SHORTCUT_MANAGER (self));
  g_return_if_fail (IDE_IS_SHORTCUT_THEME (theme));

  for (guint i = 0; i < priv->themes->len; i++)
    {
      if (g_ptr_array_index (priv->themes, i) == theme)
        {
          g_ptr_array_remove_index (priv->themes, i);
          g_list_model_items_changed (G_LIST_MODEL (self), i, 1, 0);
          break;
        }
    }
}

const gchar *
ide_shortcut_manager_get_user_dir (IdeShortcutManager *self)
{
  IdeShortcutManagerPrivate *priv = ide_shortcut_manager_get_instance_private (self);

  g_return_val_if_fail (IDE_IS_SHORTCUT_MANAGER (self), NULL);

  if (priv->user_dir == NULL)
    {
      priv->user_dir = g_build_filename (g_get_user_data_dir (),
                                         g_get_prgname (),
                                         NULL);
    }

  return priv->user_dir;
}

void
ide_shortcut_manager_set_user_dir (IdeShortcutManager *self,
                                   const gchar        *user_dir)
{
  IdeShortcutManagerPrivate *priv = ide_shortcut_manager_get_instance_private (self);

  g_return_if_fail (IDE_IS_SHORTCUT_MANAGER (self));

  if (g_strcmp0 (user_dir, priv->user_dir) != 0)
    {
      g_free (priv->user_dir);
      priv->user_dir = g_strdup (user_dir);
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_USER_DIR]);
    }
}

void
ide_shortcut_manager_append_search_path (IdeShortcutManager *self,
                                         const gchar        *directory)
{
  IdeShortcutManagerPrivate *priv;

  g_return_if_fail (!self || IDE_IS_SHORTCUT_MANAGER (self));
  g_return_if_fail (directory != NULL);

  if (self == NULL)
    self = ide_shortcut_manager_get_default ();

  priv = ide_shortcut_manager_get_instance_private (self);

  g_queue_push_tail (&priv->search_path, g_strdup (directory));
}

void
ide_shortcut_manager_prepend_search_path (IdeShortcutManager *self,
                                          const gchar        *directory)
{
  IdeShortcutManagerPrivate *priv;

  g_return_if_fail (!self || IDE_IS_SHORTCUT_MANAGER (self));
  g_return_if_fail (directory != NULL);

  if (self == NULL)
    self = ide_shortcut_manager_get_default ();

  priv = ide_shortcut_manager_get_instance_private (self);

  g_queue_push_head (&priv->search_path, g_strdup (directory));
}

/**
 * ide_shortcut_manager_get_search_path:
 * @self: A #IdeShortcutManager
 *
 * This function will get the list of search path entries. These are used to
 * load themes for the application. You should set this search path for
 * themes before calling g_initable_init() on the search manager.
 *
 * Returns: (transfer none) (element-type utf8): A #GList containing each of
 *   the search path items used to load shortcut themes.
 */
const GList *
ide_shortcut_manager_get_search_path (IdeShortcutManager *self)
{
  IdeShortcutManagerPrivate *priv;

  if (self == NULL)
    self = ide_shortcut_manager_get_default ();

  priv = ide_shortcut_manager_get_instance_private (self);

  return priv->search_path.head;
}

static GNode *
ide_shortcut_manager_find_child (IdeShortcutManager  *self,
                                 GNode               *parent,
                                 IdeShortcutNodeType  type,
                                 const gchar         *name)
{
  IdeShortcutNodeData *data;

  g_assert (IDE_IS_SHORTCUT_MANAGER (self));
  g_assert (parent != NULL);
  g_assert (type != 0);
  g_assert (name != NULL);

  for (GNode *iter = parent->children; iter != NULL; iter = iter->next)
    {
      data = iter->data;

      if (data->type == type && data->name == name)
        return iter;
    }

  return NULL;
}

static GNode *
ide_shortcut_manager_get_group (IdeShortcutManager *self,
                                const gchar        *section,
                                const gchar        *group)
{
  IdeShortcutManagerPrivate *priv = ide_shortcut_manager_get_instance_private (self);
  IdeShortcutNodeData *data;
  GNode *parent;
  GNode *node;

  g_assert (IDE_IS_SHORTCUT_MANAGER (self));
  g_assert (section != NULL);
  g_assert (group != NULL);

  node = ide_shortcut_manager_find_child (self, priv->root, IDE_SHORTCUT_NODE_SECTION, section);

  if (node == NULL)
    {
      data = g_slice_new0 (IdeShortcutNodeData);
      data->type = IDE_SHORTCUT_NODE_SECTION;
      data->name = g_intern_string (section);
      data->title = g_intern_string (section);
      data->subtitle = NULL;

      node = g_node_append_data (priv->root, data);
    }

  parent = node;

  node = ide_shortcut_manager_find_child (self, parent, IDE_SHORTCUT_NODE_GROUP, group);

  if (node == NULL)
    {
      data = g_slice_new0 (IdeShortcutNodeData);
      data->type = IDE_SHORTCUT_NODE_GROUP;
      data->name = g_intern_string (group);
      data->title = g_intern_string (group);
      data->subtitle = NULL;

      node = g_node_append_data (parent, data);
    }

  g_assert (node != NULL);

  return node;
}

void
ide_shortcut_manager_add_action (IdeShortcutManager *self,
                                 const gchar        *detailed_action_name,
                                 const gchar        *section,
                                 const gchar        *group,
                                 const gchar        *title,
                                 const gchar        *subtitle)
{
  IdeShortcutNodeData *data;
  GNode *parent;

  g_return_if_fail (!self || IDE_IS_SHORTCUT_MANAGER (self));
  g_return_if_fail (detailed_action_name != NULL);
  g_return_if_fail (title != NULL);

  if (self == NULL)
    self = ide_shortcut_manager_get_default ();

  section = g_intern_string (section);
  group = g_intern_string (group);
  title = g_intern_string (title);
  subtitle = g_intern_string (subtitle);

  parent = ide_shortcut_manager_get_group (self, section, group);

  g_assert (parent != NULL);

  data = g_slice_new0 (IdeShortcutNodeData);
  data->type = IDE_SHORTCUT_NODE_ACTION;
  data->name = g_intern_string (detailed_action_name);
  data->title = title;
  data->subtitle = subtitle;

  g_node_append_data (parent, data);

  g_signal_emit (self, signals [CHANGED], 0);
}

void
ide_shortcut_manager_add_command (IdeShortcutManager *self,
                                  const gchar        *command,
                                  const gchar        *section,
                                  const gchar        *group,
                                  const gchar        *title,
                                  const gchar        *subtitle)
{
  IdeShortcutNodeData *data;
  GNode *parent;

  g_return_if_fail (!self || IDE_IS_SHORTCUT_MANAGER (self));
  g_return_if_fail (command != NULL);
  g_return_if_fail (title != NULL);

  if (self == NULL)
    self = ide_shortcut_manager_get_default ();

  section = g_intern_string (section);
  group = g_intern_string (group);
  title = g_intern_string (title);
  subtitle = g_intern_string (subtitle);

  parent = ide_shortcut_manager_get_group (self, section, group);

  g_assert (parent != NULL);

  data = g_slice_new0 (IdeShortcutNodeData);
  data->type = IDE_SHORTCUT_NODE_COMMAND;
  data->name = g_intern_string (command);
  data->title = title;
  data->subtitle = subtitle;

  g_node_append_data (parent, data);

  g_signal_emit (self, signals [CHANGED], 0);
}

static IdeShortcutsShortcut *
create_shortcut (const IdeShortcutChord *chord,
                 const gchar            *title,
                 const gchar            *subtitle)
{
  g_autofree gchar *accel = ide_shortcut_chord_to_string (chord);

  return g_object_new (IDE_TYPE_SHORTCUTS_SHORTCUT,
                       "accelerator", accel,
                       "subtitle", subtitle,
                       "title", title,
                       "visible", TRUE,
                       NULL);
}

/**
 * ide_shortcut_manager_add_shortcuts_to_window:
 * @self: A #IdeShortcutManager
 * @window: A #IdeShortcutsWindow
 *
 * Adds shortcuts registered with the #IdeShortcutManager to the
 * #IdeShortcutsWindow.
 */
void
ide_shortcut_manager_add_shortcuts_to_window (IdeShortcutManager *self,
                                              IdeShortcutsWindow *window)
{
  IdeShortcutManagerPrivate *priv;
  GNode *parent;

  g_return_if_fail (!self || IDE_IS_SHORTCUT_MANAGER (self));
  g_return_if_fail (IDE_IS_SHORTCUTS_WINDOW (window));

  if (self == NULL)
    self = ide_shortcut_manager_get_default ();
  priv = ide_shortcut_manager_get_instance_private (self);

  /*
   * The GNode tree is in four levels. priv->root is the root of the tree and
   * contains no data items itself. It is just our stable root. The children
   * of priv->root are our section nodes. Each section node has group nodes
   * as children. Finally, the shortcut nodes are the leaves.
   */

  parent = priv->root;

  for (const GNode *sections = parent->children; sections != NULL; sections = sections->next)
    {
      IdeShortcutNodeData *section_data = sections->data;
      IdeShortcutsSection *section;

      section = g_object_new (IDE_TYPE_SHORTCUTS_SECTION,
                              "title", section_data->title,
                              "section-name", section_data->title,
                              "visible", TRUE,
                              NULL);

      for (const GNode *groups = sections->children; groups != NULL; groups = groups->next)
        {
          IdeShortcutNodeData *group_data = groups->data;
          IdeShortcutsGroup *group;

          group = g_object_new (IDE_TYPE_SHORTCUTS_GROUP,
                                "title", group_data->title,
                                "visible", TRUE,
                                NULL);

          for (const GNode *iter = groups->children; iter != NULL; iter = iter->next)
            {
              IdeShortcutNodeData *data = iter->data;
              IdeShortcutsShortcut *shortcut;
              const IdeShortcutChord *chord;
              g_autofree gchar *accel = NULL;

              if (data->type == IDE_SHORTCUT_NODE_ACTION)
                chord = ide_shortcut_theme_get_chord_for_action (priv->theme, data->name);
              else
                chord = ide_shortcut_theme_get_chord_for_command (priv->theme, data->name);

              accel = ide_shortcut_chord_to_string (chord);

              shortcut = create_shortcut (chord, data->title, data->subtitle);
              gtk_container_add (GTK_CONTAINER (group), GTK_WIDGET (shortcut));
            }

          gtk_container_add (GTK_CONTAINER (section), GTK_WIDGET (group));
        }

      gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (section));
    }
}

GNode *
_ide_shortcut_manager_get_root (IdeShortcutManager *self)
{
  IdeShortcutManagerPrivate *priv = ide_shortcut_manager_get_instance_private (self);

  g_return_val_if_fail (IDE_IS_SHORTCUT_MANAGER (self), NULL);

  return priv->root;
}

/**
 * ide_shortcut_manager_add_shortcut_entries:
 * @self: (nullable): a #IdeShortcutManager or %NULL for the default
 * @shortcuts: (array length=n_shortcuts): shortcuts to add
 * @n_shortcuts: the number of entries in @shortcuts
 * @translation_domain: (nullable): the gettext domain to use for translations
 *
 * This method will add @shortcuts to the #IdeShortcutManager.
 *
 * This provides a simple way for widgets to add their shortcuts to the manager
 * so that they may be overriden by themes or the end user.
 */
void
ide_shortcut_manager_add_shortcut_entries (IdeShortcutManager     *self,
                                           const IdeShortcutEntry *shortcuts,
                                           guint                   n_shortcuts,
                                           const gchar            *translation_domain)
{
  g_return_if_fail (!self || IDE_IS_SHORTCUT_MANAGER (self));
  g_return_if_fail (shortcuts != NULL || n_shortcuts == 0);

  if (self == NULL)
    self = ide_shortcut_manager_get_default ();

  for (guint i = 0; i < n_shortcuts; i++)
    {
      const IdeShortcutEntry *entry = &shortcuts[i];

      ide_shortcut_manager_add_command (self,
                                        g_dgettext (translation_domain, entry->command),
                                        g_dgettext (translation_domain, entry->section),
                                        g_dgettext (translation_domain, entry->group),
                                        g_dgettext (translation_domain, entry->title),
                                        g_dgettext (translation_domain, entry->subtitle));
    }
}

/**
 * ide_shortcut_manager_get_theme_by_name:
 * @self: a #IdeShortcutManager
 *
 * Locates a theme by the name of the theme.
 *
 * Returns: (transfer none) (nullable): A #IdeShortcutTheme or %NULL.
 */
IdeShortcutTheme *
ide_shortcut_manager_get_theme_by_name (IdeShortcutManager *self,
                                        const gchar        *theme_name)
{
  IdeShortcutManagerPrivate *priv = ide_shortcut_manager_get_instance_private (self);

  g_return_val_if_fail (IDE_IS_SHORTCUT_MANAGER (self), NULL);
  g_return_val_if_fail (theme_name != NULL, NULL);

  for (guint i = 0; i < priv->themes->len; i++)
    {
      IdeShortcutTheme *theme = g_ptr_array_index (priv->themes, i);

      g_assert (IDE_IS_SHORTCUT_THEME (theme));

      if (g_strcmp0 (theme_name, ide_shortcut_theme_get_name (theme)) == 0)
        return theme;
    }

  return NULL;
}
