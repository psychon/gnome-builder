/* ide-shortcut-chord.h
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

#ifndef IDE_SHORTCUT_CHORD_H
#define IDE_SHORTCUT_CHORD_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef enum
{
  IDE_SHORTCUT_MATCH_NONE,
  IDE_SHORTCUT_MATCH_EQUAL,
  IDE_SHORTCUT_MATCH_PARTIAL
} IdeShortcutMatch;

#define IDE_TYPE_SHORTCUT_CHORD       (ide_shortcut_chord_get_type())
#define IDE_TYPE_SHORTCUT_CHORD_TABLE (ide_shortcut_chord_table_get_type())
#define IDE_TYPE_SHORTCUT_MATCH       (ide_shortcut_match_get_type())

typedef struct _IdeShortcutChord      IdeShortcutChord;
typedef struct _IdeShortcutChordTable IdeShortcutChordTable;

GType                  ide_shortcut_chord_get_type            (void);
IdeShortcutChord      *ide_shortcut_chord_new_from_event      (const GdkEventKey           *event);
IdeShortcutChord      *ide_shortcut_chord_new_from_string     (const gchar                 *accelerator);
gchar                 *ide_shortcut_chord_to_string           (const IdeShortcutChord      *self);
gchar                 *ide_shortcut_chord_get_label           (const IdeShortcutChord      *self);
guint                  ide_shortcut_chord_get_length          (const IdeShortcutChord      *self);
void                   ide_shortcut_chord_get_nth_key         (const IdeShortcutChord      *self,
                                                               guint                        nth,
                                                               guint                       *keyval,
                                                               GdkModifierType             *modifier);
gboolean               ide_shortcut_chord_has_modifier        (const IdeShortcutChord      *self);
gboolean               ide_shortcut_chord_append_event        (IdeShortcutChord            *self,
                                                               const GdkEventKey           *event);
IdeShortcutMatch       ide_shortcut_chord_match               (const IdeShortcutChord      *self,
                                                               const IdeShortcutChord      *other);
guint                  ide_shortcut_chord_hash                (gconstpointer                data);
gboolean               ide_shortcut_chord_equal               (gconstpointer                data1,
                                                               gconstpointer                data2);
IdeShortcutChord      *ide_shortcut_chord_copy                (const IdeShortcutChord      *self);
void                   ide_shortcut_chord_free                (IdeShortcutChord            *self);
GType                  ide_shortcut_chord_table_get_type      (void);
IdeShortcutChordTable *ide_shortcut_chord_table_new           (void);
void                   ide_shortcut_chord_table_set_free_func (IdeShortcutChordTable       *self,
                                                               GDestroyNotify               notify);
void                   ide_shortcut_chord_table_free          (IdeShortcutChordTable       *self);
void                   ide_shortcut_chord_table_add           (IdeShortcutChordTable       *self,
                                                               const IdeShortcutChord      *chord,
                                                               gpointer                     data);
gboolean               ide_shortcut_chord_table_remove        (IdeShortcutChordTable       *self,
                                                               const IdeShortcutChord      *chord);
IdeShortcutMatch       ide_shortcut_chord_table_lookup        (IdeShortcutChordTable       *self,
                                                               const IdeShortcutChord      *chord,
                                                               gpointer                    *data);
guint                  ide_shortcut_chord_table_size          (const IdeShortcutChordTable *self);
void                   ide_shortcut_chord_table_printf        (const IdeShortcutChordTable *self);
GType                  ide_shortcut_match_get_type            (void);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (IdeShortcutChord, ide_shortcut_chord_free)

G_END_DECLS

#endif /* IDE_SHORTCUT_CHORD_H */
