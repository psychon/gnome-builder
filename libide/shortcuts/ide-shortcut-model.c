/* ide-shortcut-model.c
 *
 * Copyright (C) 2017 Christian Hergert <chergert@redhat.com>
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

#define G_LOG_DOMAIN "ide-shortcut-model"

#include "ide-shortcut-model.h"
#include "ide-shortcut-private.h"

struct _IdeShortcutModel
{
  GtkTreeStore        parent_instance;
  IdeShortcutManager *manager;
  IdeShortcutTheme   *theme;
};

G_DEFINE_TYPE (IdeShortcutModel, ide_shortcut_model, GTK_TYPE_TREE_STORE)

enum {
  PROP_0,
  PROP_MANAGER,
  PROP_THEME,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

void
ide_shortcut_model_rebuild (IdeShortcutModel *self)
{
  g_assert (IDE_IS_SHORTCUT_MODEL (self));

  gtk_tree_store_clear (GTK_TREE_STORE (self));

  if (self->manager != NULL)
    {
      GNode *root;

      root = _ide_shortcut_manager_get_root (self->manager);

      for (const GNode *iter = root->children; iter != NULL; iter = iter->next)
        {
          for (const GNode *groups = iter->children; groups != NULL; groups = groups->next)
            {
              IdeShortcutNodeData *group = groups->data;
              GtkTreeIter p;

              gtk_tree_store_append (GTK_TREE_STORE (self), &p, NULL);
              gtk_tree_store_set (GTK_TREE_STORE (self), &p,
                                  IDE_SHORTCUT_MODEL_COLUMN_TITLE, group->title,
                                  -1);

              for (const GNode *sc = groups->children; sc != NULL; sc = sc->next)
                {
                  IdeShortcutNodeData *shortcut = sc->data;
                  const IdeShortcutChord *chord = NULL;
                  g_autofree gchar *accel = NULL;
                  g_autofree gchar *down = NULL;
                  GtkTreeIter p2;

                  if (shortcut->type == IDE_SHORTCUT_NODE_ACTION)
                    chord = ide_shortcut_theme_get_chord_for_action (self->theme, shortcut->name);
                  else if (shortcut->type == IDE_SHORTCUT_NODE_COMMAND)
                    chord = ide_shortcut_theme_get_chord_for_command (self->theme, shortcut->name);

                  accel = ide_shortcut_chord_get_label (chord);
                  down = g_utf8_casefold (shortcut->title, -1);

                  gtk_tree_store_append (GTK_TREE_STORE (self), &p2, &p);
                  gtk_tree_store_set (GTK_TREE_STORE (self), &p2,
                                      IDE_SHORTCUT_MODEL_COLUMN_TYPE, shortcut->type,
                                      IDE_SHORTCUT_MODEL_COLUMN_ID, shortcut->name,
                                      IDE_SHORTCUT_MODEL_COLUMN_TITLE, shortcut->title,
                                      IDE_SHORTCUT_MODEL_COLUMN_ACCEL, accel,
                                      IDE_SHORTCUT_MODEL_COLUMN_KEYWORDS, down,
                                      IDE_SHORTCUT_MODEL_COLUMN_CHORD, chord,
                                      -1);
                }
            }
        }
    }
}

static void
ide_shortcut_model_constructed (GObject *object)
{
  IdeShortcutModel *self = (IdeShortcutModel *)object;

  g_assert (IDE_IS_SHORTCUT_MODEL (self));

  G_OBJECT_CLASS (ide_shortcut_model_parent_class)->constructed (object);

  ide_shortcut_model_rebuild (self);
}

static void
ide_shortcut_model_finalize (GObject *object)
{
  IdeShortcutModel *self = (IdeShortcutModel *)object;

  g_clear_object (&self->manager);
  g_clear_object (&self->theme);

  G_OBJECT_CLASS (ide_shortcut_model_parent_class)->finalize (object);
}

static void
ide_shortcut_model_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  IdeShortcutModel *self = IDE_SHORTCUT_MODEL (object);

  switch (prop_id)
    {
    case PROP_MANAGER:
      g_value_set_object (value, ide_shortcut_model_get_manager (self));
      break;

    case PROP_THEME:
      g_value_set_object (value, ide_shortcut_model_get_theme (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ide_shortcut_model_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  IdeShortcutModel *self = IDE_SHORTCUT_MODEL (object);

  switch (prop_id)
    {
    case PROP_MANAGER:
      ide_shortcut_model_set_manager (self, g_value_get_object (value));
      break;

    case PROP_THEME:
      ide_shortcut_model_set_theme (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ide_shortcut_model_class_init (IdeShortcutModelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = ide_shortcut_model_constructed;
  object_class->finalize = ide_shortcut_model_finalize;
  object_class->get_property = ide_shortcut_model_get_property;
  object_class->set_property = ide_shortcut_model_set_property;

  properties [PROP_MANAGER] =
    g_param_spec_object ("manager",
                         "Manager",
                         "Manager",
                         IDE_TYPE_SHORTCUT_MANAGER,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_THEME] =
    g_param_spec_object ("theme",
                         "Theme",
                         "Theme",
                         IDE_TYPE_SHORTCUT_THEME,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
ide_shortcut_model_init (IdeShortcutModel *self)
{
  GType element_types[] = {
    G_TYPE_INT,
    G_TYPE_STRING,
    G_TYPE_STRING,
    G_TYPE_STRING,
    G_TYPE_STRING,
    IDE_TYPE_SHORTCUT_CHORD,
  };

  G_STATIC_ASSERT (G_N_ELEMENTS (element_types) == IDE_SHORTCUT_MODEL_N_COLUMNS);

  self->manager = g_object_ref (ide_shortcut_manager_get_default ());

  gtk_tree_store_set_column_types (GTK_TREE_STORE (self),
                                   G_N_ELEMENTS (element_types),
                                   element_types);
}

/**
 * ide_shortcut_model_new:
 *
 * Returns: (transfer full): A #GtkTreeModel
 */
GtkTreeModel *
ide_shortcut_model_new (void)
{
  return g_object_new (IDE_TYPE_SHORTCUT_MODEL, NULL);
}

/**
 * ide_shortcut_model_get_manager:
 * @self: a #IdeShortcutModel
 *
 * Gets the manager to be edited.
 *
 * Returns: (transfer none): A #IdeShortcutManager
 */
IdeShortcutManager *
ide_shortcut_model_get_manager (IdeShortcutModel *self)
{
  g_return_val_if_fail (IDE_IS_SHORTCUT_MODEL (self), NULL);

  return self->manager;
}

void
ide_shortcut_model_set_manager (IdeShortcutModel   *self,
                                IdeShortcutManager *manager)
{
  g_return_if_fail (IDE_IS_SHORTCUT_MODEL (self));
  g_return_if_fail (!manager || IDE_IS_SHORTCUT_MANAGER (manager));

  if (g_set_object (&self->manager, manager))
    {
      ide_shortcut_model_rebuild (self);
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_MANAGER]);
    }
}

/**
 * ide_shortcut_model_get_theme:
 * @self: a #IdeShortcutModel
 *
 * Get the theme to be edited.
 *
 * Returns: (transfer none): A #IdeShortcutTheme
 */
IdeShortcutTheme *
ide_shortcut_model_get_theme (IdeShortcutModel *self)
{
  g_return_val_if_fail (IDE_IS_SHORTCUT_MODEL (self), NULL);

  return self->theme;
}

void
ide_shortcut_model_set_theme (IdeShortcutModel *self,
                              IdeShortcutTheme *theme)
{
  g_return_if_fail (IDE_IS_SHORTCUT_MODEL (self));
  g_return_if_fail (!theme || IDE_IS_SHORTCUT_THEME (theme));

  if (g_set_object (&self->theme, theme))
    {
      ide_shortcut_model_rebuild (self);
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_THEME]);
    }
}

static void
ide_shortcut_model_apply (IdeShortcutModel *self,
                          GtkTreeIter      *iter)
{
  g_autoptr(IdeShortcutChord) chord = NULL;
  g_autofree gchar *id = NULL;
  gint type = 0;

  g_assert (IDE_IS_SHORTCUT_MODEL (self));
  g_assert (IDE_IS_SHORTCUT_THEME (self->theme));
  g_assert (iter != NULL);

  gtk_tree_model_get (GTK_TREE_MODEL (self), iter,
                      IDE_SHORTCUT_MODEL_COLUMN_TYPE, &type,
                      IDE_SHORTCUT_MODEL_COLUMN_ID, &id,
                      IDE_SHORTCUT_MODEL_COLUMN_CHORD, &chord,
                      -1);

  if (type == IDE_SHORTCUT_NODE_ACTION)
    ide_shortcut_theme_set_chord_for_action (self->theme, id, chord);
  else if (type == IDE_SHORTCUT_NODE_COMMAND)
    ide_shortcut_theme_set_chord_for_command (self->theme, id, chord);
  else
    g_warning ("Unknown type: %d", type);
}

void
ide_shortcut_model_set_chord (IdeShortcutModel       *self,
                              GtkTreeIter            *iter,
                              const IdeShortcutChord *chord)
{
  g_autofree gchar *accel = NULL;

  g_return_if_fail (IDE_IS_SHORTCUT_MODEL (self));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (gtk_tree_store_iter_is_valid (GTK_TREE_STORE (self), iter));

  accel = ide_shortcut_chord_get_label (chord);

  gtk_tree_store_set (GTK_TREE_STORE (self), iter,
                      IDE_SHORTCUT_MODEL_COLUMN_ACCEL, accel,
                      IDE_SHORTCUT_MODEL_COLUMN_CHORD, chord,
                      -1);

  ide_shortcut_model_apply (self, iter);
}
