/* ide-shortcut-label.c
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

#define G_LOG_DOMAIN "ide-shortcut-label"

#include "ide-shortcut-label.h"

struct _IdeShortcutLabel
{
  GtkBox            parent_instance;
  IdeShortcutChord *chord;
};

enum {
  PROP_0,
  PROP_ACCELERATOR,
  PROP_CHORD,
  N_PROPS
};

G_DEFINE_TYPE (IdeShortcutLabel, ide_shortcut_label, GTK_TYPE_BOX)

static GParamSpec *properties [N_PROPS];

static void
ide_shortcut_label_finalize (GObject *object)
{
  IdeShortcutLabel *self = (IdeShortcutLabel *)object;

  g_clear_pointer (&self->chord, ide_shortcut_chord_free);

  G_OBJECT_CLASS (ide_shortcut_label_parent_class)->finalize (object);
}

static void
ide_shortcut_label_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  IdeShortcutLabel *self = IDE_SHORTCUT_LABEL (object);

  switch (prop_id)
    {
    case PROP_ACCELERATOR:
      g_value_take_string (value, ide_shortcut_label_get_accelerator (self));
      break;

    case PROP_CHORD:
      g_value_set_boxed (value, ide_shortcut_label_get_chord (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ide_shortcut_label_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  IdeShortcutLabel *self = IDE_SHORTCUT_LABEL (object);

  switch (prop_id)
    {
    case PROP_ACCELERATOR:
      ide_shortcut_label_set_accelerator (self, g_value_get_string (value));
      break;

    case PROP_CHORD:
      ide_shortcut_label_set_chord (self, g_value_get_boxed (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ide_shortcut_label_class_init (IdeShortcutLabelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = ide_shortcut_label_finalize;
  object_class->get_property = ide_shortcut_label_get_property;
  object_class->set_property = ide_shortcut_label_set_property;

  properties [PROP_ACCELERATOR] =
    g_param_spec_string ("accelerator",
                         "Accelerator",
                         "The accelerator for the label",
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_CHORD] =
    g_param_spec_boxed ("chord",
                         "Chord",
                         "The chord for the label",
                         IDE_TYPE_SHORTCUT_CHORD,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
ide_shortcut_label_init (IdeShortcutLabel *self)
{
  gtk_box_set_spacing (GTK_BOX (self), 12);
}

GtkWidget *
ide_shortcut_label_new (void)
{
  return g_object_new (IDE_TYPE_SHORTCUT_LABEL, NULL);
}

gchar *
ide_shortcut_label_get_accelerator (IdeShortcutLabel *self)
{
  g_return_val_if_fail (IDE_IS_SHORTCUT_LABEL (self), NULL);

  if (self->chord == NULL)
    return NULL;

  return ide_shortcut_chord_to_string (self->chord);
}

void
ide_shortcut_label_set_accelerator (IdeShortcutLabel *self,
                                    const gchar      *accelerator)
{
  g_autoptr(IdeShortcutChord) chord = NULL;

  g_return_if_fail (IDE_IS_SHORTCUT_LABEL (self));

  if (accelerator != NULL)
    chord = ide_shortcut_chord_new_from_string (accelerator);

  ide_shortcut_label_set_chord (self, chord);
}

void
ide_shortcut_label_set_chord (IdeShortcutLabel       *self,
                              const IdeShortcutChord *chord)
{
  if (!ide_shortcut_chord_equal (chord, self->chord))
    {
      g_autofree gchar *accel = NULL;

      ide_shortcut_chord_free (self->chord);
      self->chord = ide_shortcut_chord_copy (chord);

      if (self->chord != NULL)
        accel = ide_shortcut_chord_to_string (self->chord);

      gtk_container_foreach (GTK_CONTAINER (self),
                             (GtkCallback) gtk_widget_destroy,
                             NULL);

      if (accel != NULL)
        {
          g_auto(GStrv) parts = NULL;

          parts = g_strsplit (accel, "|", 0);

          for (guint i = 0; parts[i]; i++)
            {
              GtkWidget *label;

              label = g_object_new (GTK_TYPE_SHORTCUT_LABEL,
                                    "accelerator", parts[i],
                                    "visible", TRUE,
                                    NULL);
              gtk_container_add (GTK_CONTAINER (self), label);
            }
        }

      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_ACCELERATOR]);
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_CHORD]);
    }
}

/**
 * ide_shortcut_label_get_chord:
 * @self: a #IdeShortcutLabel
 *
 * Gets the chord for the label, or %NULL.
 *
 * Returns: (transfer none) (nullable): A #IdeShortcutChord or %NULL
 */
const IdeShortcutChord *
ide_shortcut_label_get_chord (IdeShortcutLabel *self)
{
  return self->chord;
}
