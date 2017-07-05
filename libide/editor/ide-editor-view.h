/* ide-editor-view.h
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

#include <gtksourceview/gtksource.h>

#include "buffers/ide-buffer.h"
#include "layout/ide-layout-view.h"
#include "sourceview/ide-source-view.h"

G_BEGIN_DECLS

#define IDE_TYPE_EDITOR_VIEW (ide_editor_view_get_type())

G_DECLARE_FINAL_TYPE (IdeEditorView, ide_editor_view, IDE, EDITOR_VIEW, IdeLayoutView)

IdeBuffer     *ide_editor_view_get_buffer (IdeEditorView *self);
IdeSourceView *ide_editor_view_get_view   (IdeEditorView *self);

G_END_DECLS
