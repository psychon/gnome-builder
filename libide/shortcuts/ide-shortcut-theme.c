/* ide-shortcut-theme.c
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

#define G_LOG_DOMAIN "ide-shortcut-theme"

#include "ide-shortcut-private.h"
#include "ide-shortcut-theme.h"

typedef struct
{
  gchar      *name;
  gchar      *parent_name;
  gchar      *title;
  gchar      *subtitle;
  GHashTable *contexts;
  GHashTable *action_to_chord;
  GHashTable *command_to_chord;
} IdeShortcutThemePrivate;

enum {
  PROP_0,
  PROP_NAME,
  PROP_PARENT_NAME,
  PROP_SUBTITLE,
  PROP_TITLE,
  N_PROPS
};

G_DEFINE_TYPE_WITH_PRIVATE (IdeShortcutTheme, ide_shortcut_theme, G_TYPE_OBJECT)

static GParamSpec *properties [N_PROPS];

static void
ide_shortcut_theme_finalize (GObject *object)
{
  IdeShortcutTheme *self = (IdeShortcutTheme *)object;
  IdeShortcutThemePrivate *priv = ide_shortcut_theme_get_instance_private (self);

  g_clear_pointer (&priv->name, g_free);
  g_clear_pointer (&priv->parent_name, g_free);
  g_clear_pointer (&priv->title, g_free);
  g_clear_pointer (&priv->subtitle, g_free);
  g_clear_pointer (&priv->contexts, g_hash_table_unref);
  g_clear_pointer (&priv->action_to_chord, g_hash_table_unref);
  g_clear_pointer (&priv->command_to_chord, g_hash_table_unref);

  G_OBJECT_CLASS (ide_shortcut_theme_parent_class)->finalize (object);
}

static void
ide_shortcut_theme_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  IdeShortcutTheme *self = (IdeShortcutTheme *)object;

  switch (prop_id)
    {
    case PROP_NAME:
      g_value_set_string (value, ide_shortcut_theme_get_name (self));
      break;

    case PROP_PARENT_NAME:
      g_value_set_string (value, ide_shortcut_theme_get_parent_name (self));
      break;

    case PROP_TITLE:
      g_value_set_string (value, ide_shortcut_theme_get_title (self));
      break;

    case PROP_SUBTITLE:
      g_value_set_string (value, ide_shortcut_theme_get_subtitle (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ide_shortcut_theme_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  IdeShortcutTheme *self = (IdeShortcutTheme *)object;
  IdeShortcutThemePrivate *priv = ide_shortcut_theme_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_NAME:
      priv->name = g_value_dup_string (value);
      break;

    case PROP_PARENT_NAME:
      ide_shortcut_theme_set_parent_name (self, g_value_get_string (value));
      break;

    case PROP_TITLE:
      g_free (priv->title);
      priv->title = g_value_dup_string (value);
      break;

    case PROP_SUBTITLE:
      g_free (priv->subtitle);
      priv->subtitle = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ide_shortcut_theme_class_init (IdeShortcutThemeClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = ide_shortcut_theme_finalize;
  object_class->get_property = ide_shortcut_theme_get_property;
  object_class->set_property = ide_shortcut_theme_set_property;

  properties [PROP_NAME] =
    g_param_spec_string ("name",
                         "Name",
                         "The name of the theme",
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  properties [PROP_PARENT_NAME] =
    g_param_spec_string ("parent-name",
                         "Parent Name",
                         "The name of the parent shortcut theme",
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_TITLE] =
    g_param_spec_string ("title",
                         "Title",
                         "The title of the theme as used for UI elements",
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  properties [PROP_SUBTITLE] =
    g_param_spec_string ("subtitle",
                         "Subtitle",
                         "The subtitle of the theme as used for UI elements",
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
ide_shortcut_theme_init (IdeShortcutTheme *self)
{
  IdeShortcutThemePrivate *priv = ide_shortcut_theme_get_instance_private (self);

  priv->contexts = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
  priv->action_to_chord = g_hash_table_new_full (g_str_hash, g_str_equal, NULL,
                                                 (GDestroyNotify)ide_shortcut_chord_free);
  priv->command_to_chord = g_hash_table_new_full (g_str_hash, g_str_equal, NULL,
                                                  (GDestroyNotify)ide_shortcut_chord_free);
}

const gchar *
ide_shortcut_theme_get_name (IdeShortcutTheme *self)
{
  IdeShortcutThemePrivate *priv = ide_shortcut_theme_get_instance_private (self);

  g_return_val_if_fail (IDE_IS_SHORTCUT_THEME (self), NULL);

  return priv->name;
}

/**
 * ide_shortcut_theme_find_context_by_name:
 * @self: An #IdeShortcutContext
 * @name: The name of the context
 *
 * Gets the context named @name. If the context does not exist, it will
 * be created.
 *
 * Returns: (not nullable) (transfer none): An #IdeShortcutContext
 */
IdeShortcutContext *
ide_shortcut_theme_find_context_by_name (IdeShortcutTheme *self,
                                         const gchar      *name)
{
  IdeShortcutThemePrivate *priv = ide_shortcut_theme_get_instance_private (self);
  IdeShortcutContext *ret;

  g_return_val_if_fail (IDE_IS_SHORTCUT_THEME (self), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  if (NULL == (ret = g_hash_table_lookup (priv->contexts, name)))
    {
      ret = ide_shortcut_context_new (name);
      g_hash_table_insert (priv->contexts, g_strdup (name), ret);
    }

  return ret;
}

static IdeShortcutContext *
ide_shortcut_theme_find_default_context_by_type (IdeShortcutTheme *self,
                                                 GType             type)
{
  g_return_val_if_fail (IDE_IS_SHORTCUT_THEME (self), NULL);
  g_return_val_if_fail (g_type_is_a (type, GTK_TYPE_WIDGET), NULL);

  return ide_shortcut_theme_find_context_by_name (self, g_type_name (type));
}

/**
 * ide_shortcut_theme_find_default_context:
 *
 * Finds the default context in the theme for @widget.
 *
 * Returns: (nullable) (transfer none): An #IdeShortcutContext or %NULL.
 */
IdeShortcutContext *
ide_shortcut_theme_find_default_context (IdeShortcutTheme *self,
                                         GtkWidget        *widget)
{
  g_return_val_if_fail (IDE_IS_SHORTCUT_THEME (self), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

  return ide_shortcut_theme_find_default_context_by_type (self, G_OBJECT_TYPE (widget));
}

void
ide_shortcut_theme_add_context (IdeShortcutTheme   *self,
                                IdeShortcutContext *context)
{
  IdeShortcutThemePrivate *priv = ide_shortcut_theme_get_instance_private (self);
  const gchar *name;

  g_return_if_fail (IDE_IS_SHORTCUT_THEME (self));
  g_return_if_fail (IDE_IS_SHORTCUT_CONTEXT (context));

  name = ide_shortcut_context_get_name (context);

  g_return_if_fail (name != NULL);

  g_hash_table_insert (priv->contexts, g_strdup (name), g_object_ref (context));
}

IdeShortcutTheme *
ide_shortcut_theme_new (const gchar *name)
{
  return g_object_new (IDE_TYPE_SHORTCUT_THEME,
                       "name", name,
                       NULL);
}

const gchar *
ide_shortcut_theme_get_title (IdeShortcutTheme *self)
{
  IdeShortcutThemePrivate *priv = ide_shortcut_theme_get_instance_private (self);

  g_return_val_if_fail (IDE_IS_SHORTCUT_THEME (self), NULL);

  return priv->title;
}

const gchar *
ide_shortcut_theme_get_subtitle (IdeShortcutTheme *self)
{
  IdeShortcutThemePrivate *priv = ide_shortcut_theme_get_instance_private (self);

  g_return_val_if_fail (IDE_IS_SHORTCUT_THEME (self), NULL);

  return priv->subtitle;
}

GHashTable *
_ide_shortcut_theme_get_contexts (IdeShortcutTheme *self)
{
  IdeShortcutThemePrivate *priv = ide_shortcut_theme_get_instance_private (self);

  g_return_val_if_fail (IDE_IS_SHORTCUT_THEME (self), NULL);

  return priv->contexts;
}

const IdeShortcutChord *
ide_shortcut_theme_get_chord_for_action (IdeShortcutTheme *self,
                                         const gchar      *detailed_action_name)
{
  IdeShortcutThemePrivate *priv = ide_shortcut_theme_get_instance_private (self);

  g_return_val_if_fail (IDE_IS_SHORTCUT_THEME (self), NULL);
  g_return_val_if_fail (detailed_action_name != NULL, NULL);

  return g_hash_table_lookup (priv->action_to_chord, detailed_action_name);
}

void
ide_shortcut_theme_set_chord_for_action (IdeShortcutTheme       *self,
                                         const gchar            *detailed_action_name,
                                         const IdeShortcutChord *chord)
{
  IdeShortcutThemePrivate *priv = ide_shortcut_theme_get_instance_private (self);

  g_return_if_fail (IDE_IS_SHORTCUT_THEME (self));
  g_return_if_fail (detailed_action_name != NULL);

  detailed_action_name = g_intern_string (detailed_action_name);

  if (chord == NULL)
    g_hash_table_remove (priv->action_to_chord, detailed_action_name);
  else
    g_hash_table_insert (priv->action_to_chord,
                         (gchar *)detailed_action_name,
                         ide_shortcut_chord_copy (chord));
}

void
ide_shortcut_theme_set_accel_for_action (IdeShortcutTheme *self,
                                         const gchar      *detailed_action_name,
                                         const gchar      *accel)
{
  g_return_if_fail (IDE_IS_SHORTCUT_THEME (self));
  g_return_if_fail (detailed_action_name != NULL);

  if (accel == NULL)
    {
      ide_shortcut_theme_set_chord_for_action (self, detailed_action_name, NULL);
    }
  else
    {
      g_autoptr(IdeShortcutChord) chord = NULL;

      chord = ide_shortcut_chord_new_from_string (accel);
      ide_shortcut_theme_set_chord_for_action (self, detailed_action_name, chord);
    }
}

const IdeShortcutChord *
ide_shortcut_theme_get_chord_for_command (IdeShortcutTheme *self,
                                          const gchar      *detailed_command_name)
{
  IdeShortcutThemePrivate *priv = ide_shortcut_theme_get_instance_private (self);

  g_return_val_if_fail (IDE_IS_SHORTCUT_THEME (self), NULL);
  g_return_val_if_fail (detailed_command_name != NULL, NULL);

  return g_hash_table_lookup (priv->command_to_chord, detailed_command_name);
}

void
ide_shortcut_theme_set_chord_for_command (IdeShortcutTheme       *self,
                                          const gchar            *detailed_command_name,
                                          const IdeShortcutChord *chord)
{
  IdeShortcutThemePrivate *priv = ide_shortcut_theme_get_instance_private (self);

  g_return_if_fail (IDE_IS_SHORTCUT_THEME (self));
  g_return_if_fail (detailed_command_name != NULL);

  detailed_command_name = g_intern_string (detailed_command_name);

  if (chord == NULL)
    g_hash_table_remove (priv->command_to_chord, detailed_command_name);
  else
    g_hash_table_insert (priv->command_to_chord,
                         (gchar *)detailed_command_name,
                         ide_shortcut_chord_copy (chord));
}

void
ide_shortcut_theme_set_accel_for_command (IdeShortcutTheme *self,
                                          const gchar      *detailed_command_name,
                                          const gchar      *accel)
{
  g_return_if_fail (IDE_IS_SHORTCUT_THEME (self));
  g_return_if_fail (detailed_command_name != NULL);

  if (accel == NULL)
    {
      ide_shortcut_theme_set_chord_for_command (self, detailed_command_name, NULL);
    }
  else
    {
      g_autoptr(IdeShortcutChord) chord = NULL;

      chord = ide_shortcut_chord_new_from_string (accel);
      ide_shortcut_theme_set_chord_for_command (self, detailed_command_name, chord);
    }
}

void
_ide_shortcut_theme_set_name (IdeShortcutTheme *self,
                              const gchar      *name)
{
  IdeShortcutThemePrivate *priv = ide_shortcut_theme_get_instance_private (self);

  g_return_if_fail (IDE_IS_SHORTCUT_THEME (self));

  if (g_strcmp0 (name, priv->name) != 0)
    {
      g_free (priv->name);
      priv->name = g_strdup (name);
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_NAME]);
    }
}

/**
 * ide_shortcut_theme_get_parent_name:
 * @self: a #IdeShortcutTheme
 *
 * Gets the name of the parent shortcut theme.
 *
 * This is used to resolve shortcuts from the parent theme without having to
 * copy them directly into this shortcut theme. It allows for some level of
 * copy-on-write (CoW).
 *
 * Returns: (nullable): The name of the parent theme, or %NULL if none is set.
 */
const gchar *
ide_shortcut_theme_get_parent_name (IdeShortcutTheme *self)
{
  IdeShortcutThemePrivate *priv = ide_shortcut_theme_get_instance_private (self);

  g_return_val_if_fail (IDE_IS_SHORTCUT_THEME (self), NULL);

  return priv->parent_name;
}

void
ide_shortcut_theme_set_parent_name (IdeShortcutTheme *self,
                                    const gchar      *parent_name)
{
  IdeShortcutThemePrivate *priv = ide_shortcut_theme_get_instance_private (self);

  g_return_if_fail (IDE_IS_SHORTCUT_THEME (self));

  if (g_strcmp0 (parent_name, priv->parent_name) != 0)
    {
      g_free (priv->parent_name);
      priv->parent_name = g_strdup (parent_name);
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_PARENT_NAME]);
    }
}
