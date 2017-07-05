/* ide-editor-spell-widget.c
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

#define G_LOG_DOMAIN "ide-spell-check-widget"

#include <dazzle.h>
#include <glib/gi18n.h>
#include <gspell/gspell.h>

#include "ide-debug.h"
#include "buffers/ide-buffer.h"
#include "spellcheck/ide-editor-spell-dict.h"
#include "spellcheck/ide-editor-spell-language-popover.h"
#include "spellcheck/ide-editor-spell-navigator.h"
#include "spellcheck/ide-editor-spell-widget.h"
#include "util/ide-gtk.h"

typedef enum
{
  CHECK_WORD_NONE,
  CHECK_WORD_CHECKING,
  CHECK_WORD_IDLE
} CheckWordState;

struct _IdeEditorSpellWidget
{
  GtkBin                 parent_instance;

  GspellNavigator       *navigator;
  IdeSourceView         *view;
  IdeBuffer             *buffer;
  GspellChecker         *checker;
  IdeEditorSpellDict    *dict;
  GPtrArray             *words_array;
  const GspellLanguage  *spellchecker_language;

  GtkLabel              *word_label;
  GtkLabel              *count_label;
  GtkEntry              *word_entry;
  GtkButton             *ignore_button;
  GtkButton             *ignore_all_button;
  GtkButton             *change_button;
  GtkButton             *change_all_button;
  GtkListBox            *suggestions_box;
  GtkBox                *count_box;

  GtkWidget             *dict_word_entry;
  GtkWidget             *dict_add_button;
  GtkWidget             *dict_words_list;

  GtkButton             *highlight_switch;
  GtkButton             *language_chooser_button;

  GtkWidget             *placeholder;
  GAction               *view_spellchecking_action;

  guint                  current_word_count;
  guint                  check_word_timeout_id;
  guint                  dict_check_word_timeout_id;
  CheckWordState         check_word_state;
  CheckWordState         dict_check_word_state;

  guint                  view_spellchecker_set : 1;

  guint                  is_checking_word : 1;
  guint                  is_check_word_invalid : 1;
  guint                  is_check_word_idle : 1;
  guint                  is_word_entry_valid : 1;

  guint                  is_dict_checking_word : 1;
  guint                  is_dict_check_word_invalid : 1;
  guint                  is_dict_check_word_idle : 1;

  guint                  spellchecking_status : 1;
};

G_DEFINE_TYPE (IdeEditorSpellWidget, ide_editor_spell_widget, GTK_TYPE_BIN)

#define CHECK_WORD_INTERVAL_MIN 100
#define DICT_CHECK_WORD_INTERVAL_MIN 100
#define WORD_ENTRY_MAX_SUGGESTIONS 6

enum {
  PROP_0,
  PROP_VIEW,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

static void
clear_suggestions_box (IdeEditorSpellWidget *self)
{
  GList *children;

  g_assert (IDE_IS_EDITOR_SPELL_WIDGET (self));

  children = gtk_container_get_children (GTK_CONTAINER (self->suggestions_box));

  for (GList *l = children; l != NULL; l = g_list_next (l))
    gtk_widget_destroy (GTK_WIDGET (l->data));
}

static void
update_global_sensiblility (IdeEditorSpellWidget *self,
                            gboolean              sensibility)
{
  g_assert (IDE_IS_EDITOR_SPELL_WIDGET (self));

  gtk_entry_set_text (self->word_entry, "");
  clear_suggestions_box (self);

  gtk_widget_set_sensitive (GTK_WIDGET (self->word_entry), sensibility);
  gtk_widget_set_sensitive (GTK_WIDGET (self->ignore_button), sensibility);
  gtk_widget_set_sensitive (GTK_WIDGET (self->ignore_all_button), sensibility);
  gtk_widget_set_sensitive (GTK_WIDGET (self->change_button), sensibility);
  gtk_widget_set_sensitive (GTK_WIDGET (self->change_all_button), sensibility);
  gtk_widget_set_sensitive (GTK_WIDGET (self->suggestions_box), sensibility);
}

static void
update_change_ignore_sensibility (IdeEditorSpellWidget *self)
{
  gboolean entry_sensitivity;

  g_assert (IDE_IS_EDITOR_SPELL_WIDGET (self));

  entry_sensitivity = (gtk_entry_get_text_length (self->word_entry) > 0);

  gtk_widget_set_sensitive (GTK_WIDGET (self->change_button),
                            entry_sensitivity);
  gtk_widget_set_sensitive (GTK_WIDGET (self->change_all_button),
                            entry_sensitivity && (self->current_word_count > 1));

  gtk_widget_set_sensitive (GTK_WIDGET (self->ignore_all_button),
                            self->current_word_count > 1);
}

GtkWidget *
ide_editor_spell_widget_get_entry (IdeEditorSpellWidget *self)
{
  g_return_val_if_fail (IDE_IS_EDITOR_SPELL_WIDGET (self), NULL);

  return GTK_WIDGET (self->word_entry);
}

static GtkWidget *
create_suggestion_row (IdeEditorSpellWidget *self,
                       const gchar          *word)
{
  GtkWidget *row;

  g_assert (IDE_IS_EDITOR_SPELL_WIDGET (self));
  g_assert (!ide_str_empty0 (word));

  row = g_object_new (GTK_TYPE_LABEL,
                      "label", word,
                      "visible", TRUE,
                      "halign", GTK_ALIGN_START,
                      NULL);

  return row;
}

static void
fill_suggestions_box (IdeEditorSpellWidget *self,
                      const gchar          *word,
                      gchar               **first_result)
{
  GSList *suggestions = NULL;
  GtkWidget *item;

  g_assert (IDE_IS_EDITOR_SPELL_WIDGET (self));
  g_assert (first_result != NULL);

  *first_result = NULL;

  clear_suggestions_box (self);
  if (ide_str_empty0 (word))
    {
      gtk_widget_set_sensitive (GTK_WIDGET (self->suggestions_box), FALSE);
      return;
    }

  if (NULL == (suggestions = gspell_checker_get_suggestions (self->checker, word, -1)))
    {
      gtk_label_set_text (GTK_LABEL (self->placeholder), _("No suggestions"));
      gtk_widget_set_sensitive (GTK_WIDGET (self->suggestions_box), FALSE);
    }
  else
    {
      *first_result = g_strdup (suggestions->data);
      gtk_widget_set_sensitive (GTK_WIDGET (self->suggestions_box), TRUE);
      for (GSList *l = (GSList *)suggestions; l != NULL; l = l->next)
        {
          item = create_suggestion_row (self, l->data);
          gtk_list_box_insert (self->suggestions_box, item, -1);
        }

      g_slist_free_full (suggestions, g_free);
    }
}

static void
update_count_label (IdeEditorSpellWidget *self)
{
  const gchar *word;
  guint count;

  g_assert (IDE_IS_EDITOR_SPELL_WIDGET (self));

  word = gtk_label_get_text (self->word_label);
  if (0 != (count = ide_editor_spell_navigator_get_count (IDE_EDITOR_SPELL_NAVIGATOR (self->navigator), word)))
    {
      g_autofree gchar *count_text = NULL;

      if (count > 1000)
        count_text = g_strdup (">1000");
      else
        count_text = g_strdup_printf ("%i", count);

      gtk_label_set_text (self->count_label, count_text);
      gtk_widget_set_visible (GTK_WIDGET (self->count_box), TRUE);
    }
  else
    gtk_widget_set_visible (GTK_WIDGET (self->count_box), TRUE);

  self->current_word_count = count;
  update_change_ignore_sensibility (self);
}

static gboolean
jump_to_next_misspelled_word (IdeEditorSpellWidget *self)
{
  GspellChecker *checker = NULL;
  g_autofree gchar *word = NULL;
  g_autofree gchar *first_result = NULL;
  GtkListBoxRow *row;
  g_autoptr(GError) error = NULL;
  gboolean ret = FALSE;

  g_assert (IDE_IS_EDITOR_SPELL_WIDGET (self));

  gtk_widget_grab_focus (GTK_WIDGET (self->word_entry));
  if ((ret = gspell_navigator_goto_next (self->navigator, &word, &checker, &error)))
    {
      gtk_label_set_text (self->word_label, word);
      update_count_label (self);

      fill_suggestions_box (self, word, &first_result);
      if (!ide_str_empty0 (first_result))
        {
          row = gtk_list_box_get_row_at_index (self->suggestions_box, 0);
          gtk_list_box_select_row (self->suggestions_box, row);
        }
    }
  else
    {
      if (error != NULL)
        gtk_label_set_text (GTK_LABEL (self->placeholder), error->message);

      self->spellchecking_status = FALSE;

      gtk_label_set_text (GTK_LABEL (self->placeholder), _("Completed spell checking"));
      gtk_widget_grab_focus (self->dict_word_entry);
      update_global_sensiblility (self, FALSE);
    }

  return ret;
}

GtkWidget *
ide_editor_spell_widget_new (IdeSourceView *source_view)
{
  return g_object_new (IDE_TYPE_EDITOR_SPELL_WIDGET,
                       "view", source_view,
                       NULL);
}

static IdeSourceView *
ide_editor_spell_widget_get_view (IdeEditorSpellWidget *self)
{
  g_return_val_if_fail (IDE_IS_EDITOR_SPELL_WIDGET (self), NULL);

  return self->view;
}

static void
ide_editor_spell_widget_set_view (IdeEditorSpellWidget *self,
                                  IdeSourceView        *view)
{
  g_return_if_fail (IDE_IS_EDITOR_SPELL_WIDGET (self));
  g_return_if_fail (IDE_IS_SOURCE_VIEW (view));

  ide_set_weak_pointer (&self->view, view);
  if (GSPELL_IS_NAVIGATOR (self->navigator))
    g_clear_object (&self->navigator);

  self->navigator = ide_editor_spell_navigator_new (GTK_TEXT_VIEW (view));
}

static gboolean
check_word_timeout_cb (IdeEditorSpellWidget *self)
{
  const gchar *word;
  g_autoptr(GError) error = NULL;
  gchar *icon_name;
  gboolean ret = TRUE;

  g_assert (IDE_IS_EDITOR_SPELL_WIDGET (self));

  self->check_word_state = CHECK_WORD_CHECKING;

  word = gtk_entry_get_text (self->word_entry);
  if (!ide_str_empty0 (word))
    {
      /* FIXME: suggestions can give a multiple-words suggestion
       * that failed to the checkword test, ex: auto tools
       */
      ret = gspell_checker_check_word (self->checker, word, -1, &error);
      if (error != NULL)
        {
          g_message ("check error:%s\n", error->message);
        }

      icon_name = ret ? "" : "dialog-warning-symbolic";
    }
  else
    icon_name = "";

  gtk_entry_set_icon_from_icon_name (self->word_entry,
                                     GTK_ENTRY_ICON_SECONDARY,
                                     icon_name);

  self->check_word_state = CHECK_WORD_NONE;
  self->is_word_entry_valid = ret;

  self->check_word_timeout_id = 0;
  if (self->is_check_word_invalid == TRUE)
    {
      self->check_word_timeout_id = g_timeout_add_full (G_PRIORITY_DEFAULT,
                                                        CHECK_WORD_INTERVAL_MIN,
                                                        (GSourceFunc)check_word_timeout_cb,
                                                        self,
                                                        NULL);
      self->check_word_state = CHECK_WORD_IDLE;
      self->is_check_word_invalid = FALSE;
    }

  return G_SOURCE_REMOVE;
}

static void
ide_editor_spell_widget__word_entry_changed_cb (IdeEditorSpellWidget *self,
                                                GtkEntry             *entry)
{
  const gchar *word;

  g_assert (IDE_IS_EDITOR_SPELL_WIDGET (self));
  g_assert (GTK_IS_ENTRY (entry));

  update_change_ignore_sensibility (self);

  word = gtk_entry_get_text (self->word_entry);
  if (ide_str_empty0 (word) && self->spellchecking_status == TRUE)
    {
      word = gtk_label_get_text (self->word_label);
      gtk_entry_set_text (GTK_ENTRY (self->dict_word_entry), word);
    }
  else
    gtk_entry_set_text (GTK_ENTRY (self->dict_word_entry), word);

  if (self->check_word_state == CHECK_WORD_CHECKING)
    {
      self->is_check_word_invalid = TRUE;
      return;
    }

  if (self->check_word_state == CHECK_WORD_IDLE)
    {
      g_source_remove (self->check_word_timeout_id);
      self->check_word_timeout_id = 0;
    }

  self->check_word_timeout_id = g_timeout_add_full (G_PRIORITY_DEFAULT,
                                                    CHECK_WORD_INTERVAL_MIN,
                                                    (GSourceFunc)check_word_timeout_cb,
                                                    self,
                                                    NULL);
  self->check_word_state = CHECK_WORD_IDLE;
}

static void
ide_editor_spell_widget__ignore_button_clicked_cb (IdeEditorSpellWidget *self,
                                                   GtkButton            *button)
{
  g_assert (IDE_IS_EDITOR_SPELL_WIDGET (self));
  g_assert (GTK_IS_BUTTON (button));

  jump_to_next_misspelled_word (self);
}

static void
ide_editor_spell_widget__ignore_all_button_clicked_cb (IdeEditorSpellWidget *self,
                                                       GtkButton            *button)
{
  const gchar *word;

  g_assert (IDE_IS_EDITOR_SPELL_WIDGET (self));
  g_assert (GTK_IS_BUTTON (button));

  word = gtk_label_get_text (self->word_label);
  g_assert (!ide_str_empty0 (word));

  gspell_checker_add_word_to_session (self->checker, word, -1);
  jump_to_next_misspelled_word (self);
}

static void
change_misspelled_word (IdeEditorSpellWidget *self,
                        gboolean              change_all)
{
  const gchar *word;
  g_autofree gchar *change_to = NULL;

  g_assert (IDE_IS_EDITOR_SPELL_WIDGET (self));

  word = gtk_label_get_text (self->word_label);
  g_assert (!ide_str_empty0 (word));

  change_to = g_strdup (gtk_entry_get_text (self->word_entry));
  g_assert (!ide_str_empty0 (change_to));

  gspell_checker_set_correction (self->checker, word, -1, change_to, -1);

  if (change_all)
    gspell_navigator_change_all (self->navigator, word, change_to);
  else
    gspell_navigator_change (self->navigator, word, change_to);

  jump_to_next_misspelled_word (self);
}

static void
ide_editor_spell_widget__change_button_clicked_cb (IdeEditorSpellWidget *self,
                                                   GtkButton            *button)
{
  g_assert (IDE_IS_EDITOR_SPELL_WIDGET (self));
  g_assert (GTK_IS_BUTTON (button));

  change_misspelled_word (self, FALSE);
}

static void
ide_editor_spell_widget__change_all_button_clicked_cb (IdeEditorSpellWidget *self,
                                                       GtkButton            *button)
{
  g_assert (IDE_IS_EDITOR_SPELL_WIDGET (self));
  g_assert (GTK_IS_BUTTON (button));

  change_misspelled_word (self, TRUE);
}

static void
ide_editor_spell_widget__row_selected_cb (IdeEditorSpellWidget *self,
                                          GtkListBoxRow        *row,
                                          GtkListBox           *listbox)
{
  const gchar *word;
  GtkLabel *label;

  g_assert (IDE_IS_EDITOR_SPELL_WIDGET (self));
  g_assert (GTK_IS_LIST_BOX_ROW (row) || row == NULL);
  g_assert (GTK_IS_LIST_BOX (listbox));

  if (row != NULL)
    {
      label = GTK_LABEL (gtk_bin_get_child (GTK_BIN (row)));
      word = gtk_label_get_text (label);

      g_signal_handlers_block_by_func (self->word_entry, ide_editor_spell_widget__word_entry_changed_cb, self);

      gtk_entry_set_text (self->word_entry, word);
      gtk_editable_set_position (GTK_EDITABLE (self->word_entry), -1);
      update_change_ignore_sensibility (self);

      g_signal_handlers_unblock_by_func (self->word_entry, ide_editor_spell_widget__word_entry_changed_cb, self);
    }
}

static void
ide_editor_spell_widget__row_activated_cb (IdeEditorSpellWidget *self,
                                           GtkListBoxRow        *row,
                                           GtkListBox           *listbox)
{
  g_assert (IDE_IS_EDITOR_SPELL_WIDGET (self));
  g_assert (GTK_IS_LIST_BOX_ROW (row));
  g_assert (GTK_IS_LIST_BOX (listbox));

  if (row != NULL)
    change_misspelled_word (self, FALSE);
}

static gboolean
ide_editor_spell_widget__key_press_event_cb (IdeEditorSpellWidget *self,
                                             GdkEventKey          *event)
{
  g_assert (IDE_IS_SOURCE_VIEW (self->view));
  g_assert (event != NULL);

  switch (event->keyval)
    {
    case GDK_KEY_Escape:
      dzl_gtk_widget_action (GTK_WIDGET (self->view),
                         "frame", "show-spellcheck",
                         g_variant_new_int32 (0));
      return GDK_EVENT_STOP;

    default:
      break;
    }

  return GDK_EVENT_PROPAGATE;
}

static void
ide_editor_spell__widget_mapped_cb (IdeEditorSpellWidget *self)
{
  GActionGroup *group = NULL;
  GtkWidget *widget = GTK_WIDGET (self->view);
  g_autoptr (GVariant) value = NULL;

  g_assert (IDE_IS_EDITOR_SPELL_WIDGET (self));

  while ((group == NULL) && (widget != NULL))
    {
      group = gtk_widget_get_action_group (widget, "view");
      widget = gtk_widget_get_parent (widget);
    }

  if (group != NULL &&
      NULL != (self->view_spellchecking_action = g_action_map_lookup_action (G_ACTION_MAP (group),
                                                                             "spellchecking")))
    {
      value = g_action_get_state (self->view_spellchecking_action);
      self->view_spellchecker_set = g_variant_get_boolean (value);
      gtk_switch_set_active (GTK_SWITCH (self->highlight_switch), self->view_spellchecker_set);
    }

  jump_to_next_misspelled_word (self);
}

static void
ide_editor_spell_widget__highlight_switch_toggled_cb (IdeEditorSpellWidget *self,
                                                      gboolean              state,
                                                      GtkSwitch            *switch_button)
{
  GspellTextView *spell_text_view;

  g_assert (IDE_IS_EDITOR_SPELL_WIDGET (self));
  g_assert (GTK_IS_SWITCH (switch_button));

  gtk_switch_set_state (switch_button, state);
  spell_text_view = gspell_text_view_get_from_gtk_text_view (GTK_TEXT_VIEW (self->view));
  gspell_text_view_set_inline_spell_checking (spell_text_view, state);
}

static void
ide_editor_spell_widget__words_counted_cb (IdeEditorSpellWidget *self,
                                           GParamSpec           *pspec,
                                           GspellNavigator      *navigator)
{
  g_assert (IDE_IS_EDITOR_SPELL_WIDGET (self));
  g_assert (GSPELL_IS_NAVIGATOR (navigator));

  update_count_label (self);
}

static GtkListBoxRow *
get_next_row_to_focus (GtkListBox    *listbox,
                       GtkListBoxRow *row)
{
  g_autoptr(GList) children = NULL;
  gint index;
  gint new_index;
  gint len;

  g_assert (GTK_IS_LIST_BOX (listbox));
  g_assert (GTK_IS_LIST_BOX_ROW (row));

  children = gtk_container_get_children (GTK_CONTAINER (listbox));
  if (0 == (len = g_list_length (children)))
    return NULL;

  index = gtk_list_box_row_get_index (row);
  if (index < len - 1)
    new_index = index + 1;
  else if (index == len - 1 && len > 1)
    new_index = index - 1;
  else
    return NULL;

  return gtk_list_box_get_row_at_index (listbox, new_index);
}

static gboolean
dict_check_word_timeout_cb (IdeEditorSpellWidget *self)
{
  const gchar *word;
  g_autofree gchar *tooltip = NULL;
  gchar *icon_name;
  gboolean valid = FALSE;

  g_assert (IDE_IS_EDITOR_SPELL_WIDGET (self));

  self->dict_check_word_state = CHECK_WORD_CHECKING;

  word = gtk_entry_get_text (GTK_ENTRY (self->dict_word_entry));
  if (!ide_str_empty0 (word))
    {
      if (ide_editor_spell_dict_personal_contains (self->dict, word))
        gtk_widget_set_tooltip_text (self->dict_word_entry, _("This word is already in the personal dictionary"));
      else if (gspell_checker_check_word (self->checker, word, -1, NULL))
        {
          tooltip = g_strdup_printf (_("This word is already in the %s dictionary"), gspell_language_get_name (self->spellchecker_language));
          gtk_widget_set_tooltip_text (self->dict_word_entry, tooltip);
        }
      else
        {
          valid = TRUE;
          gtk_widget_set_tooltip_text (self->dict_word_entry, NULL);
        }

      icon_name = valid ? "" : "dialog-warning-symbolic";
    }
  else
    icon_name = "";

  gtk_widget_set_sensitive (GTK_WIDGET (self->dict_add_button), valid);
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (self->dict_word_entry),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     icon_name);

  self->dict_check_word_state = CHECK_WORD_NONE;

  self->dict_check_word_timeout_id = 0;
  if (self->is_dict_check_word_invalid == TRUE)
    {
      self->dict_check_word_timeout_id = g_timeout_add_full (G_PRIORITY_DEFAULT,
                                                             DICT_CHECK_WORD_INTERVAL_MIN,
                                                             (GSourceFunc)dict_check_word_timeout_cb,
                                                             self,
                                                             NULL);
      self->dict_check_word_state = CHECK_WORD_IDLE;
      self->is_dict_check_word_invalid = FALSE;
    }

  return G_SOURCE_REMOVE;
}

static void
ide_editor_spell_widget__dict_word_entry_changed_cb (IdeEditorSpellWidget *self,
                                                     GtkEntry             *dict_word_entry)
{
  g_assert (IDE_IS_EDITOR_SPELL_WIDGET (self));
  g_assert (GTK_IS_ENTRY (dict_word_entry));

  if (self->dict_check_word_state == CHECK_WORD_CHECKING)
    {
      self->is_dict_check_word_invalid = TRUE;
      return;
    }

  if (self->dict_check_word_state == CHECK_WORD_IDLE)
    {
      g_source_remove (self->dict_check_word_timeout_id);
      self->dict_check_word_timeout_id = 0;
    }

  self->dict_check_word_timeout_id = g_timeout_add_full (G_PRIORITY_DEFAULT,
                                                         CHECK_WORD_INTERVAL_MIN,
                                                         (GSourceFunc)dict_check_word_timeout_cb,
                                                         self,
                                                         NULL);
  self->dict_check_word_state = CHECK_WORD_IDLE;
}

static void
remove_dict_row (IdeEditorSpellWidget *self,
                 GtkListBox           *listbox,
                 GtkListBoxRow        *row)
{
  GtkListBoxRow *next_row;
  gchar *word;
  gboolean exist;

  g_assert (IDE_IS_EDITOR_SPELL_WIDGET (self));
  g_assert (GTK_IS_LIST_BOX (listbox));
  g_assert (GTK_IS_LIST_BOX_ROW (row));

  word = g_object_get_data (G_OBJECT (row), "word");
  exist = ide_editor_spell_dict_remove_word_from_personal (self->dict, word);
  if (!exist)
    g_warning ("The word %s do not exist in the personnal dictionary", word);

  if (row == gtk_list_box_get_selected_row (listbox))
    {
      if (NULL != (next_row = get_next_row_to_focus (listbox, row)))
        {
          gtk_widget_grab_focus (GTK_WIDGET (next_row));
          gtk_list_box_select_row (listbox, next_row);
        }
      else
        gtk_widget_grab_focus (GTK_WIDGET (self->word_entry));
    }

  gtk_container_remove (GTK_CONTAINER (self->dict_words_list), GTK_WIDGET (row));
  ide_editor_spell_widget__dict_word_entry_changed_cb (self, GTK_ENTRY (self->dict_word_entry));
}

static void
dict_close_button_clicked_cb (IdeEditorSpellWidget *self,
                              GtkButton            *button)
{
  GtkWidget *row;

  g_assert (IDE_IS_EDITOR_SPELL_WIDGET (self));
  g_assert (GTK_IS_BUTTON (button));

  if (NULL != (row = gtk_widget_get_ancestor (GTK_WIDGET (button), GTK_TYPE_LIST_BOX_ROW)))
    remove_dict_row (self, GTK_LIST_BOX (self->dict_words_list), GTK_LIST_BOX_ROW (row));
}

static gboolean
dict_row_key_pressed_event_cb (IdeEditorSpellWidget *self,
                               GdkEventKey          *event,
                               GtkListBox           *listbox)
{
  GtkListBoxRow *row;

  g_assert (IDE_IS_EDITOR_SPELL_WIDGET (self));
  g_assert (event != NULL);
  g_assert (GTK_IS_LIST_BOX (listbox));

  if (NULL != (row = gtk_list_box_get_selected_row (listbox)) &&
      event->keyval == GDK_KEY_Delete)
    {
      remove_dict_row (self, GTK_LIST_BOX (self->dict_words_list), GTK_LIST_BOX_ROW (row));
      return GDK_EVENT_STOP;
    }

  return GDK_EVENT_PROPAGATE;
}

static GtkWidget *
dict_create_word_row (IdeEditorSpellWidget *self,
                      const gchar          *word)
{
  GtkWidget *row;
  GtkWidget *box;
  GtkWidget *label;
  GtkWidget *button;
  GtkStyleContext *style_context;

  g_assert (IDE_IS_EDITOR_SPELL_WIDGET (self));
  g_assert (!ide_str_empty0 (word));

  label = g_object_new (GTK_TYPE_LABEL,
                       "label", word,
                       "halign", GTK_ALIGN_START,
                       NULL);

  button = gtk_button_new_from_icon_name ("window-close-symbolic", GTK_ICON_SIZE_BUTTON);
  gtk_widget_set_can_focus (button, FALSE);
  g_signal_connect_swapped (button,
                            "clicked",
                            G_CALLBACK (dict_close_button_clicked_cb),
                            self);

  style_context = gtk_widget_get_style_context (button);
  gtk_style_context_add_class (style_context, "close");

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 0);
  gtk_box_pack_end (GTK_BOX (box), button, FALSE, FALSE, 0);

  row = gtk_list_box_row_new ();
  gtk_container_add (GTK_CONTAINER (row), box);
  g_object_set_data_full (G_OBJECT (row), "word", g_strdup (word), g_free);
  gtk_widget_show_all (row);

  return row;
}

static gboolean
check_dict_available (IdeEditorSpellWidget *self)
{
  g_assert (IDE_IS_EDITOR_SPELL_WIDGET (self));

  return (self->checker != NULL && self->spellchecker_language != NULL);
}

static void
ide_editor_spell_widget__add_button_clicked_cb (IdeEditorSpellWidget *self,
                                                GtkButton            *button)
{
  const gchar *word;
  GtkWidget *item;
  GtkWidget *toplevel;
  GtkWidget *focused_widget;

  g_assert (IDE_IS_EDITOR_SPELL_WIDGET (self));
  g_assert (GTK_IS_BUTTON (button));

  word = gtk_entry_get_text (GTK_ENTRY (self->dict_word_entry));
  /* TODO: check if word already in dict */
  if (check_dict_available (self) && !ide_str_empty0 (word))
    {
      if (!ide_editor_spell_dict_add_word_to_personal (self->dict, word))
        return;

      item = dict_create_word_row (self, word);
      gtk_list_box_insert (GTK_LIST_BOX (self->dict_words_list), item, 0);

      toplevel = gtk_widget_get_toplevel (GTK_WIDGET (self));
      if (GTK_IS_WINDOW (toplevel) &&
          NULL != (focused_widget = gtk_window_get_focus (GTK_WINDOW (toplevel))))
        {
          if (focused_widget != GTK_WIDGET (self->word_entry) &&
              focused_widget != self->dict_word_entry)
            gtk_widget_grab_focus (self->dict_word_entry);
        }

      gtk_entry_set_text (GTK_ENTRY (self->dict_word_entry), "");

    }
}

static void
dict_clean_listbox (IdeEditorSpellWidget *self)
{
  GList *children;

  g_assert (IDE_IS_EDITOR_SPELL_WIDGET (self));

  children = gtk_container_get_children (GTK_CONTAINER (self->dict_words_list));
  for (GList *l = children; l != NULL; l = g_list_next (l))
    gtk_widget_destroy (GTK_WIDGET (l->data));
}

static void
dict_fill_listbox (IdeEditorSpellWidget *self,
                   GPtrArray            *words_array)
{
  const gchar *word;
  GtkWidget *item;
  guint len;

  g_assert (IDE_IS_EDITOR_SPELL_WIDGET (self));
  g_assert (words_array != NULL);

  dict_clean_listbox (self);

  len = words_array->len;
  for (guint i = 0; i < len; ++i)
    {
      word = g_ptr_array_index (words_array, i);
      item = dict_create_word_row (self, word);
      gtk_list_box_insert (GTK_LIST_BOX (self->dict_words_list), item, -1);
    }
}

static void
ide_editor_spell_widget__language_notify_cb (IdeEditorSpellWidget *self,
                                             GParamSpec           *pspec,
                                             GtkButton            *language_chooser_button)
{
  const GspellLanguage *current_language;
  const GspellLanguage *spell_language;
  g_autofree gchar *word = NULL;
  g_autofree gchar *first_result = NULL;
  GtkListBoxRow *row;

  g_assert (IDE_IS_EDITOR_SPELL_WIDGET (self));
  g_assert (GTK_IS_BUTTON (language_chooser_button));

  current_language = gspell_checker_get_language (self->checker);
  spell_language = gspell_language_chooser_get_language (GSPELL_LANGUAGE_CHOOSER (language_chooser_button));
  if (gspell_language_compare (current_language, spell_language) != 0)
    {
      gspell_checker_set_language (self->checker, spell_language);
      fill_suggestions_box (self, word, &first_result);
      if (!ide_str_empty0 (first_result))
        {
          row = gtk_list_box_get_row_at_index (self->suggestions_box, 0);
          gtk_list_box_select_row (self->suggestions_box, row);
        }

      g_clear_pointer (&self->words_array, g_ptr_array_unref);
      if (current_language == NULL)
        {
          dict_clean_listbox (self);
          gtk_widget_set_sensitive (GTK_WIDGET (self->dict_add_button), FALSE);
          gtk_widget_set_sensitive (GTK_WIDGET (self->dict_words_list), FALSE);

          return;
        }

      ide_editor_spell_widget__dict_word_entry_changed_cb (self, GTK_ENTRY (self->dict_word_entry));
      gtk_widget_set_sensitive (GTK_WIDGET (self->dict_words_list), TRUE);

      ide_editor_spell_navigator_goto_word_start (IDE_EDITOR_SPELL_NAVIGATOR (self->navigator));
      jump_to_next_misspelled_word (self);
    }
}

static void
ide_editor_spell_widget__word_entry_suggestion_activate (IdeEditorSpellWidget *self,
                                                         GtkMenuItem          *item)
{
  gchar *word;

  g_assert (IDE_IS_EDITOR_SPELL_WIDGET (self));
  g_assert (GTK_IS_MENU_ITEM (item));

  word = g_object_get_data (G_OBJECT (item), "word");

  g_signal_handlers_block_by_func (self->word_entry, ide_editor_spell_widget__word_entry_changed_cb, self);

  gtk_entry_set_text (self->word_entry, word);
  gtk_editable_set_position (GTK_EDITABLE (self->word_entry), -1);
  update_change_ignore_sensibility (self);

  g_signal_handlers_unblock_by_func (self->word_entry, ide_editor_spell_widget__word_entry_changed_cb, self);
}

static void
ide_editor_spell_widget__populate_popup_cb (IdeEditorSpellWidget *self,
                                            GtkWidget            *popup,
                                            GtkEntry             *entry)
{
  GSList *suggestions = NULL;
  const gchar *text;
  GtkWidget *item;
  gint count = 0;

  g_assert (IDE_IS_EDITOR_SPELL_WIDGET (self));
  g_assert (GTK_IS_WIDGET (popup));
  g_assert (GTK_IS_ENTRY (entry));

  text = gtk_entry_get_text (entry);
  if (self->is_word_entry_valid ||
      ide_str_empty0 (text) ||
      NULL == (suggestions = gspell_checker_get_suggestions (self->checker, text, -1)))
    return;

  item = g_object_new (GTK_TYPE_SEPARATOR_MENU_ITEM,
                       "visible", TRUE,
                       NULL);
  gtk_menu_shell_prepend (GTK_MENU_SHELL (popup), item);

  suggestions = g_slist_reverse (suggestions);
  for (GSList *l = (GSList *)suggestions; l != NULL; l = l->next)
    {
      item = g_object_new (GTK_TYPE_MENU_ITEM,
                           "label", l->data,
                           "visible", TRUE,
                           NULL);
      g_object_set_data (G_OBJECT (item), "word", g_strdup (l->data));
      gtk_menu_shell_prepend (GTK_MENU_SHELL (popup), item);
      g_signal_connect_object (item,
                               "activate",
                               G_CALLBACK (ide_editor_spell_widget__word_entry_suggestion_activate),
                               self,
                               G_CONNECT_SWAPPED);

      if (++count >= WORD_ENTRY_MAX_SUGGESTIONS)
        break;
    }

  g_slist_free_full (suggestions, g_free);
}

static void
ide_editor_spell_widget__dict__loaded_cb (IdeEditorSpellWidget *self,
                                          IdeEditorSpellDict   *dict)
{
  g_assert (IDE_IS_EDITOR_SPELL_WIDGET (self));
  g_assert (IDE_IS_EDITOR_SPELL_DICT (dict));

  self->words_array = ide_editor_spell_dict_get_words (self->dict);
  dict_fill_listbox (self, self->words_array);
  g_clear_pointer (&self->words_array, g_ptr_array_unref);
}

static void
ide_editor_spell_widget__word_label_notify_cb (IdeEditorSpellWidget *self,
                                               GParamSpec           *pspec,
                                               GtkLabel             *word_label)
{
  const gchar *text;

  g_assert (IDE_IS_EDITOR_SPELL_WIDGET (self));
  g_assert (GTK_IS_LABEL (word_label));

  if (self->spellchecking_status == TRUE)
    text = gtk_label_get_text (word_label);
  else
    text = "";

  gtk_entry_set_text (GTK_ENTRY (self->dict_word_entry), text);
}

static void
ide_editor_spell_widget_constructed (GObject *object)
{
  IdeEditorSpellWidget *self = (IdeEditorSpellWidget *)object;
  GspellTextBuffer *spell_buffer;

  g_assert (IDE_IS_SOURCE_VIEW (self->view));

  self->buffer = IDE_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (self->view)));
  ide_buffer_set_spell_checking (self->buffer, TRUE);

  self->spellchecking_status = TRUE;

  spell_buffer = gspell_text_buffer_get_from_gtk_text_buffer (GTK_TEXT_BUFFER (self->buffer));
  self->checker = gspell_text_buffer_get_spell_checker (spell_buffer);
  ide_editor_spell_dict_set_checker (self->dict, self->checker);

  self->spellchecker_language = gspell_checker_get_language (self->checker);
  gspell_language_chooser_set_language (GSPELL_LANGUAGE_CHOOSER (self->language_chooser_button),
                                        self->spellchecker_language);

  g_signal_connect_swapped (self->navigator,
                            "notify::words-counted",
                            G_CALLBACK (ide_editor_spell_widget__words_counted_cb),
                            self);

  g_signal_connect_swapped (self->word_entry,
                            "changed",
                            G_CALLBACK (ide_editor_spell_widget__word_entry_changed_cb),
                            self);

  g_signal_connect_swapped (self->word_entry,
                            "populate-popup",
                            G_CALLBACK (ide_editor_spell_widget__populate_popup_cb),
                            self);

  g_signal_connect_swapped (self->ignore_button,
                            "clicked",
                            G_CALLBACK (ide_editor_spell_widget__ignore_button_clicked_cb),
                            self);

  g_signal_connect_swapped (self->ignore_all_button,
                            "clicked",
                            G_CALLBACK (ide_editor_spell_widget__ignore_all_button_clicked_cb),
                            self);

  g_signal_connect_swapped (self->change_button,
                            "clicked",
                            G_CALLBACK (ide_editor_spell_widget__change_button_clicked_cb),
                            self);

  g_signal_connect_swapped (self->change_all_button,
                            "clicked",
                            G_CALLBACK (ide_editor_spell_widget__change_all_button_clicked_cb),
                            self);

  g_signal_connect_swapped (self->suggestions_box,
                            "row-selected",
                            G_CALLBACK (ide_editor_spell_widget__row_selected_cb),
                            self);

  g_signal_connect_swapped (self->suggestions_box,
                            "row-activated",
                            G_CALLBACK (ide_editor_spell_widget__row_activated_cb),
                            self);

  g_signal_connect_swapped (self,
                            "key-press-event",
                            G_CALLBACK (ide_editor_spell_widget__key_press_event_cb),
                            self);

  g_signal_connect_swapped (self->highlight_switch,
                            "state-set",
                            G_CALLBACK (ide_editor_spell_widget__highlight_switch_toggled_cb),
                            self);

  g_signal_connect_object (self->language_chooser_button,
                           "notify::language",
                           G_CALLBACK (ide_editor_spell_widget__language_notify_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_swapped (self->dict_add_button,
                            "clicked",
                            G_CALLBACK (ide_editor_spell_widget__add_button_clicked_cb),
                            self);

  g_signal_connect_swapped (self->dict_word_entry,
                            "changed",
                            G_CALLBACK (ide_editor_spell_widget__dict_word_entry_changed_cb),
                            self);

  self->placeholder = gtk_label_new (NULL);
  gtk_widget_set_visible (self->placeholder, TRUE);
  gtk_list_box_set_placeholder (self->suggestions_box, self->placeholder);

  /* Due to the change of focus between the view and the spellchecker widget,
   * we need to start checking only when the widget is mapped,
   * so the view can keep the selection on the first word.
   */
  g_signal_connect_object (self,
                           "map",
                           G_CALLBACK (ide_editor_spell__widget_mapped_cb),
                           NULL,
                           G_CONNECT_AFTER);

  g_signal_connect_swapped (self->dict,
                            "loaded",
                            G_CALLBACK (ide_editor_spell_widget__dict__loaded_cb),
                            self);

  g_signal_connect_object (self->word_label,
                           "notify::label",
                           G_CALLBACK (ide_editor_spell_widget__word_label_notify_cb),
                           self,
                           G_CONNECT_SWAPPED);
}

static void
ide_editor_spell_widget_finalize (GObject *object)
{
  IdeEditorSpellWidget *self = (IdeEditorSpellWidget *)object;
  GspellTextView *spell_text_view;
  const GspellLanguage *spell_language;
  GtkTextBuffer *buffer;

  if (self->check_word_timeout_id > 0)
    g_source_remove (self->check_word_timeout_id);

  /* Set back the view spellchecking previous state */
  if (self->view != NULL)
    {
      spell_text_view = gspell_text_view_get_from_gtk_text_view (GTK_TEXT_VIEW (self->view));
      if (self->view_spellchecker_set)
        {
          gspell_text_view_set_inline_spell_checking (spell_text_view, TRUE);
          spell_language = gspell_checker_get_language (self->checker);
          if (gspell_language_compare (self->spellchecker_language, spell_language) != 0)
            gspell_checker_set_language (self->checker, self->spellchecker_language);
        }
      else
        {
          gspell_text_view_set_inline_spell_checking (spell_text_view, FALSE);
          gspell_text_view_set_enable_language_menu (spell_text_view, FALSE);

          buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (self->view));
          ide_buffer_set_spell_checking (IDE_BUFFER (buffer), FALSE);
        }
    }

  g_clear_object (&self->navigator);
  ide_clear_weak_pointer (&self->view);

  G_OBJECT_CLASS (ide_editor_spell_widget_parent_class)->finalize (object);
}

static void
ide_editor_spell_widget_get_property (GObject    *object,
                                      guint       prop_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  IdeEditorSpellWidget *self = IDE_EDITOR_SPELL_WIDGET (object);

  switch (prop_id)
    {
    case PROP_VIEW:
      g_value_set_object (value, ide_editor_spell_widget_get_view (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ide_editor_spell_widget_set_property (GObject      *object,
                                      guint         prop_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  IdeEditorSpellWidget *self = IDE_EDITOR_SPELL_WIDGET (object);

  switch (prop_id)
    {
    case PROP_VIEW:
      ide_editor_spell_widget_set_view (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
ide_editor_spell_widget_class_init (IdeEditorSpellWidgetClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = ide_editor_spell_widget_constructed;
  object_class->finalize = ide_editor_spell_widget_finalize;
  object_class->get_property = ide_editor_spell_widget_get_property;
  object_class->set_property = ide_editor_spell_widget_set_property;

  properties [PROP_VIEW] =
    g_param_spec_object ("view",
                         "View",
                         "The source view.",
                         IDE_TYPE_SOURCE_VIEW,
                         (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/builder/ui/ide-editor-spell-widget.ui");

  gtk_widget_class_bind_template_child (widget_class, IdeEditorSpellWidget, word_label);
  gtk_widget_class_bind_template_child (widget_class, IdeEditorSpellWidget, count_label);
  gtk_widget_class_bind_template_child (widget_class, IdeEditorSpellWidget, word_entry);
  gtk_widget_class_bind_template_child (widget_class, IdeEditorSpellWidget, ignore_button);
  gtk_widget_class_bind_template_child (widget_class, IdeEditorSpellWidget, ignore_all_button);
  gtk_widget_class_bind_template_child (widget_class, IdeEditorSpellWidget, change_button);
  gtk_widget_class_bind_template_child (widget_class, IdeEditorSpellWidget, change_all_button);
  gtk_widget_class_bind_template_child (widget_class, IdeEditorSpellWidget, highlight_switch);
  gtk_widget_class_bind_template_child (widget_class, IdeEditorSpellWidget, language_chooser_button);
  gtk_widget_class_bind_template_child (widget_class, IdeEditorSpellWidget, suggestions_box);
  gtk_widget_class_bind_template_child (widget_class, IdeEditorSpellWidget, dict_word_entry);
  gtk_widget_class_bind_template_child (widget_class, IdeEditorSpellWidget, dict_add_button);
  gtk_widget_class_bind_template_child (widget_class, IdeEditorSpellWidget, dict_words_list);
  gtk_widget_class_bind_template_child (widget_class, IdeEditorSpellWidget, count_box);
}

static void
ide_editor_spell_widget_init (IdeEditorSpellWidget *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
  self->dict = ide_editor_spell_dict_new (NULL);

  self->view_spellchecker_set = FALSE;
  /* FIXME: do not work, Gtk+ bug */
  gtk_entry_set_icon_tooltip_text (self->word_entry,
                                   GTK_ENTRY_ICON_SECONDARY,
                                   _("The word is not in the dictionary"));

  g_signal_connect_swapped (self->dict_words_list,
                            "key-press-event",
                            G_CALLBACK (dict_row_key_pressed_event_cb),
                            self);
}
