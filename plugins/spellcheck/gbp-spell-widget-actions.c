/* gbp-spell-widget-actions.c
 *
 * Copyright (C) 2016 Sebastien Lafargue <slafargue@gnome.org>
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

#define G_LOG_DOMAIN "gbp-spell-widget-actions"

#include "gbp-spell-dict.h"
#include "gbp-spell-private.h"

static void
gbp_spell_widget_actions_change (GSimpleAction *action,
                                 GVariant      *param,
                                 gpointer       user_data)
{
  GbpSpellWidget *self = user_data;

  g_assert (G_IS_SIMPLE_ACTION (action));
  g_assert (GBP_IS_SPELL_WIDGET (self));

  _gbp_spell_widget_change (self, FALSE);
}

static void
gbp_spell_widget_actions_change_all (GSimpleAction *action,
                                     GVariant      *param,
                                     gpointer       user_data)
{
  GbpSpellWidget *self = user_data;

  g_assert (G_IS_SIMPLE_ACTION (action));
  g_assert (GBP_IS_SPELL_WIDGET (self));

  _gbp_spell_widget_change (self, TRUE);
}

static void
gbp_spell_widget_actions_ignore (GSimpleAction *action,
                                 GVariant      *param,
                                 gpointer       user_data)
{
  GbpSpellWidget *self = user_data;

  g_assert (G_IS_SIMPLE_ACTION (action));
  g_assert (GBP_IS_SPELL_WIDGET (self));

  _gbp_spell_widget_move_next_word (self);
}

static void
gbp_spell_widget_actions_ignore_all (GSimpleAction *action,
                                     GVariant      *param,
                                     gpointer       user_data)
{
  GbpSpellWidget *self = user_data;

  g_assert (G_IS_SIMPLE_ACTION (action));
  g_assert (GBP_IS_SPELL_WIDGET (self));

  if (self->editor_view_addin != NULL)
    {
      GspellChecker *checker;
      const gchar *word;

      checker = gbp_spell_editor_view_addin_get_checker (self->editor_view_addin);
      word = gtk_label_get_text (self->word_label);

      if (!ide_str_empty0 (word))
        {
          gspell_checker_add_word_to_session (checker, word, -1);
          _gbp_spell_widget_move_next_word (self);
        }
    }
}

static void
gbp_spell_widget_actions_move_next_word (GSimpleAction *action,
                                         GVariant      *param,
                                         gpointer       user_data)
{
  GbpSpellWidget *self = user_data;

  g_assert (G_IS_SIMPLE_ACTION (action));
  g_assert (GBP_IS_SPELL_WIDGET (self));

  _gbp_spell_widget_move_next_word (self);
}

static const GActionEntry actions[] = {
  { "change", gbp_spell_widget_actions_change },
  { "change-all", gbp_spell_widget_actions_change_all },
  { "ignore", gbp_spell_widget_actions_ignore },
  { "ignore-all", gbp_spell_widget_actions_ignore_all },
  { "move-next-word", gbp_spell_widget_actions_move_next_word },
};

void
_gbp_spell_widget_init_actions (GbpSpellWidget *self)
{
  g_autoptr(GSimpleActionGroup) group = NULL;

  g_return_if_fail (GBP_IS_SPELL_WIDGET (self));

  group = g_simple_action_group_new ();
  g_action_map_add_action_entries (G_ACTION_MAP (group), actions, G_N_ELEMENTS (actions), self);
  gtk_widget_insert_action_group (GTK_WIDGET (self), "spell-widget", G_ACTION_GROUP (group));
}

void
_gbp_spell_widget_update_actions (GbpSpellWidget *self)
{
  gboolean can_change = FALSE;
  gboolean can_change_all = FALSE;
  gboolean can_ignore = FALSE;
  gboolean can_ignore_all = FALSE;
  gboolean can_move_next_word = FALSE;

  g_return_if_fail (GBP_IS_SPELL_WIDGET (self));

  if (IDE_IS_EDITOR_VIEW (self->editor) &&
      GBP_IS_SPELL_EDITOR_VIEW_ADDIN (self->editor_view_addin) &&
      self->spellchecking_status == TRUE)
    {
      g_assert (IDE_IS_EDITOR_VIEW_ADDIN (self->editor_view_addin));

      can_change = TRUE;
      can_change_all = TRUE;
      can_move_next_word = TRUE;

      can_ignore = self->current_word_count > 0;
      can_ignore_all = self->current_word_count > 1;
    }

  dzl_gtk_widget_action_set (GTK_WIDGET (self), "spell-widget", "change",
                             "enabled", can_change,
                             NULL);
  dzl_gtk_widget_action_set (GTK_WIDGET (self), "spell-widget", "change-all",
                             "enabled", can_change_all,
                             NULL);
  dzl_gtk_widget_action_set (GTK_WIDGET (self), "spell-widget", "ignore",
                             "enabled", can_ignore,
                             NULL);
  dzl_gtk_widget_action_set (GTK_WIDGET (self), "spell-widget", "ignore-all",
                             "enabled", can_ignore_all,
                             NULL);
  dzl_gtk_widget_action_set (GTK_WIDGET (self), "spell-widget", "move-next-word",
                             "enabled", can_move_next_word,
                             NULL);
}
