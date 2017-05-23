/* ide-shortcuts-shortcutprivate.h
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

#ifndef IDE_SHORTCUTS_SHORTCUT_H
#define IDE_SHORTCUTS_SHORTCUT_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define IDE_TYPE_SHORTCUTS_SHORTCUT (ide_shortcuts_shortcut_get_type())
#define IDE_SHORTCUTS_SHORTCUT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), IDE_TYPE_SHORTCUTS_SHORTCUT, IdeShortcutsShortcut))
#define IDE_SHORTCUTS_SHORTCUT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), IDE_TYPE_SHORTCUTS_SHORTCUT, IdeShortcutsShortcutClass))
#define IDE_IS_SHORTCUTS_SHORTCUT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IDE_TYPE_SHORTCUTS_SHORTCUT))
#define IDE_IS_SHORTCUTS_SHORTCUT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), IDE_TYPE_SHORTCUTS_SHORTCUT))
#define IDE_SHORTCUTS_SHORTCUT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), IDE_TYPE_SHORTCUTS_SHORTCUT, IdeShortcutsShortcutClass))


typedef struct _IdeShortcutsShortcut      IdeShortcutsShortcut;
typedef struct _IdeShortcutsShortcutClass IdeShortcutsShortcutClass;

/**
 * IdeShortcutType:
 * @IDE_SHORTCUT_ACCELERATOR:
 *   The shortcut is a keyboard accelerator. The #IdeShortcutsShortcut:accelerator
 *   property will be used.
 * @IDE_SHORTCUT_GESTURE_PINCH:
 *   The shortcut is a pinch gesture. GTK+ provides an icon and subtitle.
 * @IDE_SHORTCUT_GESTURE_STRETCH:
 *   The shortcut is a stretch gesture. GTK+ provides an icon and subtitle.
 * @IDE_SHORTCUT_GESTURE_ROTATE_CLOCKWISE:
 *   The shortcut is a clockwise rotation gesture. GTK+ provides an icon and subtitle.
 * @IDE_SHORTCUT_GESTURE_ROTATE_COUNTERCLOCKWISE:
 *   The shortcut is a counterclockwise rotation gesture. GTK+ provides an icon and subtitle.
 * @IDE_SHORTCUT_GESTURE_TWO_FINGER_SWIPE_LEFT:
 *   The shortcut is a two-finger swipe gesture. GTK+ provides an icon and subtitle.
 * @IDE_SHORTCUT_GESTURE_TWO_FINGER_SWIPE_RIGHT:
 *   The shortcut is a two-finger swipe gesture. GTK+ provides an icon and subtitle.
 * @IDE_SHORTCUT_GESTURE:
 *   The shortcut is a gesture. The #IdeShortcutsShortcut:icon property will be
 *   used.
 *
 * IdeShortcutType specifies the kind of shortcut that is being described.
 * More values may be added to this enumeration over time.
 *
 * Since: 3.20
 */
typedef enum {
  IDE_SHORTCUT_ACCELERATOR,
  IDE_SHORTCUT_GESTURE_PINCH,
  IDE_SHORTCUT_GESTURE_STRETCH,
  IDE_SHORTCUT_GESTURE_ROTATE_CLOCKWISE,
  IDE_SHORTCUT_GESTURE_ROTATE_COUNTERCLOCKWISE,
  IDE_SHORTCUT_GESTURE_TWO_FINGER_SWIPE_LEFT,
  IDE_SHORTCUT_GESTURE_TWO_FINGER_SWIPE_RIGHT,
  IDE_SHORTCUT_GESTURE
} IdeShortcutType;

GType ide_shortcuts_shortcut_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* IDE_SHORTCUTS_SHORTCUT_H */
