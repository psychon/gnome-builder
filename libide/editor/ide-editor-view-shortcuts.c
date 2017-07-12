/* ide-editor-view-shortcuts.c
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

#include "config.h"

#include <dazzle.h>
#include <glib/gi18n.h>

#include "ide-editor-private.h"

#define I_(s) (g_intern_static_string(s))

static DzlShortcutEntry editor_view_shortcuts[] = {
  { "org.gnome.builder.editor-view.save",
    "<Primary>s",
    N_("Editor"),
    N_("Files"),
    N_("Save the document") },

  { "org.gnome.builder.editor-view.save-as",
    "<Primary><shift>s",
    N_("Editor"),
    N_("Files"),
    N_("Save the document with a new name") },

  { "org.gnome.builder.editor-view.find",
    "<Primary>f",
    N_("Editor"),
    N_("Find and replace"),
    N_("Find") },

  { "org.gnome.builder.editor-view.find-and-replace",
    "<Primary>h",
    N_("Editor"),
    N_("Find and replace"),
    N_("Find and replace") },

  { "org.gnome.builder.editor-view.next-match",
    "<Primary>g",
    N_("Editor"),
    N_("Find and replace"),
    N_("Move to the next match") },

  { "org.gnome.builder.editor-view.prev-match",
    "<Primary><Shift>g",
    N_("Editor"),
    N_("Find and replace"),
    N_("Move to the previous match") },

  { "org.gnome.builder.editor-view.next-error",
    NULL,
    N_("Editor"),
    N_("Find and replace"),
    N_("Move to the next error") },

  { "org.gnome.builder.editor-view.prev-error",
    NULL,
    N_("Editor"),
    N_("Find and replace"),
    N_("Move to the previous error") },

  { "org.gnome.builder.editor-view.clear-highlight",
    "<Primary><Shift>k",
    N_("Editor"),
    N_("Find and replace"),
    N_("Find the next match") },
};

void
_ide_editor_view_init_shortcuts (IdeEditorView *self)
{
  DzlShortcutController *controller;

  g_return_if_fail (IDE_IS_EDITOR_VIEW (self));

  controller = dzl_shortcut_controller_find (GTK_WIDGET (self));

  dzl_shortcut_controller_add_command_action (controller,
                                              I_("org.gnome.builder.editor-view.find"),
                                              NULL,
                                              I_("editor-view.find"));

  dzl_shortcut_controller_add_command_action (controller,
                                              I_("org.gnome.builder.editor-view.find-and-replace"),
                                              NULL,
                                              I_("editor-view.find-and-replace"));

  dzl_shortcut_controller_add_command_action (controller,
                                              I_("org.gnome.builder.editor-view.next-match"),
                                              NULL,
                                              I_("editor-view.move-next-search-result"));

  dzl_shortcut_controller_add_command_action (controller,
                                              I_("org.gnome.builder.editor-view.prev-match"),
                                              NULL,
                                              I_("editor-view.move-prevous-search-result"));

  dzl_shortcut_controller_add_command_action (controller,
                                              I_("org.gnome.builder.editor-view.next-error"),
                                              NULL,
                                              I_("editor-view.move-next-error"));

  dzl_shortcut_controller_add_command_action (controller,
                                              I_("org.gnome.builder.editor-view.prev-error"),
                                              NULL,
                                              I_("editor-view.move-prevous-error"));

  dzl_shortcut_controller_add_command_action (controller,
                                              I_("org.gnome.builder.editor-view.clear-highlight"),
                                              NULL,
                                              I_("editor-view.clear-highlight"));

  dzl_shortcut_controller_add_command_action (controller,
                                              I_("org.gnome.builder.editor-view.save"),
                                              NULL,
                                              I_("editor-view.save"));

  dzl_shortcut_controller_add_command_action (controller,
                                              I_("org.gnome.builder.editor-view.save-as"),
                                              NULL,
                                              I_("editor-view.save-as"));

  dzl_shortcut_manager_add_shortcut_entries (NULL,
                                             editor_view_shortcuts,
                                             G_N_ELEMENTS (editor_view_shortcuts),
                                             GETTEXT_PACKAGE);
}
