/* ide-editor-view-addin.c
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

#include "ide-editor-view-addin.h"

G_DEFINE_INTERFACE (IdeEditorViewAddin, ide_editor_view_addin, G_TYPE_OBJECT)

static void
ide_editor_view_addin_default_init (IdeEditorViewAddinInterface *iface)
{
}

void
ide_editor_view_addin_load (IdeEditorViewAddin *self,
                            IdeEditorView      *view)
{
  g_return_if_fail (IDE_IS_EDITOR_VIEW_ADDIN (self));
  g_return_if_fail (IDE_IS_EDITOR_VIEW (view));

  if (IDE_EDITOR_VIEW_ADDIN_GET_IFACE (self)->load)
    IDE_EDITOR_VIEW_ADDIN_GET_IFACE (self)->load (self, view);
}

void
ide_editor_view_addin_unload (IdeEditorViewAddin *self,
                              IdeEditorView      *view)
{
  g_return_if_fail (IDE_IS_EDITOR_VIEW_ADDIN (self));
  g_return_if_fail (IDE_IS_EDITOR_VIEW (view));

  if (IDE_EDITOR_VIEW_ADDIN_GET_IFACE (self)->unload)
    IDE_EDITOR_VIEW_ADDIN_GET_IFACE (self)->unload (self, view);
}

void
ide_editor_view_addin_language_changed (IdeEditorViewAddin *self,
                                        const gchar        *language_id)
{
  g_return_if_fail (IDE_IS_EDITOR_VIEW_ADDIN (self));

  if (IDE_EDITOR_VIEW_ADDIN_GET_IFACE (self)->language_changed)
    IDE_EDITOR_VIEW_ADDIN_GET_IFACE (self)->language_changed (self, language_id);
}
