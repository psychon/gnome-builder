/* ide-shortcut-chord.c
 *
 * Copyright (C) 2017 Christian Hergert <chergert@redhat.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
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

#define G_LOG_DOMAIN "ide-shortcut-chord"

#include <stdlib.h>
#include <string.h>

#include "ide-shortcut-chord.h"
#include "ide-shortcut-private.h"

#define MAX_CHORD_SIZE 4

G_DEFINE_BOXED_TYPE (IdeShortcutChord, ide_shortcut_chord,
                     ide_shortcut_chord_copy, ide_shortcut_chord_free)
G_DEFINE_POINTER_TYPE (IdeShortcutChordTable, ide_shortcut_chord_table)

typedef struct
{
  guint           keyval;
  GdkModifierType modifier;
} IdeShortcutKey;

struct _IdeShortcutChord
{
  IdeShortcutKey keys[MAX_CHORD_SIZE];
};

typedef struct
{
  IdeShortcutChord chord;
  gpointer data;
} IdeShortcutChordTableEntry;

struct _IdeShortcutChordTable
{
  IdeShortcutChordTableEntry *entries;
  GDestroyNotify              destroy;
  guint                       len;
  guint                       size;
};

static GdkModifierType
sanitize_modifier_mask (GdkModifierType mods)
{
  mods &= gtk_accelerator_get_default_mod_mask ();
  mods &= ~GDK_LOCK_MASK;

  return mods;
}

static gint
ide_shortcut_chord_compare (const IdeShortcutChord *a,
                            const IdeShortcutChord *b)
{
  return memcmp (a, b, sizeof *a);
}

static gboolean
ide_shortcut_chord_is_valid (IdeShortcutChord *self)
{
  g_assert (self != NULL);

  /* Ensure we got a valid first key at least */
  if (self->keys[0].keyval == 0)
    return FALSE;

  return TRUE;
}

IdeShortcutChord *
ide_shortcut_chord_new_from_event (const GdkEventKey *key)
{
  IdeShortcutChord *self;

  g_return_val_if_fail (key != NULL, NULL);

  /* Ignore modifier keypresses */
  if (key->is_modifier)
    return NULL;

  self = g_slice_new0 (IdeShortcutChord);

  self->keys[0].keyval = gdk_keyval_to_lower (key->keyval);
  self->keys[0].modifier = sanitize_modifier_mask (key->state);

  if (self->keys[0].keyval != key->keyval)
    self->keys[0].modifier |= GDK_SHIFT_MASK;

  if (!ide_shortcut_chord_is_valid (self))
    g_clear_pointer (&self, ide_shortcut_chord_free);

  return self;
}

IdeShortcutChord *
ide_shortcut_chord_new_from_string (const gchar *accelerator)
{
  IdeShortcutChord *self;
  g_auto(GStrv) parts = NULL;

  g_return_val_if_fail (accelerator != NULL, NULL);

  /* We might have a single key, or chord defined */
  parts = g_strsplit (accelerator, "|", 0);

  /* Make sure we won't overflow the keys array */
  if (g_strv_length (parts) > G_N_ELEMENTS (self->keys))
    return NULL;

  self = g_slice_new0 (IdeShortcutChord);

  /* Parse each section from the accelerator */
  for (guint i = 0; parts[i]; i++)
    gtk_accelerator_parse (parts[i], &self->keys[i].keyval, &self->keys[i].modifier);

  /* Ensure we got a valid first key at least */
  if (!ide_shortcut_chord_is_valid (self))
    g_clear_pointer (&self, ide_shortcut_chord_free);

  return self;
}

gboolean
ide_shortcut_chord_append_event (IdeShortcutChord  *self,
                                 const GdkEventKey *key)
{
  guint i;

  g_return_val_if_fail (self != NULL, FALSE);
  g_return_val_if_fail (key != NULL, FALSE);

  for (i = 0; i < G_N_ELEMENTS (self->keys); i++)
    {
      if (self->keys[i].keyval == 0)
        {
          self->keys[i].keyval = gdk_keyval_to_lower (key->keyval);
          self->keys[i].modifier = sanitize_modifier_mask (key->state);

          if (self->keys[i].keyval != key->keyval)
            self->keys[i].modifier |= GDK_SHIFT_MASK;

          return TRUE;
        }
    }

  return FALSE;
}

static inline gboolean
ide_shortcut_key_equal (const IdeShortcutKey *keya,
                        const IdeShortcutKey *keyb)
{
  if (keya == keyb)
    return TRUE;
  else if (keya == NULL || keyb == NULL)
    return FALSE;

  return memcmp (keya, keyb, sizeof *keya) == 0;
}

static inline guint
ide_shortcut_chord_count_keys (const IdeShortcutChord *self)
{
  guint count = 0;

  for (guint i = 0; i < G_N_ELEMENTS (self->keys); i++)
    {
      if (self->keys[i].keyval != 0)
        count++;
      else
        break;
    }

  return count;
}

IdeShortcutMatch
ide_shortcut_chord_match (const IdeShortcutChord *self,
                          const IdeShortcutChord *other)
{
  guint self_count = 0;
  guint other_count = 0;

  g_return_val_if_fail (self != NULL, IDE_SHORTCUT_MATCH_NONE);
  g_return_val_if_fail (other != NULL, IDE_SHORTCUT_MATCH_NONE);

  self_count = ide_shortcut_chord_count_keys (self);
  other_count = ide_shortcut_chord_count_keys (other);

  if (self_count > other_count)
    return IDE_SHORTCUT_MATCH_NONE;

  if (0 == memcmp (self->keys, other->keys, sizeof (IdeShortcutKey) * self_count))
    return self_count == other_count ? IDE_SHORTCUT_MATCH_EQUAL : IDE_SHORTCUT_MATCH_PARTIAL;

  return IDE_SHORTCUT_MATCH_NONE;
}

gchar *
ide_shortcut_chord_to_string (const IdeShortcutChord *self)
{
  GString *str;

  if (self == NULL || self->keys[0].keyval == 0)
    return NULL;

  str = g_string_new (NULL);

  for (guint i = 0; i < G_N_ELEMENTS (self->keys); i++)
    {
      const IdeShortcutKey *key = &self->keys[i];
      g_autofree gchar *name = NULL;

      if (key->keyval == 0 && key->modifier == 0)
        break;

      name = gtk_accelerator_name (key->keyval, key->modifier);

      if (i != 0)
        g_string_append_c (str, '|');

      g_string_append (str, name);
    }

  return g_string_free (str, FALSE);
}

gchar *
ide_shortcut_chord_get_label (const IdeShortcutChord *self)
{
  GString *str;

  if (self == NULL || self->keys[0].keyval == 0)
    return NULL;

  str = g_string_new (NULL);

  for (guint i = 0; i < G_N_ELEMENTS (self->keys); i++)
    {
      const IdeShortcutKey *key = &self->keys[i];
      g_autofree gchar *name = NULL;

      if (key->keyval == 0 && key->modifier == 0)
        break;

      name = gtk_accelerator_get_label (key->keyval, key->modifier);

      if (i != 0)
        g_string_append_c (str, ' ');

      g_string_append (str, name);
    }

  return g_string_free (str, FALSE);
}

IdeShortcutChord *
ide_shortcut_chord_copy (const IdeShortcutChord *self)
{
  IdeShortcutChord *copy;

  if (self == NULL)
    return NULL;

  copy = g_slice_new (IdeShortcutChord);
  memcpy (copy, self, sizeof *copy);

  return copy;
}

guint
ide_shortcut_chord_hash (gconstpointer data)
{
  const IdeShortcutChord *self = data;
  guint hash = 0;

  for (guint i = 0; i < G_N_ELEMENTS (self->keys); i++)
    {
      const IdeShortcutKey *key = &self->keys[i];

      hash ^= key->keyval;
      hash ^= key->modifier;
    }

  return hash;
}

gboolean
ide_shortcut_chord_equal (gconstpointer data1,
                          gconstpointer data2)
{
  if (data1 == data2)
    return TRUE;
  else if (data1 == NULL || data2 == NULL)
    return FALSE;

  return 0 == memcmp (((const IdeShortcutChord *)data1)->keys,
                      ((const IdeShortcutChord *)data2)->keys,
                      sizeof (IdeShortcutChord));
}

void
ide_shortcut_chord_free (IdeShortcutChord *self)
{
  if (self != NULL)
    g_slice_free (IdeShortcutChord, self);
}

GType
ide_shortcut_match_get_type (void)
{
  static GType type_id;

  if (g_once_init_enter (&type_id))
    {
      static GEnumValue values[] = {
        { IDE_SHORTCUT_MATCH_NONE, "IDE_SHORTCUT_MATCH_NONE", "none" },
        { IDE_SHORTCUT_MATCH_EQUAL, "IDE_SHORTCUT_MATCH_EQUAL", "equal" },
        { IDE_SHORTCUT_MATCH_PARTIAL, "IDE_SHORTCUT_MATCH_PARTIAL", "partial" },
        { 0 }
      };
      GType _type_id = g_enum_register_static ("IdeShortcutMatch", values);
      g_once_init_leave (&type_id, _type_id);
    }

  return type_id;
}

static gint
ide_shortcut_chord_table_sort (gconstpointer a,
                               gconstpointer b)
{
  const IdeShortcutChordTableEntry *keya = a;
  const IdeShortcutChordTableEntry *keyb = b;

  return ide_shortcut_chord_compare (&keya->chord, &keyb->chord);
}

/**
 * ide_shortcut_chord_table_new: (skip)
 */
IdeShortcutChordTable *
ide_shortcut_chord_table_new (void)
{
  IdeShortcutChordTable *table;

  table = g_slice_new0 (IdeShortcutChordTable);
  table->len = 0;
  table->size = 4;
  table->destroy = NULL;
  table->entries = g_new0 (IdeShortcutChordTableEntry, table->size);

  return table;
}

void
ide_shortcut_chord_table_free (IdeShortcutChordTable *self)
{
  if (self != NULL)
    {
      if (self->destroy != NULL)
        {
          for (guint i = 0; i < self->len; i++)
            self->destroy (self->entries[i].data);
        }
      g_free (self->entries);
      g_slice_free (IdeShortcutChordTable, self);
    }
}

void
ide_shortcut_chord_table_add (IdeShortcutChordTable  *self,
                              const IdeShortcutChord *chord,
                              gpointer                data)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (chord != NULL);

  if (self->len == self->size)
    {
      self->size *= 2;
      self->entries = g_renew (IdeShortcutChordTableEntry, self->entries, self->size);
    }

  self->entries[self->len].chord = *chord;
  self->entries[self->len].data = data;

  self->len++;

  qsort (self->entries,
         self->len,
         sizeof (IdeShortcutChordTableEntry),
         ide_shortcut_chord_table_sort);
}

gboolean
ide_shortcut_chord_table_remove (IdeShortcutChordTable  *self,
                                 const IdeShortcutChord *chord)
{
  g_return_val_if_fail (self != NULL, FALSE);

  for (guint i = 0; i < self->len; i++)
    {
      IdeShortcutChordTableEntry *ele = &self->entries[i];

      if (ide_shortcut_chord_equal (&ele->chord, chord))
        {
          gpointer data = ele->data;

          if (i + 1 < self->len)
            memmove (ele, ele + 1, self->len - i - 1);

          self->len--;

          if (self->destroy != NULL)
            self->destroy (data);

          return TRUE;
        }
    }

  return FALSE;
}

static gint
ide_shortcut_chord_find_partial (gconstpointer a,
                                 gconstpointer b)
{
  const IdeShortcutChord *key = a;
  const IdeShortcutChordTableEntry *element = b;

  /*
   * We are only looking for a partial match here so that we can walk backwards
   * after the bsearch to the first partial match.
   */
  if (ide_shortcut_chord_match (key, &element->chord) != IDE_SHORTCUT_MATCH_NONE)
    return 0;

  return ide_shortcut_chord_compare (key, &element->chord);
}

IdeShortcutMatch
ide_shortcut_chord_table_lookup (IdeShortcutChordTable  *self,
                                 const IdeShortcutChord *chord,
                                 gpointer               *data)
{
  const IdeShortcutChordTableEntry *match;

  g_return_val_if_fail (self != NULL, IDE_SHORTCUT_MATCH_NONE);
  g_return_val_if_fail (chord != NULL, IDE_SHORTCUT_MATCH_NONE);

  if (data != NULL)
    *data = NULL;

  if (self->len == 0)
    return IDE_SHORTCUT_MATCH_NONE;

  /*
   * This function works by performing a binary search to locate ourself
   * somewhere within a match zone of the array. Once we are there, we walk
   * back to the first item that is a partial match.  After that, we walk
   * through every potential match looking for an exact match until we reach a
   * non-partial-match or the end of the array.
   *
   * Based on our findings, we return the appropriate IdeShortcutMatch.
   */

  match = bsearch (chord, self->entries, self->len, sizeof (IdeShortcutChordTableEntry),
                   ide_shortcut_chord_find_partial);

  if (match != NULL)
    {
      const IdeShortcutChordTableEntry *begin = self->entries;
      const IdeShortcutChordTableEntry *end = self->entries + self->len;
      IdeShortcutMatch ret = IDE_SHORTCUT_MATCH_PARTIAL;

      /* Find the first patial match */
      while ((match - 1) >= begin &&
             ide_shortcut_chord_match (chord, &(match - 1)->chord) != IDE_SHORTCUT_MATCH_NONE)
        match--;

      g_assert (match >= begin);

      /* Now walk forward to see if we have an exact match */
      while (IDE_SHORTCUT_MATCH_NONE != (ret = ide_shortcut_chord_match (chord, &match->chord)))
        {
          if (ret == IDE_SHORTCUT_MATCH_EQUAL)
            {
              if (data != NULL)
                *data = match->data;
              return IDE_SHORTCUT_MATCH_EQUAL;
            }

          match++;

          g_assert (match <= end);

          if (ret == 0 || match == end)
            break;
        }

      return IDE_SHORTCUT_MATCH_PARTIAL;
    }

  return IDE_SHORTCUT_MATCH_NONE;
}

void
ide_shortcut_chord_table_set_free_func (IdeShortcutChordTable *self,
                                        GDestroyNotify         destroy)
{
  g_return_if_fail (self != NULL);

  self->destroy = destroy;
}

guint
ide_shortcut_chord_table_size (const IdeShortcutChordTable *self)
{
  return self ? self->len : 0;
}

void
ide_shortcut_chord_table_printf (const IdeShortcutChordTable *self)
{
  if (self == NULL)
    return;

  for (guint i = 0; i < self->len; i++)
    {
      const IdeShortcutChordTableEntry *entry = &self->entries[i];
      g_autofree gchar *str = ide_shortcut_chord_to_string (&entry->chord);

      g_print ("%s\n", str);
    }
}

void
_ide_shortcut_chord_table_iter_init (IdeShortcutChordTableIter *iter,
                                     IdeShortcutChordTable     *table)
{
  g_return_if_fail (iter != NULL);

  iter->table = table;
  iter->position = 0;
}

gboolean
_ide_shortcut_chord_table_iter_next (IdeShortcutChordTableIter  *iter,
                                     const IdeShortcutChord    **chord,
                                     gpointer                   *value)
{
  g_return_val_if_fail (iter != NULL, FALSE);

  if (iter->table == NULL)
    return FALSE;

  if (iter->position < iter->table->len)
    {
      *chord = &iter->table->entries[iter->position].chord;
      *value = iter->table->entries[iter->position].data;
      iter->position++;
      return TRUE;
    }

  return FALSE;
}

gboolean
ide_shortcut_chord_has_modifier (const IdeShortcutChord *self)
{
  g_return_val_if_fail (self != NULL, FALSE);

  return self->keys[0].modifier != 0;
}

guint
ide_shortcut_chord_get_length (const IdeShortcutChord *self)
{
  if (self != NULL)
    {
      for (guint i = 0; i < G_N_ELEMENTS (self->keys); i++)
        {
          if (self->keys[i].keyval == 0)
            return i;
        }

      return G_N_ELEMENTS (self->keys);
    }

  return 0;
}

void
ide_shortcut_chord_get_nth_key (const IdeShortcutChord *self,
                                guint                   nth,
                                guint                  *keyval,
                                GdkModifierType        *modifier)
{
  if (nth < G_N_ELEMENTS (self->keys))
    {
      if (keyval)
        *keyval = self->keys[nth].keyval;
      if (modifier)
        *modifier = self->keys[nth].modifier;
    }
  else
    {
      if (keyval)
        *keyval = 0;
      if (modifier)
        *modifier = 0;
    }
}
