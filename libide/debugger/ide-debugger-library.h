/* ide-debugger-library.h
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

#include <gio/gio.h>

#include "ide-debugger-types.h"

G_BEGIN_DECLS

#define IDE_TYPE_DEBUGGER_LIBRARY (ide_debugger_library_get_type())

G_DECLARE_DERIVABLE_TYPE (IdeDebuggerLibrary, ide_debugger_library, IDE, DEBUGGER_LIBRARY, GObject)

struct _IdeDebuggerLibraryClass
{
  GObjectClass parent_class;

  gpointer _reserved1;
  gpointer _reserved2;
  gpointer _reserved3;
  gpointer _reserved4;
  gpointer _reserved5;
  gpointer _reserved6;
  gpointer _reserved7;
  gpointer _reserved8;
};

gint                ide_debugger_library_compare         (IdeDebuggerLibrary            *a,
                                                          IdeDebuggerLibrary            *b);
IdeDebuggerLibrary *ide_debugger_library_new             (const gchar                   *id);
const gchar        *ide_debugger_library_get_id          (IdeDebuggerLibrary            *self);
GPtrArray          *ide_debugger_library_get_ranges      (IdeDebuggerLibrary            *self);
void                ide_debugger_library_add_range       (IdeDebuggerLibrary            *self,
                                                          const IdeDebuggerAddressRange *range);
const gchar        *ide_debugger_library_get_host_name   (IdeDebuggerLibrary            *self);
void                ide_debugger_library_set_host_name   (IdeDebuggerLibrary            *self,
                                                          const gchar                   *host_name);
const gchar        *ide_debugger_library_get_target_name (IdeDebuggerLibrary            *self);
void                ide_debugger_library_set_target_name (IdeDebuggerLibrary            *self,
                                                          const gchar                   *target_name);

G_END_DECLS
