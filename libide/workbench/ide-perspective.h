/* ide-perspective.h
 *
 * Copyright (C) 2015 Christian Hergert <chergert@redhat.com>
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

#ifndef IDE_PERSPECTIVE_H
#define IDE_PERSPECTIVE_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define IDE_TYPE_PERSPECTIVE (ide_perspective_get_type())

G_DECLARE_INTERFACE (IdePerspective, ide_perspective, IDE, PERSPECTIVE, GtkWidget)

struct _IdePerspectiveInterface
{
  GTypeInterface parent;

  gboolean      (*agree_to_shutdown)   (IdePerspective *self);
  gchar        *(*get_icon_name)       (IdePerspective *self);
  gchar        *(*get_id)              (IdePerspective *self);
  gboolean      (*get_needs_attention) (IdePerspective *self);
  gint          (*get_priority)        (IdePerspective *self);
  gchar        *(*get_title)           (IdePerspective *self);
  GtkWidget    *(*get_titlebar)        (IdePerspective *self);
  gboolean      (*is_early)            (IdePerspective *self);
  void          (*set_fullscreen)      (IdePerspective *self,
                                        gboolean        fullscreen);
  void          (*views_foreach)       (IdePerspective *self,
                                        GtkCallback     callback,
                                        gpointer        user_data);
  gchar        *(*get_accelerator)     (IdePerspective *self);
  void          (*restore_state)       (IdePerspective *self);
};

gboolean      ide_perspective_agree_to_shutdown   (IdePerspective *self);
gchar        *ide_perspective_get_icon_name       (IdePerspective *self);
gchar        *ide_perspective_get_id              (IdePerspective *self);
gboolean      ide_perspective_get_needs_attention (IdePerspective *self);
gint          ide_perspective_get_priority        (IdePerspective *self);
gchar        *ide_perspective_get_title           (IdePerspective *self);
GtkWidget    *ide_perspective_get_titlebar        (IdePerspective *self);
gboolean      ide_perspective_is_early            (IdePerspective *self);
void          ide_perspective_set_fullscreen      (IdePerspective *self,
                                                   gboolean        fullscreen);
void          ide_perspective_views_foreach       (IdePerspective *self,
                                                   GtkCallback     callback,
                                                   gpointer        user_data);
gchar        *ide_perspective_get_accelerator     (IdePerspective *self);
void          ide_perspective_restore_state       (IdePerspective *self);

G_END_DECLS

#endif /* IDE_PERSPECTIVE_H */
