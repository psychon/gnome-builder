/* ide-shortcut-model.h
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

#ifndef IDE_SHORTCUT_MODEL_H
#define IDE_SHORTCUT_MODEL_H

#include <gtk/gtk.h>

#include "ide-shortcut-chord.h"
#include "ide-shortcut-manager.h"
#include "ide-shortcut-theme.h"

G_BEGIN_DECLS

#define IDE_TYPE_SHORTCUT_MODEL (ide_shortcut_model_get_type())

G_DECLARE_FINAL_TYPE (IdeShortcutModel, ide_shortcut_model, IDE, SHORTCUT_MODEL, GtkTreeStore)

GtkTreeModel       *ide_shortcut_model_new         (void);
IdeShortcutManager *ide_shortcut_model_get_manager (IdeShortcutModel       *self);
void                ide_shortcut_model_set_manager (IdeShortcutModel       *self,
                                                    IdeShortcutManager     *manager);
IdeShortcutTheme   *ide_shortcut_model_get_theme   (IdeShortcutModel       *self);
void                ide_shortcut_model_set_theme   (IdeShortcutModel       *self,
                                                    IdeShortcutTheme       *theme);
void                ide_shortcut_model_set_chord   (IdeShortcutModel       *self,
                                                    GtkTreeIter            *iter,
                                                    const IdeShortcutChord *chord);
void                ide_shortcut_model_rebuild     (IdeShortcutModel       *self);

G_END_DECLS

#endif /* IDE_SHORTCUT_MODEL_H */
