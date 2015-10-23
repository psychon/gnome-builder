/* ide-source-view-movements.h
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

#ifndef IDE_SOURCE_VIEW_HELPER_H
#define IDE_SOURCE_VIEW_HELPER_H

#include "ide-source-view.h"

G_BEGIN_DECLS

void _ide_source_view_apply_movement (IdeSourceView         *source_view,
                                      IdeSourceViewMovement  movement,
                                      gboolean               extend_selection,
                                      gboolean               exclusive,
                                      guint                  count,
                                      gunichar               command,
                                      gunichar               modifier,
                                      gunichar               search_char,
                                      gint                  *target_offset);

void _ide_source_view_select_inner   (IdeSourceView *self,
                                      gunichar       inner_left,
                                      gunichar       inner_right,
                                      guint          count,
                                      gboolean       exclusive,
                                      gboolean       string_mode);

G_END_DECLS

#endif /* IDE_SOURCE_VIEW_HELPER_H */
