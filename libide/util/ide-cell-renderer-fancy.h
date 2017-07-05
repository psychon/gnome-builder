/* ide-cell-renderer-fancy.h
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

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define IDE_TYPE_CELL_RENDERER_FANCY (ide_cell_renderer_fancy_get_type())

G_DECLARE_FINAL_TYPE (IdeCellRendererFancy, ide_cell_renderer_fancy, IDE, CELL_RENDERER_FANCY, GtkCellRenderer)

GtkCellRenderer *ide_cell_renderer_fancy_new        (void);
void             ide_cell_renderer_fancy_take_title (IdeCellRendererFancy *self,
                                                     gchar                *title);
void             ide_cell_renderer_fancy_set_title  (IdeCellRendererFancy *self,
                                                     const gchar          *title);
void             ide_cell_renderer_fancy_set_body   (IdeCellRendererFancy *self,
                                                     const gchar          *body);

G_END_DECLS
