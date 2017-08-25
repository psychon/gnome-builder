/* ide-debugger-plugin.c
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

#include <libpeas/peas.h>

#if 0
#include "debugger/ide-debugger-editor-view-addin.h"
#endif
#include "debugger/ide-debugger-workbench-addin.h"
#include "editor/ide-editor-view-addin.h"
#include "workbench/ide-workbench-addin.h"

void
ide_debugger_register_types (PeasObjectModule *module)
{
#if 0
  peas_object_module_register_extension_type (module,
                                              IDE_TYPE_EDITOR_VIEW_ADDIN,
                                              IDE_TYPE_DEBUGGER_EDITOR_VIEW_ADDIN);
#endif

  peas_object_module_register_extension_type (module,
                                              IDE_TYPE_WORKBENCH_ADDIN,
                                              IDE_TYPE_DEBUGGER_WORKBENCH_ADDIN);
}
