/* ide-debug-manager.c
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

#define G_LOG_DOMAIN "ide-debug-manager"

#include <dazzle.h>
#include <glib/gi18n.h>

#include "ide-debug.h"

#include "debugger/ide-debug-manager.h"
#include "debugger/ide-debugger.h"
#include "debugger/ide-debugger-private.h"
#include "plugins/ide-extension-util.h"
#include "runner/ide-runner.h"

struct _IdeDebugManager
{
  IdeObject           parent_instance;

  GHashTable         *breakpoints;
  IdeDebugger        *debugger;
  DzlSignalGroup     *debugger_signals;
  IdeRunner          *runner;

  guint               active : 1;
};

typedef struct
{
  IdeDebugger *debugger;
  IdeRunner   *runner;
  gint         priority;
} DebuggerLookup;

enum {
  PROP_0,
  PROP_ACTIVE,
  PROP_DEBUGGER,
  N_PROPS
};

enum {
  BREAKPOINT_ADDED,
  BREAKPOINT_REMOVED,
  BREAKPOINT_REACHED,
  N_SIGNALS
};

static GParamSpec *properties [N_PROPS];
static guint signals [N_SIGNALS];

G_DEFINE_TYPE (IdeDebugManager, ide_debug_manager, IDE_TYPE_OBJECT)

static void
ide_debug_manager_set_active (IdeDebugManager *self,
                              gboolean         active)
{
  g_assert (IDE_IS_DEBUG_MANAGER (self));

  active = !!active;

  if (active != self->active)
    {
      self->active = active;
      g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_ACTIVE]);
    }
}

static void
ide_debug_manager_debugger_stopped (IdeDebugManager       *self,
                                    IdeDebuggerStopReason  stop_reason,
                                    IdeDebuggerBreakpoint *breakpoint,
                                    IdeDebugger           *debugger)
{
  IDE_ENTRY;

  g_assert (IDE_IS_DEBUG_MANAGER (self));
  g_assert (IDE_IS_DEBUGGER_STOP_REASON (stop_reason));
  g_assert (!breakpoint || IDE_IS_DEBUGGER_BREAKPOINT (breakpoint));
  g_assert (IDE_IS_DEBUGGER (debugger));

  switch (stop_reason)
    {
    case IDE_DEBUGGER_STOP_EXITED:
    case IDE_DEBUGGER_STOP_EXITED_NORMALLY:
    case IDE_DEBUGGER_STOP_EXITED_SIGNALED:
      /* Cleanup any lingering debugger process */
      if (self->runner != NULL)
        ide_runner_force_quit (self->runner);
      break;

    case IDE_DEBUGGER_STOP_BREAKPOINT_HIT:
    case IDE_DEBUGGER_STOP_FUNCTION_FINISHED:
    case IDE_DEBUGGER_STOP_LOCATION_REACHED:
    case IDE_DEBUGGER_STOP_SIGNAL_RECEIVED:
    case IDE_DEBUGGER_STOP_CATCH:
    case IDE_DEBUGGER_STOP_UNKNOWN:
      if (breakpoint != NULL)
        {
          IDE_TRACE_MSG ("Emitting breakpoint-reached");
          g_signal_emit (self, signals [BREAKPOINT_REACHED], 0, breakpoint);
        }
      break;

    default:
      g_assert_not_reached ();
    }

  IDE_EXIT;
}

static void
ide_debug_manager_breakpoint_added (IdeDebugManager       *self,
                                    IdeDebuggerBreakpoint *breakpoint,
                                    IdeDebugger           *debugger)
{
  IdeDebuggerBreakpoints *breakpoints;
  IdeDebuggerBreakMode mode;
  g_autoptr(GFile) file = NULL;
  const gchar *path;
  guint line;

  g_assert (IDE_IS_DEBUG_MANAGER (self));
  g_assert (IDE_IS_DEBUGGER_BREAKPOINT (breakpoint));
  g_assert (IDE_IS_DEBUGGER (debugger));

  path = ide_debugger_breakpoint_get_file (breakpoint);
  file = g_file_new_for_path (path);

  breakpoints = g_hash_table_lookup (self->breakpoints, file);

  if (breakpoints == NULL)
    {
      breakpoints = g_object_new (IDE_TYPE_DEBUGGER_BREAKPOINTS,
                                  "file", file,
                                  NULL);
      g_hash_table_insert (self->breakpoints, g_steal_pointer (&file), breakpoints);
    }

  mode = ide_debugger_breakpoint_get_mode (breakpoint);
  line = ide_debugger_breakpoint_get_line (breakpoint);

  ide_debugger_breakpoints_set_line (breakpoints, line, mode);
}

static void
ide_debug_manager_breakpoint_removed (IdeDebugManager       *self,
                                      IdeDebuggerBreakpoint *breakpoint,
                                      IdeDebugger           *debugger)
{
  IdeDebuggerBreakpoints *breakpoints;
  g_autoptr(GFile) file = NULL;
  const gchar *path;
  guint line;

  g_assert (IDE_IS_DEBUG_MANAGER (self));
  g_assert (IDE_IS_DEBUGGER_BREAKPOINT (breakpoint));
  g_assert (IDE_IS_DEBUGGER (debugger));

  line = ide_debugger_breakpoint_get_line (breakpoint);
  path = ide_debugger_breakpoint_get_file (breakpoint);
  file = g_file_new_for_path (path);

  breakpoints = g_hash_table_lookup (self->breakpoints, file);
  if (breakpoints != NULL)
    ide_debugger_breakpoints_set_line (breakpoints, line, IDE_DEBUGGER_BREAK_NONE);
}

static void
ide_debug_manager_finalize (GObject *object)
{
  IdeDebugManager *self = (IdeDebugManager *)object;

  g_clear_object (&self->debugger);
  g_clear_object (&self->debugger_signals);
  g_clear_object (&self->runner);
  g_clear_pointer (&self->breakpoints, g_hash_table_unref);

  G_OBJECT_CLASS (ide_debug_manager_parent_class)->finalize (object);
}

static void
ide_debug_manager_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  IdeDebugManager *self = IDE_DEBUG_MANAGER (object);

  switch (prop_id)
    {
    case PROP_ACTIVE:
      g_value_set_boolean (value, self->active);
      break;

    case PROP_DEBUGGER:
      g_value_set_object (value, self->debugger);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ide_debug_manager_class_init (IdeDebugManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = ide_debug_manager_finalize;
  object_class->get_property = ide_debug_manager_get_property;

  /**
   * IdeDebugManager:active:
   *
   * If the debugger is active.
   *
   * This can be used to determine if the controls should be made visible
   * in the workbench.
   */
  properties [PROP_ACTIVE] =
    g_param_spec_boolean ("active",
                          "Active",
                          "If the debugger is running",
                          FALSE,
                          (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  properties [PROP_DEBUGGER] =
    g_param_spec_object ("debugger",
                         "Debugger",
                         "The current debugger being used",
                         IDE_TYPE_DEBUGGER,
                         (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  signals [BREAKPOINT_ADDED] =
    g_signal_new ("breakpoint-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL,
                  G_TYPE_NONE, 1, IDE_TYPE_DEBUGGER_BREAKPOINT);

  signals [BREAKPOINT_REMOVED] =
    g_signal_new ("breakpoint-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL,
                  G_TYPE_NONE, 1, IDE_TYPE_DEBUGGER_BREAKPOINT);

  signals [BREAKPOINT_REACHED] =
    g_signal_new ("breakpoint-reached",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL,
                  G_TYPE_NONE, 1, IDE_TYPE_DEBUGGER_BREAKPOINT);
}

static void
ide_debug_manager_init (IdeDebugManager *self)
{
  self->breakpoints = g_hash_table_new_full ((GHashFunc)g_file_hash,
                                             (GEqualFunc)g_file_equal,
                                             g_object_unref,
                                             g_object_unref);

  self->debugger_signals = dzl_signal_group_new (IDE_TYPE_DEBUGGER);

  dzl_signal_group_connect_swapped (self->debugger_signals,
                                    "stopped",
                                    G_CALLBACK (ide_debug_manager_debugger_stopped),
                                    self);

  dzl_signal_group_connect_swapped (self->debugger_signals,
                                    "breakpoint-added",
                                    G_CALLBACK (ide_debug_manager_breakpoint_added),
                                    self);

  dzl_signal_group_connect_swapped (self->debugger_signals,
                                    "breakpoint-removed",
                                    G_CALLBACK (ide_debug_manager_breakpoint_removed),
                                    self);
}

static void
debugger_lookup (PeasExtensionSet *set,
                 PeasPluginInfo   *plugin_info,
                 PeasExtension    *exten,
                 gpointer          user_data)
{
  DebuggerLookup *lookup = user_data;
  IdeDebugger *debugger = (IdeDebugger *)exten;
  gint priority = G_MAXINT;

  g_assert (PEAS_IS_EXTENSION_SET (set));
  g_assert (plugin_info != NULL);
  g_assert (IDE_IS_DEBUGGER (debugger));
  g_assert (lookup != NULL);

  if (ide_debugger_supports_runner (debugger, lookup->runner, &priority))
    {
      if (lookup->debugger == NULL || priority < lookup->priority)
        {
          g_set_object (&lookup->debugger, debugger);
          lookup->priority = priority;
        }
    }
}

/**
 * ide_debug_manager_find_debugger:
 * @self: a #IdeDebugManager
 * @runner: An #IdeRunner
 *
 * Locates a debugger for the given runner, or %NULL if no debugger
 * supports the runner.
 *
 * Returns: (transfer full) (nullable): An #IdeDebugger or %NULL
 */
IdeDebugger *
ide_debug_manager_find_debugger (IdeDebugManager *self,
                                 IdeRunner       *runner)
{
  g_autoptr(PeasExtensionSet) set = NULL;
  IdeContext *context;
  DebuggerLookup lookup;

  g_return_val_if_fail (IDE_IS_DEBUG_MANAGER (self), NULL);
  g_return_val_if_fail (IDE_IS_RUNNER (runner), NULL);

  context = ide_object_get_context (IDE_OBJECT (runner));

  lookup.debugger = NULL;
  lookup.runner = runner;
  lookup.priority = G_MAXINT;

  set = ide_extension_set_new (peas_engine_get_default (),
                               IDE_TYPE_DEBUGGER,
                               "context", context,
                               NULL);

  peas_extension_set_foreach (set, debugger_lookup, &lookup);

  return lookup.debugger;
}

static void
ide_debug_manager_runner_exited (IdeDebugManager *self,
                                 IdeRunner       *runner)
{
  g_assert (IDE_IS_DEBUG_MANAGER (self));
  g_assert (IDE_IS_RUNNER (runner));

  g_clear_object (&self->runner);
  g_clear_object (&self->debugger);

  ide_debug_manager_set_active (self, FALSE);

  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_DEBUGGER]);
}

/**
 * ide_debug_manager_start:
 * @self: an #IdeDebugManager
 * @runner: an #IdeRunner
 * @error: A location for an @error
 *
 * Attempts to start a runner using a discovered debugger backend.
 *
 * Returns: %TRUE if successful; otherwise %FALSE and @error is set.
 */
gboolean
ide_debug_manager_start (IdeDebugManager  *self,
                         IdeRunner        *runner,
                         GError          **error)
{
  g_autoptr(IdeDebugger) debugger = NULL;
  gboolean ret = FALSE;

  IDE_ENTRY;

  g_return_val_if_fail (IDE_IS_DEBUG_MANAGER (self), FALSE);
  g_return_val_if_fail (IDE_IS_RUNNER (runner), FALSE);

  debugger = ide_debug_manager_find_debugger (self, runner);

  if (debugger == NULL)
    {
      ide_runner_set_failed (runner, TRUE);
      g_set_error (error,
                   G_IO_ERROR,
                   G_IO_ERROR_NOT_SUPPORTED,
                   _("A suitable debugger could not be found."));
      IDE_GOTO (failure);
    }

  ide_debugger_prepare (debugger, runner);

  g_signal_connect_object (runner,
                           "exited",
                           G_CALLBACK (ide_debug_manager_runner_exited),
                           self,
                           G_CONNECT_SWAPPED);

  self->runner = g_object_ref (runner);
  self->debugger = g_steal_pointer (&debugger);

  dzl_signal_group_set_target (self->debugger_signals, self->debugger);

  ide_debug_manager_set_active (self, TRUE);

  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_DEBUGGER]);

  ret = TRUE;

failure:
  IDE_RETURN (ret);
}

void
ide_debug_manager_stop (IdeDebugManager *self)
{
  g_return_if_fail (IDE_IS_DEBUG_MANAGER (self));

  dzl_signal_group_set_target (self->debugger_signals, NULL);

  if (self->runner != NULL)
    {
      ide_runner_force_quit (self->runner);
      g_clear_object (&self->runner);
    }

  g_clear_object (&self->debugger);
  g_object_notify_by_pspec (G_OBJECT (self), properties [PROP_DEBUGGER]);
}

gboolean
ide_debug_manager_get_active (IdeDebugManager *self)
{
  g_return_val_if_fail (IDE_IS_DEBUG_MANAGER (self), FALSE);

  return self->active;
}

/**
 * ide_debug_manager_get_debugger:
 * @self: a #IdeDebugManager
 *
 * Gets the debugger instance, if it is loaded.
 *
 * Returns: (transfer none) (nullable): An #IdeDebugger or %NULL
 */
IdeDebugger *
ide_debug_manager_get_debugger (IdeDebugManager *self)
{
  g_return_val_if_fail (IDE_IS_DEBUG_MANAGER (self), NULL);

  return self->debugger;
}
