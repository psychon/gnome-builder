/* ide-layout-stack-header.h
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

#include <dazzle.h>

G_BEGIN_DECLS

#define IDE_TYPE_LAYOUT_STACK_HEADER (ide_layout_stack_header_get_type())

G_DECLARE_FINAL_TYPE (IdeLayoutStackHeader, ide_layout_stack_header, IDE, LAYOUT_STACK_HEADER, DzlPriorityBox)

GtkWidget *ide_layout_stack_header_new              (void);
void       ide_layout_stack_header_add_custom_title (IdeLayoutStackHeader *self,
                                                     GtkWidget            *widget,
                                                     gint                  priority);

G_END_DECLS
