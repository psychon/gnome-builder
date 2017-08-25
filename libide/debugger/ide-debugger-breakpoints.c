/* ide-debugger-breakpoints.c
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

#define G_LOG_DOMAIN "ide-debugger-breakpoints"

#include <stdlib.h>

#include "ide-debug.h"

#include "debugger/ide-debugger-breakpoints.h"

typedef struct
{
  guint line;
  IdeDebuggerBreakMode mode;
} LineInfo;

struct _IdeDebuggerBreakpoints
{
  GObject parent_instance;
  GArray *lines;
  GFile *file;
};

enum {
  PROP_0,
  PROP_FILE,
  N_PROPS
};

G_DEFINE_TYPE (IdeDebuggerBreakpoints, ide_debugger_breakpoints, G_TYPE_OBJECT)

static GParamSpec *properties [N_PROPS];

static gint
line_info_compare (gconstpointer a,
                   gconstpointer b)
{
  const LineInfo *lia = a;
  const LineInfo *lib = b;

  return (gint)lia->line - (gint)lib->line;
}

static void
ide_debugger_breakpoints_finalize (GObject *object)
{
  IdeDebuggerBreakpoints *self = (IdeDebuggerBreakpoints *)object;

  g_clear_object (&self->file);
  g_clear_pointer (&self->lines, g_array_unref);

  G_OBJECT_CLASS (ide_debugger_breakpoints_parent_class)->finalize (object);
}

static void
ide_debugger_breakpoints_get_property (GObject    *object,
                                       guint       prop_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  IdeDebuggerBreakpoints *self = IDE_DEBUGGER_BREAKPOINTS (object);

  switch (prop_id)
    {
    case PROP_FILE:
      g_value_set_object (value, self->file);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ide_debugger_breakpoints_set_property (GObject      *object,
                                       guint         prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  IdeDebuggerBreakpoints *self = IDE_DEBUGGER_BREAKPOINTS (object);

  switch (prop_id)
    {
    case PROP_FILE:
      self->file = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ide_debugger_breakpoints_class_init (IdeDebuggerBreakpointsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = ide_debugger_breakpoints_finalize;
  object_class->get_property = ide_debugger_breakpoints_get_property;
  object_class->set_property = ide_debugger_breakpoints_set_property;

  properties [PROP_FILE] =
    g_param_spec_object ("file",
                         "File",
                         "The file for the breakpoints",
                         G_TYPE_FILE,
                         (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
ide_debugger_breakpoints_init (IdeDebuggerBreakpoints *self)
{
}

IdeDebuggerBreakMode
ide_debugger_breakpoints_get_line (IdeDebuggerBreakpoints *self,
                                   guint                   line)
{
  g_return_val_if_fail (IDE_IS_DEBUGGER_BREAKPOINTS (self), 0);

  if (self->lines != NULL)
    {
      LineInfo info = { line, 0 };
      LineInfo *ret;

      ret = bsearch (&info, (gpointer)self->lines->data,
                     self->lines->len, sizeof (LineInfo),
                     line_info_compare);

      if (ret)
        return ret->mode;
    }

  return 0;
}

void
ide_debugger_breakpoints_set_line (IdeDebuggerBreakpoints *self,
                                   guint                   line,
                                   IdeDebuggerBreakMode    mode)
{
  LineInfo info;

  g_return_if_fail (IDE_IS_DEBUGGER_BREAKPOINTS (self));

  if (self->lines != NULL)
    {
      for (guint i = 0; i < self->lines->len; i++)
        {
          LineInfo *ele = &g_array_index (self->lines, LineInfo, i);

          if (ele->line == line)
            {
              g_array_remove_index_fast (self->lines, i);
              break;
            }
        }
    }

  if (mode == IDE_DEBUGGER_BREAK_NONE)
    return;

  if (self->lines == NULL)
    self->lines = g_array_new (FALSE, FALSE, sizeof (LineInfo));

  info.line = line;
  info.mode = mode;

  g_array_append_val (self->lines, info);
  g_array_sort (self->lines, line_info_compare);
}
