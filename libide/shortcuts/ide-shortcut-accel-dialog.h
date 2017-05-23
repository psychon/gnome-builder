/* ide-shortcut-accel-dialog.h
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

#ifndef IDE_SHORTCUT_ACCEL_DIALOG_H
#define IDE_SHORTCUT_ACCEL_DIALOG_H

#include <gtk/gtk.h>

#include "ide-shortcut-chord.h"

G_BEGIN_DECLS

#define IDE_TYPE_SHORTCUT_ACCEL_DIALOG (ide_shortcut_accel_dialog_get_type())

G_DECLARE_FINAL_TYPE (IdeShortcutAccelDialog, ide_shortcut_accel_dialog, IDE, SHORTCUT_ACCEL_DIALOG, GtkDialog)

GtkWidget              *ide_shortcut_accel_dialog_new                (void);
gchar                  *ide_shortcut_accel_dialog_get_accelerator    (IdeShortcutAccelDialog *self);
void                    ide_shortcut_accel_dialog_set_accelerator    (IdeShortcutAccelDialog *self,
                                                                      const gchar            *accelerator);
const IdeShortcutChord *ide_shortcut_accel_dialog_get_chord          (IdeShortcutAccelDialog *self);
const gchar            *ide_shortcut_accel_dialog_get_shortcut_title (IdeShortcutAccelDialog *self);
void                    ide_shortcut_accel_dialog_set_shortcut_title (IdeShortcutAccelDialog *self,
                                                                      const gchar            *title);

G_END_DECLS

#endif /* IDE_SHORTCUT_ACCEL_DIALOG_H */
