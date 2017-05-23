/* ide-shortcut-theme-private.h
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

#ifndef IDE_SHORTCUT_THEME_PRIVATE_H
#define IDE_SHORTCUT_THEME_PRIVATE_H

#include "ide-shortcut-chord.h"
#include "ide-shortcut-manager.h"
#include "ide-shortcut-theme.h"

G_BEGIN_DECLS

typedef struct
{
  IdeShortcutChordTable *table;
  guint                  position;
} IdeShortcutChordTableIter;

typedef enum
{
  IDE_SHORTCUT_NODE_SECTION = 1,
  IDE_SHORTCUT_NODE_GROUP,
  IDE_SHORTCUT_NODE_ACTION,
  IDE_SHORTCUT_NODE_COMMAND,
} IdeShortcutNodeType;

typedef struct
{
  IdeShortcutNodeType  type;
  const gchar         *name;
  const gchar         *title;
  const gchar         *subtitle;
} IdeShortcutNodeData;

typedef enum
{
  SHORTCUT_ACTION = 1,
  SHORTCUT_SIGNAL,
} ShortcutType;

typedef struct _Shortcut
{
  ShortcutType      type;
  union {
    struct {
      const gchar  *prefix;
      const gchar  *name;
      GVariant     *param;
    } action;
    struct {
      const gchar  *name;
      GQuark        detail;
      GArray       *params;
    } signal;
  };
  struct _Shortcut *next;
} Shortcut;

typedef enum
{
  IDE_SHORTCUT_MODEL_COLUMN_TYPE,
  IDE_SHORTCUT_MODEL_COLUMN_ID,
  IDE_SHORTCUT_MODEL_COLUMN_TITLE,
  IDE_SHORTCUT_MODEL_COLUMN_ACCEL,
  IDE_SHORTCUT_MODEL_COLUMN_KEYWORDS,
  IDE_SHORTCUT_MODEL_COLUMN_CHORD,
  IDE_SHORTCUT_MODEL_N_COLUMNS
} IdeShortcutModelColumn;

GNode                 *_ide_shortcut_manager_get_root      (IdeShortcutManager         *self);
GtkTreeModel          *_ide_shortcut_theme_create_model    (IdeShortcutTheme           *self);
GHashTable            *_ide_shortcut_theme_get_contexts    (IdeShortcutTheme           *self);
IdeShortcutChordTable *_ide_shortcut_context_get_table     (IdeShortcutContext         *self);
void                   _ide_shortcut_chord_table_iter_init (IdeShortcutChordTableIter  *iter,
                                                            IdeShortcutChordTable      *table);
gboolean               _ide_shortcut_chord_table_iter_next (IdeShortcutChordTableIter  *iter,
                                                            const IdeShortcutChord    **chord,
                                                            gpointer                   *value);

G_END_DECLS

#endif /* IDE_SHORTCUT_THEME_PRIVATE_H */
