/* ide-layout-stack.h
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

#include "ide-layout-view.h"

G_BEGIN_DECLS

#define IDE_TYPE_LAYOUT_STACK (ide_layout_stack_get_type())

G_DECLARE_DERIVABLE_TYPE (IdeLayoutStack, ide_layout_stack, IDE, LAYOUT_STACK, GtkBox)

struct _IdeLayoutStackClass
{
  GtkBoxClass parent_class;

  void     (*agree_to_close_async)  (IdeLayoutStack       *stack,
                                     GCancellable         *cancellable,
                                     GAsyncReadyCallback   callback,
                                     gpointer              user_data);
  gboolean (*agree_to_close_finish) (IdeLayoutStack       *stack,
                                     GAsyncResult         *result,
                                     GError              **error);

  gpointer _reserved1;
  gpointer _reserved2;
  gpointer _reserved3;
  gpointer _reserved4;
  gpointer _reserved5;
  gpointer _reserved6;
  gpointer _reserved7;
  gpointer _reserved8;
};

GtkWidget     *ide_layout_stack_new                   (void);
GtkWidget     *ide_layout_stack_get_titlebar          (IdeLayoutStack       *self);
IdeLayoutView *ide_layout_stack_get_visible_child     (IdeLayoutStack       *self);
void           ide_layout_stack_set_visible_child     (IdeLayoutStack       *self,
                                                       IdeLayoutView        *view);
gboolean       ide_layout_stack_get_has_view          (IdeLayoutStack       *self);
void           ide_layout_stack_agree_to_close_async  (IdeLayoutStack       *self,
                                                       GCancellable         *cancellable,
                                                       GAsyncReadyCallback   callback,
                                                       gpointer              user_data);
gboolean       ide_layout_stack_agree_to_close_finish (IdeLayoutStack       *self,
                                                       GAsyncResult         *result,
                                                       GError              **error);

G_END_DECLS
