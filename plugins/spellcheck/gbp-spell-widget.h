/* gbp-spell-widget.h
 *
 * Copyright (C) 2016 Sebastien Lafargue <slafargue@gnome.org>
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

#include <ide.h>

G_BEGIN_DECLS

#define GBP_TYPE_SPELL_WIDGET (gbp_spell_widget_get_type())

G_DECLARE_FINAL_TYPE (GbpSpellWidget, gbp_spell_widget, GBP, SPELL_WIDGET, GtkBin)

GtkWidget     *gbp_spell_widget_new        (IdeEditorView  *editor);
IdeEditorView *gbp_spell_widget_get_editor (GbpSpellWidget *self);
void           gbp_spell_widget_set_editor (GbpSpellWidget *self,
                                            IdeEditorView  *editor);

G_END_DECLS
