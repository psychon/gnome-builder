/* ide-layout-private.h
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

#include "ide-layout-grid.h"
#include "ide-layout-grid-column.h"
#include "ide-layout-stack.h"
#include "ide-layout-stack-header.h"
#include "ide-layout-view.h"

G_BEGIN_DECLS

void            _ide_layout_stack_init_actions            (IdeLayoutStack       *self);
void            _ide_layout_stack_init_shortcuts          (IdeLayoutStack       *self);
void            _ide_layout_stack_update_actions          (IdeLayoutStack       *self);
void            _ide_layout_stack_transfer                (IdeLayoutStack       *self,
                                                           IdeLayoutStack       *dest,
                                                           IdeLayoutView        *view);
void            _ide_layout_grid_column_init_actions      (IdeLayoutGridColumn  *self);
void            _ide_layout_grid_column_update_actions    (IdeLayoutGridColumn  *self);
gboolean        _ide_layout_grid_column_is_empty          (IdeLayoutGridColumn  *self);
void            _ide_layout_grid_column_try_close         (IdeLayoutGridColumn  *self);
IdeLayoutStack *_ide_layout_grid_get_nth_stack            (IdeLayoutGrid        *self,
                                                           gint                  nth);
IdeLayoutStack *_ide_layout_grid_get_nth_stack_for_column (IdeLayoutGrid        *self,
                                                           IdeLayoutGridColumn  *column,
                                                           gint                  nth);
void            _ide_layout_stack_request_close           (IdeLayoutStack       *stack,
                                                           IdeLayoutView        *view);
void            _ide_layout_stack_header_update           (IdeLayoutStackHeader *self,
                                                           IdeLayoutView        *view);
void            _ide_layout_stack_header_hide             (IdeLayoutStackHeader *self);
void            _ide_layout_stack_header_popdown          (IdeLayoutStackHeader *self);
void            _ide_layout_stack_header_set_views        (IdeLayoutStackHeader *self,
                                                           GListModel           *model);
void            _ide_layout_stack_header_set_title        (IdeLayoutStackHeader *self,
                                                           const gchar          *title);
void            _ide_layout_stack_header_set_modified     (IdeLayoutStackHeader *self,
                                                           gboolean              modified);

G_END_DECLS
