/* ide-back-forward-item.h
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

#ifndef IDE_BACK_FORWARD_ITEM_H
#define IDE_BACK_FORWARD_ITEM_H

#include <gtk/gtk.h>

#include "ide-object.h"

#include "util/ide-uri.h"

G_BEGIN_DECLS

#define IDE_TYPE_BACK_FORWARD_ITEM (ide_back_forward_item_get_type())

G_DECLARE_FINAL_TYPE (IdeBackForwardItem, ide_back_forward_item, IDE, BACK_FORWARD_ITEM, IdeObject)

IdeBackForwardItem *ide_back_forward_item_new     (IdeContext         *context,
                                                   IdeUri             *uri,
						   GtkTextMark	      *mark);
GtkTextMark        *ide_back_forward_item_get_mark (IdeBackForwardItem *self);
IdeUri             *ide_back_forward_item_get_uri (IdeBackForwardItem *self);
gboolean            ide_back_forward_item_chain   (IdeBackForwardItem *self,
                                                   IdeBackForwardItem *other);

G_END_DECLS

#endif /* IDE_BACK_FORWARD_ITEM_H */
