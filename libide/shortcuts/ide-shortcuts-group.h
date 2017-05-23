/* ide-shortcuts-groupprivate.h
 *
 * Copyright (C) 2015 Christian Hergert <christian@hergert.me>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __IDE_SHORTCUTS_GROUP_H__
#define __IDE_SHORTCUTS_GROUP_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define IDE_TYPE_SHORTCUTS_GROUP            (ide_shortcuts_group_get_type ())
#define IDE_SHORTCUTS_GROUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), IDE_TYPE_SHORTCUTS_GROUP, IdeShortcutsGroup))
#define IDE_SHORTCUTS_GROUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), IDE_TYPE_SHORTCUTS_GROUP, IdeShortcutsGroupClass))
#define IDE_IS_SHORTCUTS_GROUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IDE_TYPE_SHORTCUTS_GROUP))
#define IDE_IS_SHORTCUTS_GROUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), IDE_TYPE_SHORTCUTS_GROUP))
#define IDE_SHORTCUTS_GROUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), IDE_TYPE_SHORTCUTS_GROUP, IdeShortcutsGroupClass))


typedef struct _IdeShortcutsGroup         IdeShortcutsGroup;
typedef struct _IdeShortcutsGroupClass    IdeShortcutsGroupClass;

GType ide_shortcuts_group_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __IDE_SHORTCUTS_GROUP_H__ */
