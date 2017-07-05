/* ide-editor-private.h
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

#pragma once

#include "editor/ide-editor-perspective.h"
#include "editor/ide-editor-search-bar.h"
#include "editor/ide-editor-sidebar.h"
#include "editor/ide-editor-view-addin.h"
#include "editor/ide-editor-view.h"
#include "plugins/ide-extension-set-adapter.h"

G_BEGIN_DECLS

struct _IdeEditorView
{
  IdeLayoutView            parent_instance;

  IdeExtensionSetAdapter  *addins;

  IdeBuffer               *buffer;
  DzlBindingGroup         *buffer_bindings;
  DzlSignalGroup          *buffer_signals;

  GtkOverlay              *overlay;
  IdeSourceView           *source_view;
  GtkScrolledWindow       *scroller;
  IdeEditorSearchBar      *search_bar;
  GtkRevealer             *search_revealer;
  GtkProgressBar          *progress_bar;
};

void _ide_editor_view_init_actions          (IdeEditorView        *self);
void _ide_editor_view_init_settings         (IdeEditorView        *self);
void _ide_editor_view_init_shortcuts        (IdeEditorView        *self);
void _ide_editor_sidebar_set_open_pages     (IdeEditorSidebar     *self,
                                             GListModel           *open_pages);
void _ide_editor_perspective_init_actions   (IdeEditorPerspective *self);
void _ide_editor_perspective_init_shortcuts (IdeEditorPerspective *self);

G_END_DECLS
