/* ide-editor-view-private.h
 *
 * Copyright (C) 2015 Christian Hergert <christian@hergert.me>
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

#ifndef IDE_EDITOR_VIEW_PRIVATE_H
#define IDE_EDITOR_VIEW_PRIVATE_H

#include <dazzle.h>
#include <libpeas/peas.h>
#include <gtk/gtk.h>

#include "buffers/ide-buffer.h"
#include "editor/ide-editor-frame.h"
#include "editor/ide-editor-tweak-widget.h"
#include "layout/ide-layout-view.h"

G_BEGIN_DECLS

struct _IdeEditorView
{
  IdeLayoutView         parent_instance;

  IdeBuffer            *document;
  PeasExtensionSet     *extensions;
  GSettings            *settings;
  gchar                *title;

  IdeEditorFrame       *frame1;
  IdeEditorFrame       *frame2;
  IdeEditorFrame       *last_focused_frame;
  GtkButton            *modified_cancel_button;
  GtkRevealer          *modified_revealer;
  GtkPaned             *paned;
  GtkProgressBar       *progress_bar;
};

G_END_DECLS

#endif /* IDE_EDITOR_VIEW_PRIVATE_H */
