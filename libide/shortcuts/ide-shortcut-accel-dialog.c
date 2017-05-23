/* ide-shortcut-accel-dialog.c
 *
 * Copyright (C) 2016 Endless, Inc
 *           (C) 2017 Christian Hergert
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
 *
 * Authors: Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
 *          Christian Hergert <chergert@redhat.com>
 */

#define G_LOG_DOMAIN "ide-shortcut-accel-dialog"

#include <glib/gi18n.h>

#include "ide-shortcut-accel-dialog.h"
#include "ide-shortcut-chord.h"
#include "ide-shortcut-label.h"

struct _IdeShortcutAccelDialog
{
  GtkDialog             parent_instance;

  GtkStack             *stack;
  GtkLabel             *display_label;
  IdeShortcutLabel     *display_shortcut;
  GtkLabel             *selection_label;
  GtkButton            *button_cancel;
  GtkButton            *button_set;

  GdkDevice            *grab_pointer;

  gchar                *shortcut_title;
  IdeShortcutChord     *chord;

  gulong                grab_source;

  guint                 first_modifier;
};

enum {
  PROP_0,
  PROP_ACCELERATOR,
  PROP_SHORTCUT_TITLE,
  N_PROPS
};

G_DEFINE_TYPE (IdeShortcutAccelDialog, ide_shortcut_accel_dialog, GTK_TYPE_DIALOG)

static GParamSpec *properties [N_PROPS];

/*
 * ide_shortcut_accel_dialog_begin_grab:
 *
 * This function returns %G_SOURCE_REMOVE so that it may be used as
 * a GSourceFunc when necessary.
 *
 * Returns: %G_SOURCE_REMOVE always.
 */
static gboolean
ide_shortcut_accel_dialog_begin_grab (IdeShortcutAccelDialog *self)
{
  g_autoptr(GList) seats = NULL;
  GdkWindow *window;
  GdkDisplay *display;
  GdkSeat *first_seat;
  GdkDevice *device;
  GdkDevice *pointer;
  GdkGrabStatus status;

  g_assert (IDE_IS_SHORTCUT_ACCEL_DIALOG (self));

  self->grab_source = 0;

  if (!gtk_widget_get_mapped (GTK_WIDGET (self)))
    return G_SOURCE_REMOVE;

  if (NULL == (window = gtk_widget_get_window (GTK_WIDGET (self))))
    return G_SOURCE_REMOVE;

  display = gtk_widget_get_display (GTK_WIDGET (self));

  if (NULL == (seats = gdk_display_list_seats (display)))
    return G_SOURCE_REMOVE;

  first_seat = seats->data;
  device = gdk_seat_get_keyboard (first_seat);

  if (device == NULL)
    {
      g_warning ("Keyboard grab unsuccessful, no keyboard in seat");
      return G_SOURCE_REMOVE;
    }

  if (gdk_device_get_source (device) == GDK_SOURCE_KEYBOARD)
    pointer = gdk_device_get_associated_device (device);
  else
    pointer = device;

  status = gdk_seat_grab (gdk_device_get_seat (pointer),
                          window,
                          GDK_SEAT_CAPABILITY_KEYBOARD,
                          FALSE,
                          NULL,
                          NULL,
                          NULL,
                          NULL);

  if (status != GDK_GRAB_SUCCESS)
    return G_SOURCE_REMOVE;

  self->grab_pointer = pointer;

  g_debug ("Grab started on %s with device %s",
           G_OBJECT_TYPE_NAME (self),
           G_OBJECT_TYPE_NAME (device));

  gtk_grab_add (GTK_WIDGET (self));

  return G_SOURCE_REMOVE;
}

static void
ide_shortcut_accel_dialog_release_grab (IdeShortcutAccelDialog *self)
{
  g_assert (IDE_IS_SHORTCUT_ACCEL_DIALOG (self));

  if (self->grab_pointer != NULL)
    {
      gdk_seat_ungrab (gdk_device_get_seat (self->grab_pointer));
      self->grab_pointer = NULL;
      gtk_grab_remove (GTK_WIDGET (self));
    }
}

static void
ide_shortcut_accel_dialog_map (GtkWidget *widget)
{
  IdeShortcutAccelDialog *self = (IdeShortcutAccelDialog *)widget;

  g_assert (IDE_IS_SHORTCUT_ACCEL_DIALOG (self));

  GTK_WIDGET_CLASS (ide_shortcut_accel_dialog_parent_class)->map (widget);

  self->grab_source =
    g_timeout_add_full (G_PRIORITY_LOW,
                        100,
                        (GSourceFunc) ide_shortcut_accel_dialog_begin_grab,
                        g_object_ref (self),
                        g_object_unref);
}

static void
ide_shortcut_accel_dialog_unmap (GtkWidget *widget)
{
  IdeShortcutAccelDialog *self = (IdeShortcutAccelDialog *)widget;

  g_assert (IDE_IS_SHORTCUT_ACCEL_DIALOG (self));

  ide_shortcut_accel_dialog_release_grab (self);

  GTK_WIDGET_CLASS (ide_shortcut_accel_dialog_parent_class)->unmap (widget);
}

static gboolean
ide_shortcut_accel_dialog_is_editing (IdeShortcutAccelDialog *self)
{
  g_assert (IDE_IS_SHORTCUT_ACCEL_DIALOG (self));

  return self->grab_pointer != NULL;
}

static void
ide_shortcut_accel_dialog_apply_state (IdeShortcutAccelDialog *self)
{
  g_assert (IDE_IS_SHORTCUT_ACCEL_DIALOG (self));

  if (self->chord != NULL)
    {
      gtk_stack_set_visible_child_name (self->stack, "display");
      gtk_dialog_set_response_sensitive (GTK_DIALOG (self), GTK_RESPONSE_ACCEPT, TRUE);
    }
  else
    {
      gtk_stack_set_visible_child_name (self->stack, "selection");
      gtk_dialog_set_response_sensitive (GTK_DIALOG (self), GTK_RESPONSE_ACCEPT, FALSE);
    }
}

static gboolean
ide_shortcut_accel_dialog_key_press_event (GtkWidget   *widget,
                                           GdkEventKey *key)
{
  IdeShortcutAccelDialog *self = (IdeShortcutAccelDialog *)widget;

  g_assert (IDE_IS_SHORTCUT_ACCEL_DIALOG (self));
  g_assert (key != NULL);

  if (ide_shortcut_accel_dialog_is_editing (self))
    {
      GdkModifierType real_mask;
      guint keyval_lower;

      if (key->is_modifier)
        {
          /*
           * If we are just starting a chord, we need to stash the modifier
           * so that we know when we have finished the sequence.
           */
          if (self->chord == NULL && self->first_modifier == 0)
            self->first_modifier = key->keyval;

          goto chain_up;
        }

      real_mask = key->state & gtk_accelerator_get_default_mod_mask ();
      keyval_lower = gdk_keyval_to_lower (key->keyval);

      /* Normalize <Tab> */
      if (keyval_lower == GDK_KEY_ISO_Left_Tab)
        keyval_lower = GDK_KEY_Tab;

      /* Put shift back if it changed the case of the key */
      if (keyval_lower != key->keyval)
        real_mask |= GDK_SHIFT_MASK;

      /* We don't want to use SysRq as a keybinding but we do
       * want Alt+Print), so we avoid translation from Alt+Print to SysRq
       */
      if (keyval_lower == GDK_KEY_Sys_Req && (real_mask & GDK_MOD1_MASK) != 0)
        keyval_lower = GDK_KEY_Print;

      /* A single Escape press cancels the editing */
      if (!key->is_modifier && real_mask == 0 && keyval_lower == GDK_KEY_Escape)
        {
          ide_shortcut_accel_dialog_release_grab (self);
          gtk_dialog_response (GTK_DIALOG (self), GTK_RESPONSE_CANCEL);
          return GDK_EVENT_STOP;
        }

      /* Backspace disables the current shortcut */
      if (real_mask == 0 && keyval_lower == GDK_KEY_BackSpace)
        {
          ide_shortcut_accel_dialog_set_accelerator (self, NULL);
          gtk_dialog_response (GTK_DIALOG (self), GTK_RESPONSE_ACCEPT);
          return GDK_EVENT_STOP;
        }

      if (self->chord == NULL)
        self->chord = ide_shortcut_chord_new_from_event (key);
      else
        ide_shortcut_chord_append_event (self->chord, key);

      ide_shortcut_accel_dialog_apply_state (self);

      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_ACCELERATOR]);

      return GDK_EVENT_STOP;
    }

chain_up:
  return GTK_WIDGET_CLASS (ide_shortcut_accel_dialog_parent_class)->key_press_event (widget, key);
}

static gboolean
ide_shortcut_accel_dialog_key_release_event (GtkWidget   *widget,
                                             GdkEventKey *key)
{
  IdeShortcutAccelDialog *self = (IdeShortcutAccelDialog *)widget;

  g_assert (IDE_IS_SHORTCUT_ACCEL_DIALOG (self));
  g_assert (key != NULL);

  if (self->chord != NULL)
    {
      /*
       * If we have a chord defined and there was no modifier,
       * then any key release should be enough for us to cancel
       * our grab.
       */
      if (!ide_shortcut_chord_has_modifier (self->chord))
        {
          ide_shortcut_accel_dialog_release_grab (self);
          goto chain_up;
        }

      /*
       * If we started our sequence with a modifier, we want to
       * release our grab when that modifier has been released.
       */
      if (key->is_modifier &&
          self->first_modifier != 0 &&
          self->first_modifier == key->keyval)
        {
          self->first_modifier = 0;
          ide_shortcut_accel_dialog_release_grab (self);
          goto chain_up;
        }
    }

  /* Clear modifier if it was released before a chord was made */
  if (self->first_modifier == key->keyval)
    self->first_modifier = 0;

chain_up:
  return GTK_WIDGET_CLASS (ide_shortcut_accel_dialog_parent_class)->key_release_event (widget, key);
}

static void
ide_shortcut_accel_dialog_destroy (GtkWidget *widget)
{
  IdeShortcutAccelDialog *self = (IdeShortcutAccelDialog *)widget;

  g_assert (IDE_IS_SHORTCUT_ACCEL_DIALOG (self));

  if (self->grab_source != 0)
    {
      g_source_remove (self->grab_source);
      self->grab_source = 0;
    }

  GTK_WIDGET_CLASS (ide_shortcut_accel_dialog_parent_class)->destroy (widget);
}

static void
ide_shortcut_accel_dialog_finalize (GObject *object)
{
  IdeShortcutAccelDialog *self = (IdeShortcutAccelDialog *)object;

  g_clear_pointer (&self->shortcut_title, g_free);
  g_clear_pointer (&self->chord, ide_shortcut_chord_free);

  G_OBJECT_CLASS (ide_shortcut_accel_dialog_parent_class)->finalize (object);
}

static void
ide_shortcut_accel_dialog_get_property (GObject    *object,
                                        guint       prop_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  IdeShortcutAccelDialog *self = IDE_SHORTCUT_ACCEL_DIALOG (object);

  switch (prop_id)
    {
    case PROP_ACCELERATOR:
      g_value_take_string (value, ide_shortcut_accel_dialog_get_accelerator (self));
      break;

    case PROP_SHORTCUT_TITLE:
      g_value_set_string (value, ide_shortcut_accel_dialog_get_shortcut_title (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ide_shortcut_accel_dialog_set_property (GObject      *object,
                                        guint         prop_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  IdeShortcutAccelDialog *self = IDE_SHORTCUT_ACCEL_DIALOG (object);

  switch (prop_id)
    {
    case PROP_ACCELERATOR:
      ide_shortcut_accel_dialog_set_accelerator (self, g_value_get_string (value));
      break;

    case PROP_SHORTCUT_TITLE:
      ide_shortcut_accel_dialog_set_shortcut_title (self, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ide_shortcut_accel_dialog_class_init (IdeShortcutAccelDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = ide_shortcut_accel_dialog_finalize;
  object_class->get_property = ide_shortcut_accel_dialog_get_property;
  object_class->set_property = ide_shortcut_accel_dialog_set_property;

  widget_class->destroy = ide_shortcut_accel_dialog_destroy;
  widget_class->map = ide_shortcut_accel_dialog_map;
  widget_class->unmap = ide_shortcut_accel_dialog_unmap;
  widget_class->key_press_event = ide_shortcut_accel_dialog_key_press_event;
  widget_class->key_release_event = ide_shortcut_accel_dialog_key_release_event;

  properties [PROP_ACCELERATOR] =
    g_param_spec_string ("accelerator",
                         "Accelerator",
                         "Accelerator",
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  properties [PROP_SHORTCUT_TITLE] =
    g_param_spec_string ("shortcut-title",
                         "Title",
                         "Title",
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/builder/ui/ide-shortcut-accel-dialog.ui");

  gtk_widget_class_bind_template_child (widget_class, IdeShortcutAccelDialog, stack);
  gtk_widget_class_bind_template_child (widget_class, IdeShortcutAccelDialog, selection_label);
  gtk_widget_class_bind_template_child (widget_class, IdeShortcutAccelDialog, display_label);
  gtk_widget_class_bind_template_child (widget_class, IdeShortcutAccelDialog, display_shortcut);
  gtk_widget_class_bind_template_child (widget_class, IdeShortcutAccelDialog, button_cancel);
  gtk_widget_class_bind_template_child (widget_class, IdeShortcutAccelDialog, button_set);

  g_type_ensure (IDE_TYPE_SHORTCUT_LABEL);
}

static void
ide_shortcut_accel_dialog_init (IdeShortcutAccelDialog *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_dialog_add_buttons (GTK_DIALOG (self),
                          _("Cancel"), GTK_RESPONSE_CANCEL,
                          _("Set"), GTK_RESPONSE_ACCEPT,
                          NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_ACCEPT);

  gtk_dialog_set_response_sensitive (GTK_DIALOG (self), GTK_RESPONSE_ACCEPT, FALSE);

  g_object_bind_property (self, "accelerator",
                          self->display_shortcut, "accelerator",
                          G_BINDING_SYNC_CREATE);
}

gchar *
ide_shortcut_accel_dialog_get_accelerator (IdeShortcutAccelDialog *self)
{
  g_return_val_if_fail (IDE_IS_SHORTCUT_ACCEL_DIALOG (self), NULL);

  if (self->chord == NULL)
    return NULL;

  return ide_shortcut_chord_to_string (self->chord);
}

void
ide_shortcut_accel_dialog_set_accelerator (IdeShortcutAccelDialog *self,
                                           const gchar            *accelerator)
{
  g_autoptr(IdeShortcutChord) chord = NULL;

  g_return_if_fail (IDE_IS_SHORTCUT_ACCEL_DIALOG (self));

  if (accelerator)
    chord = ide_shortcut_chord_new_from_string (accelerator);

  if (!ide_shortcut_chord_equal (chord, self->chord))
    {
      ide_shortcut_chord_free (self->chord);
      self->chord = g_steal_pointer (&chord);
      gtk_dialog_set_response_sensitive (GTK_DIALOG (self),
                                         GTK_RESPONSE_ACCEPT,
                                         self->chord != NULL);
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_ACCELERATOR]);
    }
}

void
ide_shortcut_accel_dialog_set_shortcut_title (IdeShortcutAccelDialog *self,
                                              const gchar            *shortcut_title)
{
  g_return_if_fail (IDE_IS_SHORTCUT_ACCEL_DIALOG (self));

  if (g_strcmp0 (shortcut_title, self->shortcut_title) != 0)
    {
      g_autofree gchar *label = NULL;

      if (shortcut_title != NULL)
        {
          /* Translators: <b>%s</b> is used to show the provided text in bold */
          label = g_strdup_printf (_("Enter new shortcut to change <b>%s</b>."), shortcut_title);
        }

      gtk_label_set_label (self->selection_label, label);
      gtk_label_set_label (self->display_label, label);

      g_free (self->shortcut_title);
      self->shortcut_title = g_strdup (shortcut_title);

      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_SHORTCUT_TITLE]);
    }
}

const gchar *
ide_shortcut_accel_dialog_get_shortcut_title (IdeShortcutAccelDialog *self)
{
  g_return_val_if_fail (IDE_IS_SHORTCUT_ACCEL_DIALOG (self), NULL);

  return self->shortcut_title;
}

const IdeShortcutChord *
ide_shortcut_accel_dialog_get_chord (IdeShortcutAccelDialog *self)
{
  g_return_val_if_fail (IDE_IS_SHORTCUT_ACCEL_DIALOG (self), NULL);

  return self->chord;
}
