/* ide-shortcut-label.h
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

#ifndef IDE_SHORTCUT_LABEL_H
#define IDE_SHORTCUT_LABEL_H

#include <gtk/gtk.h>

#include "ide-shortcut-chord.h"

G_BEGIN_DECLS

#define IDE_TYPE_SHORTCUT_LABEL (ide_shortcut_label_get_type())

G_DECLARE_FINAL_TYPE (IdeShortcutLabel, ide_shortcut_label, IDE, SHORTCUT_LABEL, GtkBox)

GtkWidget              *ide_shortcut_label_new             (void);
gchar                  *ide_shortcut_label_get_accelerator (IdeShortcutLabel       *self);
void                    ide_shortcut_label_set_accelerator (IdeShortcutLabel       *self,
                                                            const gchar            *accelerator);
void                    ide_shortcut_label_set_chord       (IdeShortcutLabel       *self,
                                                            const IdeShortcutChord *chord);
const IdeShortcutChord *ide_shortcut_label_get_chord       (IdeShortcutLabel       *self);

G_END_DECLS

#endif /* IDE_SHORTCUT_LABEL_H */
