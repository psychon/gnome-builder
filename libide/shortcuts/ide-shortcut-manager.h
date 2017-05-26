/* ide-shortcut-manager.c
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

#ifndef IDE_SHORTCUT_MANAGER_H
#define IDE_SHORTCUT_MANAGER_H

#include <gtk/gtk.h>

#include "ide-shortcut-theme.h"
#include "ide-shortcuts-window.h"

G_BEGIN_DECLS

#define IDE_TYPE_SHORTCUT_MANAGER (ide_shortcut_manager_get_type())

G_DECLARE_DERIVABLE_TYPE (IdeShortcutManager, ide_shortcut_manager, IDE, SHORTCUT_MANAGER, GObject)

typedef struct
{
  const gchar *command;
  const gchar *section;
  const gchar *group;
  const gchar *title;
  const gchar *subtitle;
  /* TODO: Should we add a default accelerator to add to the default theme? */
} IdeShortcutEntry;

struct _IdeShortcutManagerClass
{
  GObjectClass parent_instance;

  gpointer _reserved1;
  gpointer _reserved2;
  gpointer _reserved3;
  gpointer _reserved4;
  gpointer _reserved5;
  gpointer _reserved6;
  gpointer _reserved7;
  gpointer _reserved8;
};

IdeShortcutManager *ide_shortcut_manager_get_default             (void);
void                ide_shortcut_manager_append_search_path      (IdeShortcutManager     *self,
                                                                  const gchar            *directory);
void                ide_shortcut_manager_prepend_search_path     (IdeShortcutManager     *self,
                                                                  const gchar            *directory);
IdeShortcutTheme   *ide_shortcut_manager_get_theme               (IdeShortcutManager     *self);
void                ide_shortcut_manager_set_theme               (IdeShortcutManager     *self,
                                                                  IdeShortcutTheme       *theme);
const gchar        *ide_shortcut_manager_get_theme_name          (IdeShortcutManager     *self);
void                ide_shortcut_manager_set_theme_name          (IdeShortcutManager     *self,
                                                                  const gchar            *theme_name);
gboolean            ide_shortcut_manager_handle_event            (IdeShortcutManager     *self,
                                                                  const GdkEventKey      *event,
                                                                  GtkWidget              *toplevel);
void                ide_shortcut_manager_add_theme               (IdeShortcutManager     *self,
                                                                  IdeShortcutTheme       *theme);
void                ide_shortcut_manager_remove_theme            (IdeShortcutManager     *self,
                                                                  IdeShortcutTheme       *theme);
const gchar        *ide_shortcut_manager_get_user_dir            (IdeShortcutManager     *self);
void                ide_shortcut_manager_set_user_dir            (IdeShortcutManager     *self,
                                                                  const gchar            *user_dir);
void                ide_shortcut_manager_add_action              (IdeShortcutManager     *self,
                                                                  const gchar            *detailed_action_name,
                                                                  const gchar            *section,
                                                                  const gchar            *group,
                                                                  const gchar            *title,
                                                                  const gchar            *subtitle);
void                ide_shortcut_manager_add_command             (IdeShortcutManager     *self,
                                                                  const gchar            *command,
                                                                  const gchar            *section,
                                                                  const gchar            *group,
                                                                  const gchar            *title,
                                                                  const gchar            *subtitle);
void                ide_shortcut_manager_add_shortcut_entries    (IdeShortcutManager     *self,
                                                                  const IdeShortcutEntry *shortcuts,
                                                                  guint                   n_shortcuts,
                                                                  const gchar            *translation_domain);
void                ide_shortcut_manager_add_shortcuts_to_window (IdeShortcutManager     *self,
                                                                  IdeShortcutsWindow     *window);

G_END_DECLS

#endif /* IDE_SHORTCUT_MANAGER_H */
