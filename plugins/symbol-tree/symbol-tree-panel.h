/* symbol-tree-panel.h
 *
 * Copyright (C) 2016 Christian Hergert <chergert@redhat.com>
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

#ifndef SYMBOL_TREE_PANEL_H
#define SYMBOL_TREE_PANEL_H

#include <ide.h>

G_BEGIN_DECLS

#define SYMBOL_TYPE_TREE_PANEL (symbol_tree_panel_get_type())

G_DECLARE_FINAL_TYPE (SymbolTreePanel, symbol_tree_panel, SYMBOL, TREE_PANEL, DzlDockWidget)

void symbol_tree_panel_reset (SymbolTreePanel *self);

G_END_DECLS

#endif /* SYMBOL_TREE_PANEL_H */