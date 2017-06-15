/* ide-workbench-shortcuts.h
 *
 * Copyright (C) 2017 Christian Hergert <chergert@redhat.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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

#ifndef IDE_WORKBENCH_SHORTCUTS_H
#define IDE_WORKBENCH_SHORTCUTS_H

#include <dazzle.h>

G_BEGIN_DECLS

static const DzlShortcutEntry ide_workbench_shortcuts[] = {
  { "org.gnome.Builder.Shortcuts",  "<Primary><Shift>question", N_("Application"), N_("Application"), N_("Keyboard Shortcuts") },
  { "org.gnome.Builder.Help",       "F1",                       N_("Application"), N_("Application"), N_("Help") },
  { "org.gnome.Builder.About",      "<Shift>F1",                N_("Application"), N_("Application"), N_("About") },
  { "org.gnome.Builder.Quit",       "<Primary>q",               N_("Application"), N_("Application"), N_("Quit") },
};

G_END_DECLS

#endif /* IDE_WORKBENCH_SHORTCUTS_H */
