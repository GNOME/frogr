/*
 * frogr-details-dialog.c -- Picture details dialog
 *
 * Copyright (C) 2009-2011 Mario Sanchez Prada
 * Authors: Mario Sanchez Prada <msanchez@igalia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 3 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>
 *
 */

#include "frogr-details-dialog.h"

#include "frogr-config.h"
#include "frogr-controller.h"
#include "frogr-global-defs.h"
#include "frogr-picture.h"
#include "frogr-util.h"

#include <config.h>
#include <glib/gi18n.h>
#include <flicksoup/flicksoup.h>

/* Path relative to the application data dir */
#define MPICTURES_IMAGE "/images/mpictures.png"

#define DIALOG_MIN_WIDTH 740
#define DIALOG_MIN_HEIGHT 400

#define PICTURE_WIDTH 180
#define PICTURE_HEIGHT 180

#define FROGR_DETAILS_DIALOG_GET_PRIVATE(object)                \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),                       \
                                FROGR_TYPE_DETAILS_DIALOG,      \
                                FrogrDetailsDialogPrivate))

G_DEFINE_TYPE (FrogrDetailsDialog, frogr_details_dialog, GTK_TYPE_DIALOG)

typedef struct _FrogrDetailsDialogPrivate {
  GtkWidget *title_entry;
  GtkWidget *desc_tview;
  GtkWidget *tags_entry;
  GtkWidget *public_rb;
  GtkWidget *private_rb;
  GtkWidget *friend_cb;
  GtkWidget *family_cb;
  GtkWidget *show_in_search_cb;
  GtkWidget *license_cb;
  GtkWidget *photo_content_rb;
  GtkWidget *sshot_content_rb;
  GtkWidget *other_content_rb;
  GtkWidget *safe_rb;
  GtkWidget *moderate_rb;
  GtkWidget *restricted_rb;
  GtkTextBuffer *text_buffer;

  GtkWidget *picture_img;
  GtkWidget *picture_button;
  GtkWidget *picture_container;
  GtkWidget *mpictures_label;
  GdkPixbuf *mpictures_pixbuf;

  GtkTreeModel *treemodel;
  GSList *pictures;
  gulong picture_button_handler_id;
  gchar *reference_description;
} FrogrDetailsDialogPrivate;

/* Properties */
enum  {
  PROP_0,
  PROP_PICTURES
};

/* Tree view columns */
enum {
  TEXT_COL,
  N_COLS
};

static const gchar *license_descriptions[] = {
  N_("Default (as specified in flickr)"),
  N_("All rights reserved"),
  N_("CC Attribution-NonCommercial-ShareAlike"),
  N_("CC Attribution-NonCommercial"),
  N_("CC Attribution-NonCommercial-NoDerivs"),
  N_("CC Attribution"),
  N_("CC Attribution-ShareAlike"),
  N_("CC Attribution-NoDerivs"),
  NULL
};


/* Prototypes */

static void _create_widgets (FrogrDetailsDialog *self);

static void _populate_treemodel_with_tags (FrogrDetailsDialog *self, GSList *tags);

static gboolean _tag_list_completion_func (GtkEntryCompletion *completion, const gchar *key,
                                           GtkTreeIter *iter, gpointer data);

static gboolean _completion_match_selected_cb (GtkEntryCompletion *widget, GtkTreeModel *model,
                                               GtkTreeIter *iter, gpointer data);

static void _update_ui (FrogrDetailsDialog *self);

static void _load_picture_from_disk (FrogrDetailsDialog *self);

static void _load_picture_from_disk_cb (GObject *object,
                                        GAsyncResult *res,
                                        gpointer data);

static void _place_picture_in_dialog_and_show (FrogrDetailsDialog *self);

static void _fill_dialog_with_data (FrogrDetailsDialog *self);

static gboolean _validate_dialog_data (FrogrDetailsDialog *self);

static gboolean _save_data (FrogrDetailsDialog *self);

static void _on_radio_button_clicked (GtkButton *tbutton, gpointer data);

static void _on_picture_button_clicked (GtkButton *button, gpointer data);

static void _on_toggle_button_toggled (GtkToggleButton *tbutton, gpointer data);

static void _dialog_response_cb (GtkDialog *dialog, gint response, gpointer data);


/* Private API */

static void
_create_widgets (FrogrDetailsDialog *self)
{
  FrogrDetailsDialogPrivate *priv = NULL;
  FrogrConfig *config = NULL;
  GtkWidget *main_vbox = NULL;
  GtkWidget *vbox = NULL;
  GtkWidget *hbox = NULL;
  GtkWidget *section_vbox = NULL;
  GtkWidget *internal_hbox = NULL;
  GtkWidget *visibility_vbox = NULL;
  GtkWidget *private_vbox = NULL;
  GtkWidget *content_type_hbox = NULL;
  GtkWidget *safety_level_hbox = NULL;
  GtkWidget *align = NULL;
  GtkWidget *label = NULL;
  GtkWidget *widget = NULL;
  GtkWidget *table = NULL;
  GtkWidget *scroller = NULL;
  GtkTreeModel *model = NULL;
  gchar *markup = NULL;
  gint i;

  priv = FROGR_DETAILS_DIALOG_GET_PRIVATE (self);

  main_vbox = gtk_dialog_get_content_area (GTK_DIALOG (self));

#ifdef GTK_API_VERSION_3
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
#else
  hbox = gtk_hbox_new (FALSE, 0);
  vbox = gtk_vbox_new (FALSE, 0);
#endif

  /* Left side (image, radio buttons, checkboxes...) */

  widget = gtk_button_new ();
  gtk_widget_set_tooltip_text (widget, _("Open with image viewer"));
  gtk_button_set_relief (GTK_BUTTON (widget), GTK_RELIEF_NONE);
  priv->picture_button = widget;

  align = gtk_alignment_new (0.5, 0, 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 0);
  priv->picture_container = align;

  widget = gtk_image_new ();
  gtk_widget_set_size_request (widget, PICTURE_WIDTH, -1);
  priv->picture_img = widget;

  widget = gtk_label_new (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
  priv->mpictures_label = widget;
  priv->mpictures_pixbuf = NULL;

  /* Visibility */

#ifdef GTK_API_VERSION_3
  section_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  visibility_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
#else
  section_vbox = gtk_vbox_new (FALSE, 6);
  visibility_vbox = gtk_vbox_new (FALSE, 6);
#endif

  markup = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>",
                                    _("Visibility"));
  widget = gtk_label_new (markup);
  gtk_label_set_use_markup (GTK_LABEL (widget), TRUE);
  g_free (markup);

  align = gtk_alignment_new (0, 0, 0, 0);
  gtk_container_add (GTK_CONTAINER (align), widget);
  gtk_box_pack_start (GTK_BOX (section_vbox), align, FALSE, FALSE, 0);

#ifdef GTK_API_VERSION_3
  internal_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
#else
  internal_hbox = gtk_hbox_new (FALSE, 6);
#endif

  widget = gtk_radio_button_new_with_mnemonic (NULL, _("_Private"));
  gtk_box_pack_start (GTK_BOX (internal_hbox), widget, FALSE, FALSE, 0);
  priv->private_rb = widget;

  widget = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (priv->private_rb), _("P_ublic"));
  gtk_box_pack_start (GTK_BOX (internal_hbox), widget, FALSE, FALSE, 0);
  priv->public_rb = widget;

  gtk_box_pack_start (GTK_BOX (visibility_vbox), internal_hbox, FALSE, FALSE, 0);

#ifdef GTK_API_VERSION_3
  private_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
#else
  private_vbox = gtk_vbox_new (FALSE, 6);
#endif

  widget = gtk_check_button_new_with_mnemonic (_("Visible to _Family"));
  gtk_box_pack_start (GTK_BOX (private_vbox), widget, FALSE, FALSE, 0);
  priv->family_cb = widget;

  widget = gtk_check_button_new_with_mnemonic (_("Visible to F_riends"));
  gtk_box_pack_start (GTK_BOX (private_vbox), widget, FALSE, FALSE, 0);
  priv->friend_cb = widget;

#ifdef GTK_API_VERSION_3
  internal_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
#else
  internal_hbox = gtk_hbox_new (FALSE, 0);
#endif

  gtk_box_pack_start (GTK_BOX (internal_hbox), private_vbox, FALSE, FALSE, 12);
  gtk_box_pack_start (GTK_BOX (visibility_vbox), internal_hbox, FALSE, FALSE, 0);

#ifdef GTK_API_VERSION_3
  internal_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
#else
  internal_hbox = gtk_hbox_new (FALSE, 0);
#endif

  gtk_box_pack_start (GTK_BOX (internal_hbox), visibility_vbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (section_vbox), internal_hbox, FALSE, FALSE, 0);

#ifdef GTK_API_VERSION_3
  internal_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
#else
  internal_hbox = gtk_hbox_new (FALSE, 0);
#endif

  widget = gtk_check_button_new_with_mnemonic (_("_Show up in Global Search Results"));
  gtk_box_pack_start (GTK_BOX (internal_hbox), widget, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (section_vbox), internal_hbox, FALSE, FALSE, 0);
  priv->show_in_search_cb = widget;

  gtk_box_pack_start (GTK_BOX (vbox), section_vbox, FALSE, FALSE, 6);

  /* Content type */

#ifdef GTK_API_VERSION_3
  section_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  content_type_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
#else
  section_vbox = gtk_vbox_new (FALSE, 6);
  content_type_hbox = gtk_hbox_new (FALSE, 6);
#endif

  markup = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>",
                                    _("Content type"));
  widget = gtk_label_new (markup);
  gtk_label_set_use_markup (GTK_LABEL (widget), TRUE);
  g_free (markup);

  align = gtk_alignment_new (0, 0, 0, 0);
  gtk_container_add (GTK_CONTAINER (align), widget);
  gtk_box_pack_start (GTK_BOX (section_vbox), align, FALSE, FALSE, 0);

  widget = gtk_radio_button_new_with_mnemonic (NULL, _("P_hoto"));
  gtk_box_pack_start (GTK_BOX (content_type_hbox), widget, FALSE, FALSE, 0);
  priv->photo_content_rb = widget;

  widget = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (priv->photo_content_rb), _("Scree_nshot"));
  gtk_box_pack_start (GTK_BOX (content_type_hbox), widget, FALSE, FALSE, 0);
  priv->sshot_content_rb = widget;

  widget = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (priv->photo_content_rb), _("Oth_er"));
  gtk_box_pack_start (GTK_BOX (content_type_hbox), widget, FALSE, FALSE, 0);
  priv->other_content_rb = widget;

  gtk_box_pack_start (GTK_BOX (section_vbox), content_type_hbox, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), section_vbox, FALSE, FALSE, 6);

  /* Safety level */

#ifdef GTK_API_VERSION_3
  section_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  safety_level_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
#else
  section_vbox = gtk_vbox_new (FALSE, 6);
  safety_level_hbox = gtk_hbox_new (FALSE, 6);
#endif

  markup = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>",
                                    _("Safety level"));
  widget = gtk_label_new (markup);
  gtk_label_set_use_markup (GTK_LABEL (widget), TRUE);
  g_free (markup);

  align = gtk_alignment_new (0, 0, 0, 0);
  gtk_container_add (GTK_CONTAINER (align), widget);
  gtk_box_pack_start (GTK_BOX (section_vbox), align, FALSE, FALSE, 0);

  widget = gtk_radio_button_new_with_mnemonic (NULL, _("S_afe"));
  gtk_box_pack_start (GTK_BOX (safety_level_hbox), widget, FALSE, FALSE, 0);
  priv->safe_rb = widget;

  widget = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (priv->safe_rb), _("_Moderate"));
  gtk_box_pack_start (GTK_BOX (safety_level_hbox), widget, FALSE, FALSE, 0);
  priv->moderate_rb = widget;

  widget = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (priv->safe_rb), _("Restr_icted"));
  gtk_box_pack_start (GTK_BOX (safety_level_hbox), widget, FALSE, FALSE, 0);
  priv->restricted_rb = widget;

  gtk_box_pack_start (GTK_BOX (section_vbox), safety_level_hbox, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), section_vbox, FALSE, FALSE, 0);

  /* License type */

#ifdef GTK_API_VERSION_3
  section_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
#else
  section_vbox = gtk_vbox_new (FALSE, 6);
#endif

  markup = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>",
                                    _("License type"));
  widget = gtk_label_new (markup);
  gtk_label_set_use_markup (GTK_LABEL (widget), TRUE);
  g_free (markup);

  align = gtk_alignment_new (0, 0, 0, 0);
  gtk_container_add (GTK_CONTAINER (align), widget);
  gtk_box_pack_start (GTK_BOX (section_vbox), align, FALSE, FALSE, 0);

#if GTK_CHECK_VERSION (2,24,0)
  widget = gtk_combo_box_text_new ();
  for (i = 0; license_descriptions[i]; i++)
    gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (widget), i, gettext (license_descriptions[i]));
#else
  for (i = 0; license_descriptions[i]; i++)
    gtk_combo_box_insert_text (GTK_COMBO_BOX (widget), i, gettext (license_descriptions[i]));
#endif
  priv->license_cb = widget;
  gtk_box_pack_start (GTK_BOX (section_vbox), widget, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), section_vbox, FALSE, FALSE, 6);

  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 6);

  /* Right side (text fields) */

  table = gtk_table_new (3, 2, FALSE);

  label = gtk_label_new_with_mnemonic (_("_Title:"));
  align = gtk_alignment_new (1, 0, 1, 0);
  gtk_container_add (GTK_CONTAINER (align), label);
  gtk_table_attach (GTK_TABLE (table), align, 0, 1, 0, 1,
                    0, 0, 6, 6);

  widget = gtk_entry_new ();
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), widget);
  gtk_table_attach (GTK_TABLE (table), widget, 1, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, 0, 6, 6);
  priv->title_entry = widget;

  label = gtk_label_new_with_mnemonic (_("_Description:"));
  align = gtk_alignment_new (1, 0, 1, 0);
  gtk_container_add (GTK_CONTAINER (align), label);
  gtk_table_attach (GTK_TABLE (table), align, 0, 1, 1, 2,
                    0, GTK_EXPAND | GTK_FILL, 6, 6);

  widget = gtk_text_view_new ();
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), widget);
  gtk_text_view_set_accepts_tab (GTK_TEXT_VIEW (widget), FALSE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (widget), GTK_WRAP_WORD);

  scroller = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroller),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroller),
                                       GTK_SHADOW_ETCHED_IN);
  gtk_container_add (GTK_CONTAINER (scroller), widget);

  align = gtk_alignment_new (1, 0, 1, 1);
  gtk_container_add (GTK_CONTAINER (align), scroller);
  gtk_table_attach (GTK_TABLE (table), align, 1, 2, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 6, 6);
  priv->desc_tview = widget;
  priv->text_buffer =
    gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->desc_tview));

  label = gtk_label_new_with_mnemonic (_("Ta_gs:"));
  align = gtk_alignment_new (1, 0, 1, 0);
  gtk_container_add (GTK_CONTAINER (align), label);
  gtk_table_attach (GTK_TABLE (table), align, 0, 1, 2, 3,
                    0, 0, 6, 6);

  widget = gtk_entry_new ();
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), widget);
  gtk_table_attach (GTK_TABLE (table), widget, 1, 2, 2, 3,
                    GTK_EXPAND | GTK_FILL, 0, 6, 6);
  priv->tags_entry = widget;

  gtk_box_pack_start (GTK_BOX (hbox), table, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, TRUE, TRUE, 6);

  model = GTK_TREE_MODEL (gtk_list_store_new (1, G_TYPE_STRING));
  priv->treemodel = model;

  config = frogr_config_get_instance ();
  if (config && frogr_config_get_tags_autocompletion (config))
    {
      GtkEntryCompletion *completion = NULL;

      /* Prepare auto completion for tags */
      completion = gtk_entry_completion_new ();
      gtk_entry_completion_set_text_column (GTK_ENTRY_COMPLETION (completion), TEXT_COL);
      gtk_entry_completion_set_inline_completion (GTK_ENTRY_COMPLETION (completion), TRUE);
      gtk_entry_completion_set_match_func (GTK_ENTRY_COMPLETION (completion),
                                           _tag_list_completion_func,
                                           self, NULL);
      gtk_entry_set_completion (GTK_ENTRY (priv->tags_entry), completion);

      gtk_entry_completion_set_model (GTK_ENTRY_COMPLETION (completion), model);

      g_signal_connect (G_OBJECT (completion), "match-selected",
                        G_CALLBACK (_completion_match_selected_cb), self);
    }

  /* Connect signals */
  g_signal_connect (G_OBJECT (priv->public_rb), "clicked",
                    G_CALLBACK (_on_radio_button_clicked), self);

  g_signal_connect (G_OBJECT (priv->private_rb), "clicked",
                    G_CALLBACK (_on_radio_button_clicked), self);

  g_signal_connect (G_OBJECT (priv->family_cb), "toggled",
                    G_CALLBACK (_on_toggle_button_toggled), self);

  g_signal_connect (G_OBJECT (priv->friend_cb), "toggled",
                    G_CALLBACK (_on_toggle_button_toggled), self);

  g_signal_connect (G_OBJECT (priv->show_in_search_cb), "toggled",
                    G_CALLBACK (_on_toggle_button_toggled), self);

  g_signal_connect (G_OBJECT (priv->photo_content_rb), "clicked",
                    G_CALLBACK (_on_radio_button_clicked), self);

  g_signal_connect (G_OBJECT (priv->sshot_content_rb), "clicked",
                    G_CALLBACK (_on_radio_button_clicked), self);

  g_signal_connect (G_OBJECT (priv->other_content_rb), "clicked",
                    G_CALLBACK (_on_radio_button_clicked), self);

  g_signal_connect (G_OBJECT (priv->safe_rb), "clicked",
                    G_CALLBACK (_on_radio_button_clicked), self);

  g_signal_connect (G_OBJECT (priv->moderate_rb), "clicked",
                    G_CALLBACK (_on_radio_button_clicked), self);

  g_signal_connect (G_OBJECT (priv->restricted_rb), "clicked",
                    G_CALLBACK (_on_radio_button_clicked), self);

  /* Show widgets */
  gtk_widget_show_all (main_vbox);
}

static void
_populate_treemodel_with_tags (FrogrDetailsDialog *self, GSList *tags)
{
  FrogrDetailsDialogPrivate *priv = NULL;
  GSList *item = NULL;
  gchar *tag = NULL;
  GtkTreeIter iter;

  priv = FROGR_DETAILS_DIALOG_GET_PRIVATE (self);

  for (item = tags; item; item = g_slist_next (item))
    {
      tag = (gchar *) item->data;
      gtk_list_store_append (GTK_LIST_STORE (priv->treemodel), &iter);
      gtk_list_store_set (GTK_LIST_STORE (priv->treemodel), &iter,
                          0, tag, -1);
    }
}

static gboolean
_tag_list_completion_func (GtkEntryCompletion *completion, const gchar *key,
                           GtkTreeIter *iter, gpointer data)
{
  FrogrDetailsDialog *self = NULL;
  FrogrDetailsDialogPrivate *priv = NULL;
  gchar *stripped_entry_text = NULL;
  gchar *basetext = NULL;
  gchar *tag = NULL;
  gchar *lc_basetext = NULL;
  gchar *lc_tag = NULL;
  gint cursor_pos = 0;
  gboolean matches = FALSE;

  self = FROGR_DETAILS_DIALOG (data);
  priv = FROGR_DETAILS_DIALOG_GET_PRIVATE (self);
  gtk_tree_model_get (priv->treemodel, iter, TEXT_COL, &tag, -1);

  /* Do nothing if not a valid tag */
  if (!tag)
    return FALSE;

  /* Do nothing if the cursor is not in the last position */
  cursor_pos = gtk_editable_get_position (GTK_EDITABLE (priv->tags_entry));
  if (cursor_pos < gtk_entry_get_text_length (GTK_ENTRY (priv->tags_entry)))
    return FALSE;

  /* Look for the last token in 'key' */
  stripped_entry_text = gtk_editable_get_chars (GTK_EDITABLE (priv->tags_entry), 0, cursor_pos);
  stripped_entry_text = g_strstrip (stripped_entry_text);
  basetext = g_strrstr (stripped_entry_text, " ");
  if (basetext)
    basetext++;
  else
    basetext = stripped_entry_text;

  /* Downcase everything and compare */
  lc_basetext = g_utf8_strdown (basetext, -1);
  lc_tag = g_utf8_strdown (tag, -1);
  if (g_str_has_prefix (lc_tag, lc_basetext))
    matches = TRUE;

  g_free (stripped_entry_text);
  g_free (tag);
  g_free (lc_basetext);
  g_free (lc_tag);

  return matches;
}

static gboolean
_completion_match_selected_cb (GtkEntryCompletion *widget, GtkTreeModel *model,
                               GtkTreeIter *iter, gpointer data)
{
  FrogrDetailsDialog *self = NULL;
  FrogrDetailsDialogPrivate *priv = NULL;
  gchar *tag = NULL;
  const gchar *entry_text = NULL;
  const gchar *matching_text = NULL;
  gchar *base_text = NULL;
  gchar *new_text = NULL;
  glong entry_text_len = 0;
  glong matching_text_len = 0;

  self = FROGR_DETAILS_DIALOG (data);
  priv = FROGR_DETAILS_DIALOG_GET_PRIVATE (self);
  gtk_tree_model_get (model, iter, TEXT_COL, &tag, -1);

  entry_text = gtk_entry_get_text (GTK_ENTRY (priv->tags_entry));
  matching_text = g_strrstr (entry_text, " ");
  if (matching_text)
    matching_text++;
  else
    matching_text = entry_text;

  entry_text_len = gtk_entry_get_text_length (GTK_ENTRY (priv->tags_entry));
  matching_text_len = g_utf8_strlen (matching_text, -1);

  base_text = gtk_editable_get_chars (GTK_EDITABLE (priv->tags_entry), 0, entry_text_len - matching_text_len);
  new_text = g_strdup_printf ("%s%s ", base_text, tag);

  gtk_entry_set_text (GTK_ENTRY (priv->tags_entry), new_text);
  gtk_editable_set_position (GTK_EDITABLE (priv->tags_entry), -1);

  g_free (tag);
  g_free (base_text);
  g_free (new_text);

  return TRUE;
}

static void
_update_ui (FrogrDetailsDialog *self)
{
  FrogrDetailsDialogPrivate *priv =
    FROGR_DETAILS_DIALOG_GET_PRIVATE (self);
  gboolean active;

  /* Adjust sensitiveness for check buttons */
  active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->public_rb));
  gtk_widget_set_sensitive (priv->friend_cb, !active);
  gtk_widget_set_sensitive (priv->family_cb, !active);
}

static void
_load_picture_from_disk (FrogrDetailsDialog *self)
{
  FrogrDetailsDialogPrivate *priv = FROGR_DETAILS_DIALOG_GET_PRIVATE (self);
  guint n_pictures;

  n_pictures = g_slist_length (priv->pictures);
  if (n_pictures > 1)
    {
      gchar *mpictures_str = NULL;

      /* Get the 'multiple pictures pixbuf' if not got yet (lazy approach) */
      if (!priv->mpictures_pixbuf)
        {
          gchar *mpictures_full_path = NULL;

          /* Set the image for editing multiple pictures */
          mpictures_full_path = g_strdup_printf ("%s/" MPICTURES_IMAGE,
                                                 frogr_util_get_app_data_dir ());
          priv->mpictures_pixbuf = gdk_pixbuf_new_from_file (mpictures_full_path, NULL);
          g_free (mpictures_full_path);
        }

      /* Just set the pixbuf in the image */
      gtk_image_set_from_pixbuf (GTK_IMAGE (priv->picture_img), priv->mpictures_pixbuf);

      /* Visually indicate how many pictures are being edited */
      mpictures_str = g_strdup_printf (ngettext ("(%d Picture)", "(%d Pictures)", n_pictures), n_pictures);
      gtk_label_set_text (GTK_LABEL (priv->mpictures_label), mpictures_str);
      g_free (mpictures_str);

      /* No need to spawn any async operation, show the dialog now */
      _place_picture_in_dialog_and_show (self);
    }
  else
    {
      FrogrPicture *picture = NULL;
      gchar *file_uri = NULL;
      GFile *gfile = NULL;

      picture = FROGR_PICTURE (priv->pictures->data);
      file_uri = (gchar *)frogr_picture_get_fileuri (picture);
      gfile = g_file_new_for_uri (file_uri);

      /* Asynchronously load the picture */
      g_file_load_contents_async (gfile, NULL, _load_picture_from_disk_cb, self);
    }
}

static void
_load_picture_from_disk_cb (GObject *object,
                            GAsyncResult *res,
                            gpointer data)
{
  FrogrDetailsDialog *self = FROGR_DETAILS_DIALOG (data);
  FrogrDetailsDialogPrivate *priv = FROGR_DETAILS_DIALOG_GET_PRIVATE (self);
  GFile *file = NULL;
  GError *error = NULL;
  gchar *contents = NULL;
  gsize length = 0;

  file = G_FILE (object);
  if (g_file_load_contents_finish (file, res, &contents, &length, NULL, &error))
    {
      GdkPixbufLoader *pixbuf_loader = gdk_pixbuf_loader_new ();

      if (gdk_pixbuf_loader_write (pixbuf_loader,
                                   (const guchar *)contents,
                                   length,
                                   &error))
        {
          GdkPixbuf *pixbuf = NULL;
          GdkPixbuf *s_pixbuf = NULL;

          gdk_pixbuf_loader_close (pixbuf_loader, NULL);
          pixbuf = gdk_pixbuf_loader_get_pixbuf (pixbuf_loader);

          /* Get (scaled, and maybe rotated) pixbuf */
          s_pixbuf = frogr_util_get_corrected_pixbuf (pixbuf, PICTURE_WIDTH, PICTURE_HEIGHT);

          gtk_image_set_from_pixbuf (GTK_IMAGE (priv->picture_img), s_pixbuf);
          g_object_unref (s_pixbuf);

          /* Everything should be fine by now, show it */
          _place_picture_in_dialog_and_show (self);
        }
    }

  /* Show error to the user and finalize dialog if needed */
  if (error)
    {
      GtkWindow *parent_window = NULL;
      gchar *error_msg = NULL;

      parent_window = gtk_window_get_transient_for (GTK_WINDOW (self));
      gtk_widget_destroy (GTK_WIDGET (self));

      if (error)
        {
          error_msg = g_strdup (error->message);
          g_error_free (error);
        }
      else
        error_msg = g_strdup (_("An error happened trying to load the picture"));

      frogr_util_show_error_dialog (parent_window, error_msg);
      g_free (error_msg);
    }
}

static void
_place_picture_in_dialog_and_show (FrogrDetailsDialog *self)
{
  FrogrDetailsDialogPrivate *priv = FROGR_DETAILS_DIALOG_GET_PRIVATE (self);

  gtk_button_set_image (GTK_BUTTON (priv->picture_button),
                        priv->picture_img);

  gtk_container_add (GTK_CONTAINER (priv->picture_container),
                     priv->picture_button);

  priv->picture_button_handler_id =
    g_signal_connect (G_OBJECT (priv->picture_button), "clicked",
                      G_CALLBACK (_on_picture_button_clicked),
                      self);

  gtk_widget_show_all (GTK_WIDGET (self));
}

static void
_fill_dialog_with_data (FrogrDetailsDialog *self)
{
  FrogrDetailsDialogPrivate *priv =
    FROGR_DETAILS_DIALOG_GET_PRIVATE (self);

  FrogrPicture *picture;
  GSList *item;
  GtkWidget *picture_widget;
  gchar *title_val = NULL;
  gchar *desc_val = NULL;
  gchar *tags_val = NULL;
  gboolean is_public_val = FALSE;
  gboolean is_friend_val = FALSE;
  gboolean is_family_val = FALSE;
  gboolean show_in_search_val = FALSE;
  gboolean license_inconsistent = FALSE;
  FspLicense license_val = FSP_LICENSE_NONE;
  FspSafetyLevel safety_level_val = FSP_SAFETY_LEVEL_NONE;
  FspContentType content_type_val = FSP_CONTENT_TYPE_NONE;

  /* Take first element values */
  item = priv->pictures;
  picture = FROGR_PICTURE (item->data);

  title_val = (gchar *)frogr_picture_get_title (picture);
  desc_val = (gchar *)frogr_picture_get_description (picture);
  tags_val = (gchar *)frogr_picture_get_tags (picture);
  is_public_val = frogr_picture_is_public (picture);
  is_friend_val = frogr_picture_is_friend (picture);
  is_family_val = frogr_picture_is_family (picture);
  show_in_search_val = frogr_picture_show_in_search (picture);
  license_val = frogr_picture_get_license (picture);
  safety_level_val = frogr_picture_get_safety_level (picture);
  content_type_val = frogr_picture_get_content_type (picture);

  /* Iterate over the rest of elements */
  for (item = g_slist_next (item); item; item = g_slist_next (item))
    {
      gchar *title = NULL;
      gchar *desc = NULL;
      gchar *tags = NULL;
      gboolean is_public = FALSE;
      gboolean is_friend = FALSE;
      gboolean is_family = FALSE;
      gboolean show_in_search = FALSE;
      FspLicense license = FSP_LICENSE_NONE;
      FspSafetyLevel safety_level = FSP_SAFETY_LEVEL_NONE;
      FspContentType content_type = FSP_CONTENT_TYPE_NONE;

      /* Retrieve needed data */
      picture = FROGR_PICTURE (item->data);

      /* Only retrieve title and desc when needed */
      if (title_val != NULL)
        title = (gchar *)frogr_picture_get_title (picture);
      if (desc_val != NULL)
        desc = (gchar *)frogr_picture_get_description (picture);
      if (tags_val != NULL)
        tags = (gchar *)frogr_picture_get_tags (picture);

      /* boolean properties */
      is_public = frogr_picture_is_public (picture);
      is_friend = frogr_picture_is_friend (picture);
      is_family = frogr_picture_is_family (picture);
      show_in_search = frogr_picture_show_in_search (picture);

      /* License */
      license = frogr_picture_get_license (picture);

      /* Content type and safety level */
      safety_level = frogr_picture_get_safety_level (picture);
      content_type = frogr_picture_get_content_type (picture);

      /* Update actual values for the dialog */
      if (title_val && title)
        title_val = g_str_equal (title_val, title) ? title_val : NULL;
      else
        title_val = NULL;

      if (desc_val && desc)
        desc_val = g_str_equal (desc_val, desc) ? desc_val : NULL;
      else
        desc_val = NULL;

      if (tags_val && tags)
        tags_val = g_str_equal (tags_val, tags) ? tags_val : NULL;
      else
        tags_val = NULL;

      /* Check radio and check buttons consistence */
      if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (priv->public_rb)))
        {
          gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (priv->public_rb),
                                              is_public_val != is_public);
        }
      if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (priv->private_rb)))
        {
          gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (priv->private_rb),
                                              is_public_val != is_public);
        }
      if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (priv->family_cb)))
        {
          gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (priv->family_cb),
                                              is_family_val != is_family);
        }
      if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (priv->friend_cb)))
        {
          gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (priv->friend_cb),
                                              is_friend_val != is_friend);
        }
      if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (priv->show_in_search_cb)))
        {
          gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (priv->show_in_search_cb),
                                              show_in_search_val != show_in_search);
        }

      if (!license_inconsistent && license_val != license)
        {
          license_inconsistent = TRUE;
        }

      if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (priv->photo_content_rb)))
        {
          gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (priv->photo_content_rb),
                                              content_type_val != content_type);
        }
      if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (priv->photo_content_rb)))
        {
          gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (priv->photo_content_rb),
                                              content_type_val != content_type);
        }
      if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (priv->sshot_content_rb)))
        {
          gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (priv->sshot_content_rb),
                                              content_type_val != content_type);
        }
      if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (priv->other_content_rb)))
        {
          gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (priv->other_content_rb),
                                              content_type_val != content_type);
        }
      if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (priv->safe_rb)))
        {
          gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (priv->safe_rb),
                                              safety_level_val != safety_level);
        }
      if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (priv->moderate_rb)))
        {
          gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (priv->moderate_rb),
                                              safety_level_val != safety_level);
        }
      if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (priv->restricted_rb)))
        {
          gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (priv->restricted_rb),
                                              safety_level_val != safety_level);
        }

      /* Update merged value */
      is_public_val = is_public;
      is_friend_val = is_friend;
      is_family_val = is_family;
      show_in_search_val = show_in_search;
      license_val = license;
      content_type_val = content_type;
      safety_level_val = safety_level;
    }

  /* Fill in with data */

  if (title_val != NULL)
    gtk_entry_set_text (GTK_ENTRY (priv->title_entry), title_val);

  if (desc_val != NULL)
    {
      gtk_text_buffer_set_text (GTK_TEXT_BUFFER (priv->text_buffer), desc_val, -1);
      priv->reference_description = g_strstrip (g_strdup (desc_val));
    }
  else
    {
      /* We store "" in this case for ease further comparisons. */
      priv->reference_description = g_strdup ("");
    }

  if (tags_val != NULL)
    gtk_entry_set_text (GTK_ENTRY (priv->tags_entry), tags_val);

  if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (priv->public_rb)))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->public_rb), is_public_val);
  if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (priv->private_rb)))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->private_rb), !is_public_val);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->friend_cb),
                                is_friend_val);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->family_cb),
                                is_family_val);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->show_in_search_cb),
                                show_in_search_val);

  if (license_inconsistent)
    gtk_combo_box_set_active (GTK_COMBO_BOX (priv->license_cb), FSP_LICENSE_LAST + 1);
  else
    gtk_combo_box_set_active (GTK_COMBO_BOX (priv->license_cb), license_val + 1);

  if (content_type_val == FSP_CONTENT_TYPE_SCREENSHOT
      && !gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (priv->sshot_content_rb)))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->sshot_content_rb), TRUE);
  else if (content_type_val == FSP_CONTENT_TYPE_OTHER
      && !gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (priv->other_content_rb)))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->other_content_rb), TRUE);
  else if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (priv->photo_content_rb)))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->photo_content_rb), TRUE);

  if (safety_level_val == FSP_SAFETY_LEVEL_MODERATE
      && !gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (priv->moderate_rb)))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->moderate_rb), TRUE);
  else if (safety_level_val == FSP_SAFETY_LEVEL_RESTRICTED
      && !gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (priv->restricted_rb)))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->restricted_rb), TRUE);
  else if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (priv->safe_rb)))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->safe_rb), TRUE);

  picture_widget = gtk_bin_get_child (GTK_BIN (priv->picture_container));
  if (picture_widget)
    {
      gtk_container_remove (GTK_CONTAINER (priv->picture_container),
                            picture_widget);
    }

  if (priv->picture_button_handler_id)
    {
      g_signal_handler_disconnect (priv->picture_button,
                                   priv->picture_button_handler_id);
    }

  /* Update UI */
  _update_ui (self);

  /* Initial widget to grab focus */
  gtk_widget_grab_focus (priv->title_entry);
  gtk_editable_set_position (GTK_EDITABLE (priv->title_entry), -1);

  /* Load the picture from disk, asynchronously */
  _load_picture_from_disk (self);
}

static gboolean
_validate_dialog_data (FrogrDetailsDialog *self)
{
  FrogrDetailsDialogPrivate *priv =
    FROGR_DETAILS_DIALOG_GET_PRIVATE (self);

  gboolean result = TRUE;

  /* Mandatory fields (only if editing a single picture) */
  if (g_slist_length (priv->pictures) <= 1)
    {
      gchar *title =
        g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->title_entry)));

      if ((title == NULL) || g_str_equal (g_strstrip (title), ""))
        result = FALSE;

      g_free (title);
    }

  /* Validated if reached */
  return result;
}

static gboolean
_save_data (FrogrDetailsDialog *self)
{
  FrogrDetailsDialogPrivate *priv =
    FROGR_DETAILS_DIALOG_GET_PRIVATE (self);

  GtkTextIter start;
  GtkTextIter end;
  gchar *title = NULL;
  gchar *description = NULL;
  gchar *tags = NULL;
  gboolean is_public;
  gboolean is_friend;
  gboolean is_family;
  gboolean show_in_search;
  FspLicense license;
  FspSafetyLevel safety_level;
  FspContentType content_type;
  gboolean result = FALSE;

  /* Save data */
  title = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->title_entry)));
  title = g_strstrip (title);

  gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (priv->text_buffer),
                              &start, &end);
  description = gtk_text_buffer_get_text (GTK_TEXT_BUFFER (priv->text_buffer),
                                          &start, &end, FALSE);
  description = g_strstrip (description);

  tags = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->tags_entry)));
  tags = g_strstrip (tags);

  is_public =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->public_rb));

  if (!is_public)
    {
      is_friend =
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->friend_cb));
      is_family =
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->family_cb));
    }
  else
    {
      /* This data is not used for public images */
      is_friend = FALSE;
      is_family = FALSE;
    }

  show_in_search =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->show_in_search_cb));

  /* License type (values in the combo are shifted +1) */
  license = gtk_combo_box_get_active (GTK_COMBO_BOX (priv->license_cb)) - 1;

  /* Content type */
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->photo_content_rb)))
    content_type = FSP_CONTENT_TYPE_PHOTO;
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->sshot_content_rb)))
    content_type = FSP_CONTENT_TYPE_SCREENSHOT;
  else
    content_type = FSP_CONTENT_TYPE_OTHER;

  /* Safety level */
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->safe_rb)))
    safety_level = FSP_SAFETY_LEVEL_SAFE;
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->moderate_rb)))
    safety_level = FSP_SAFETY_LEVEL_MODERATE;
  else
    safety_level = FSP_SAFETY_LEVEL_RESTRICTED;

  /* validate dialog */
  if (_validate_dialog_data (self))
    {
      FrogrPicture *picture;
      GSList *item;
      guint n_pictures;

      /* Iterate over the rest of elements */
      n_pictures = g_slist_length (priv->pictures);
      for (item = priv->pictures; item; item = g_slist_next (item))
        {
          picture = FROGR_PICTURE (item->data);

          if (!g_str_equal (title, "") || (n_pictures <= 1))
            frogr_picture_set_title (picture, title);

          if (!g_str_equal (description, priv->reference_description))
            frogr_picture_set_description (picture, description);

          if (!g_str_equal (tags, "") || (n_pictures <= 1))
            frogr_picture_set_tags (picture, tags);

          if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (priv->public_rb)))
            frogr_picture_set_public (picture, is_public);
          if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (priv->friend_cb)))
            frogr_picture_set_friend (picture, is_friend);
          if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (priv->family_cb)))
            frogr_picture_set_family (picture, is_family);
          if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (priv->show_in_search_cb)))
            frogr_picture_set_show_in_search (picture, show_in_search);

          if (license >= FSP_LICENSE_NONE && license < FSP_LICENSE_LAST)
            frogr_picture_set_license (picture, license);

          if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (priv->photo_content_rb))
              && !gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (priv->sshot_content_rb))
              && !gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (priv->other_content_rb)))
            frogr_picture_set_content_type (picture, content_type);

          if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (priv->safe_rb))
              && !gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (priv->moderate_rb))
              && !gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (priv->restricted_rb)))
            frogr_picture_set_safety_level (picture, safety_level);

          /* Everything went fine */
          result = TRUE;
        }
    }
  else
    {
      /* This shows a dialog notifying the problem to the user */
      frogr_util_show_error_dialog (GTK_WINDOW (self), _("Missing data required"));
    }

  /* free */
  g_free (title);
  g_free (description);
  g_free (tags);

  /* Return result */
  return result;
}

/* Event handlers */

static void
_on_radio_button_clicked (GtkButton *tbutton, gpointer data)
{
  FrogrDetailsDialog *self = NULL;
  GtkWidget *button = NULL;
  GSList *buttons = NULL;
  GSList *item = NULL;

  g_return_if_fail (GTK_IS_RADIO_BUTTON (tbutton));

  buttons = gtk_radio_button_get_group (GTK_RADIO_BUTTON (tbutton));

  for (item = buttons; item; item = g_slist_next (item))
    {
      button = GTK_WIDGET (item->data);
      gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (button), FALSE);
    }

  self = FROGR_DETAILS_DIALOG (data);
  _update_ui (self);
}

static void
_on_toggle_button_toggled (GtkToggleButton *tbutton, gpointer data)
{
  FrogrDetailsDialog *self = FROGR_DETAILS_DIALOG (data);

  /* Reset consistence and update UI */
  gtk_toggle_button_set_inconsistent (tbutton, FALSE);
  _update_ui (self);
}

static void
_on_picture_button_clicked (GtkButton *button, gpointer data)
{
  FrogrDetailsDialog *self = FROGR_DETAILS_DIALOG (data);
  FrogrDetailsDialogPrivate *priv = FROGR_DETAILS_DIALOG_GET_PRIVATE (self);
  GSList *current_pic = NULL;
  GList *uris_list = NULL;

  for (current_pic = priv->pictures; current_pic; current_pic = g_slist_next (current_pic))
    {
      FrogrPicture *picture = FROGR_PICTURE (current_pic->data);
      uris_list = g_list_append (uris_list, (gchar*) frogr_picture_get_fileuri (picture));
    }

  frogr_util_open_images_in_viewer (uris_list);

  g_list_free (uris_list);
}

static void
_dialog_response_cb (GtkDialog *dialog, gint response, gpointer data)
{
  FrogrDetailsDialog *self = FROGR_DETAILS_DIALOG (dialog);

  /* Try to save data if response is OK */
  if (response == GTK_RESPONSE_OK && _save_data (self) == FALSE)
    return;

  gtk_widget_destroy (GTK_WIDGET (self));

  /* Ensure pictures are reordered on OK */
  if (response == GTK_RESPONSE_OK)
    frogr_controller_reorder_pictures (frogr_controller_get_instance ());
}

static void
_frogr_details_dialog_set_property (GObject *object,
                                    guint prop_id,
                                    const GValue *value,
                                    GParamSpec *pspec)
{
  FrogrDetailsDialogPrivate *priv = FROGR_DETAILS_DIALOG_GET_PRIVATE (object);

  switch (prop_id)
    {
    case PROP_PICTURES:
      priv->pictures = (GSList *) g_value_get_pointer (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
_frogr_details_dialog_get_property (GObject *object,
                                    guint prop_id,
                                    GValue *value,
                                    GParamSpec *pspec)
{
  FrogrDetailsDialogPrivate *priv = FROGR_DETAILS_DIALOG_GET_PRIVATE (object);

  switch (prop_id)
    {
    case PROP_PICTURES:
      g_value_set_pointer (value, priv->pictures);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
_frogr_details_dialog_dispose (GObject *object)
{
  FrogrDetailsDialogPrivate *priv = FROGR_DETAILS_DIALOG_GET_PRIVATE (object);

  if (priv->mpictures_pixbuf)
    {
      g_object_unref (priv->mpictures_pixbuf);
      priv->mpictures_pixbuf = NULL;
    }

  if (priv->pictures)
    {
      g_slist_foreach (priv->pictures, (GFunc)g_object_unref, NULL);
      g_slist_free (priv->pictures);
      priv->pictures = NULL;
    }

  if (priv->treemodel)
    {
      g_object_unref (priv->treemodel);
      priv->treemodel = NULL;
    }

  G_OBJECT_CLASS(frogr_details_dialog_parent_class)->dispose (object);
}

static void
_frogr_details_dialog_finalize (GObject *object)
{
  FrogrDetailsDialogPrivate *priv = FROGR_DETAILS_DIALOG_GET_PRIVATE (object);

  g_free (priv->reference_description);

  G_OBJECT_CLASS(frogr_details_dialog_parent_class)->finalize (object);
}

static void
frogr_details_dialog_class_init (FrogrDetailsDialogClass *klass)
{
  GObjectClass *obj_class = (GObjectClass *)klass;
  GParamSpec *pspec;

  /* GObject signals */
  obj_class->set_property = _frogr_details_dialog_set_property;
  obj_class->get_property = _frogr_details_dialog_get_property;
  obj_class->dispose = _frogr_details_dialog_dispose;
  obj_class->finalize = _frogr_details_dialog_finalize;

  /* Install properties */
  pspec = g_param_spec_pointer ("pictures",
                                "pictures",
                                "List of pictures for "
                                "the details dialog",
                                G_PARAM_READWRITE
                                | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (obj_class, PROP_PICTURES, pspec);

  g_type_class_add_private (obj_class, sizeof (FrogrDetailsDialogPrivate));
}

static void
frogr_details_dialog_init (FrogrDetailsDialog *self)
{
  FrogrDetailsDialogPrivate *priv =
    FROGR_DETAILS_DIALOG_GET_PRIVATE (self);

  priv->treemodel = NULL;
  priv->pictures = NULL;
  priv->picture_button_handler_id = 0;
  priv->reference_description = NULL;

  _create_widgets (self);

  gtk_dialog_add_buttons (GTK_DIALOG (self),
                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                          GTK_STOCK_OK, GTK_RESPONSE_OK,
                          NULL);
  gtk_container_set_border_width (GTK_CONTAINER (self), 6);

  g_signal_connect (G_OBJECT (self), "response",
                    G_CALLBACK (_dialog_response_cb), NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (self),
                                   GTK_RESPONSE_OK);
}


/* Public API */

void
frogr_details_dialog_show (GtkWindow *parent, GSList *fpictures, GSList *tags)
{
  FrogrDetailsDialog *self = NULL;
  GObject *new = NULL;

  new = g_object_new (FROGR_TYPE_DETAILS_DIALOG,
                      "modal", TRUE,
                      "pictures", fpictures,
                      "transient-for", parent,
                      "width-request", DIALOG_MIN_WIDTH,
                      "height-request", -1,
                      "resizable", TRUE,
                      "title", _("Edit Picture Details"),
                      NULL);

  self = FROGR_DETAILS_DIALOG (new);

  _fill_dialog_with_data (self);
  _populate_treemodel_with_tags (self, tags);
}
