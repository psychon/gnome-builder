/* ide-shortcut-theme-editor.h
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

#ifndef IDE_SHORTCUT_THEME_EDITOR_H
#define IDE_SHORTCUT_THEME_EDITOR_H

#include <gtk/gtk.h>

#include "ide-shortcut-theme.h"

G_BEGIN_DECLS

#define IDE_TYPE_SHORTCUT_THEME_EDITOR (ide_shortcut_theme_editor_get_type())

G_DECLARE_DERIVABLE_TYPE (IdeShortcutThemeEditor, ide_shortcut_theme_editor, IDE, SHORTCUT_THEME_EDITOR, GtkBin)

struct _IdeShortcutThemeEditorClass
{
  GtkBinClass parent_class;

  gpointer _reserved1;
  gpointer _reserved2;
  gpointer _reserved3;
  gpointer _reserved4;
  gpointer _reserved5;
  gpointer _reserved6;
  gpointer _reserved7;
  gpointer _reserved8;
};

GtkWidget        *ide_shortcut_theme_editor_new       (void);
IdeShortcutTheme *ide_shortcut_theme_editor_get_theme (IdeShortcutThemeEditor *self);
void              ide_shortcut_theme_editor_set_theme (IdeShortcutThemeEditor *self,
                                                       IdeShortcutTheme       *theme);

G_END_DECLS

#endif /* IDE_SHORTCUT_THEME_EDITOR_H */
