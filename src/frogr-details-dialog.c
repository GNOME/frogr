/*
 * frogr-details-dialog.c -- Picture details dialog
 *
 * Copyright (C) 2009, 2010, 2011 Mario Sanchez Prada
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
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "frogr-details-dialog.h"

#include "frogr-controller.h"
#include "frogr-picture.h"
#include "frogr-util.h"

#include <config.h>
#include <glib/gi18n.h>
#include <flicksoup/flicksoup.h>

#define MPICTURES_IMAGE APP_DATA_DIR "/images/mpictures.png"

#define DIALOG_MIN_WIDTH 640
#define DIALOG_MIN_HEIGHT 420

#define PICTURE_WIDTH 120
#define PICTURE_HEIGHT 120

#define FROGR_DETAILS_DIALOG_GET_PRIVATE(object)                \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),                       \
                                FROGR_TYPE_DETAILS_DIALOG,      \
                                FrogrDetailsDialogPrivate))

G_DEFINE_TYPE (FrogrDetailsDialog, frogr_details_dialog, GTK_TYPE_DIALOG);

typedef struct _FrogrDetailsDialogPrivate {
  GtkWidget *title_entry;
  GtkWidget *desc_tview;
  GtkWidget *tags_entry;
  GtkWidget *public_rb;
  GtkWidget *private_rb;
  GtkWidget *friend_cb;
  GtkWidget *family_cb;
  GtkWidget *show_in_search_cb;
  GtkWidget *photo_content_rb;
  GtkWidget *sshot_content_rb;
  GtkWidget *other_content_rb;
  GtkWidget *safe_rb;
  GtkWidget *moderate_rb;
  GtkWidget *restricted_rb;
  GtkTextBuffer *text_buffer;
  GtkWidget *picture_img;
  GtkTreeModel *treemodel;
  GSList *pictures;
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


/* Prototypes */

static void _create_widgets (FrogrDetailsDialog *self);

static void _populate_treemodel_with_tags (FrogrDetailsDialog *self, GSList *tags);

static gboolean _tag_list_completion_func (GtkEntryCompletion *completion, const gchar *key,
                                           GtkTreeIter *iter, gpointer data);

static gboolean _completion_match_selected_cb (GtkEntryCompletion *widget, GtkTreeModel *model,
                                               GtkTreeIter *iter, gpointer data);

static void _update_ui (FrogrDetailsDialog *self);

static GdkPixbuf *_get_scaled_pixbuf (GdkPixbuf *pixbuf);

static void _fill_dialog_with_data (FrogrDetailsDialog *self);

static gboolean _validate_dialog_data (FrogrDetailsDialog *self);

static gboolean _save_data (FrogrDetailsDialog *self);

static void _on_radio_button_clicked (GtkButton *tbutton, gpointer data);

static void _on_toggle_button_toggled (GtkToggleButton *tbutton, gpointer data);

static void _dialog_response_cb (GtkDialog *dialog, gint response, gpointer data);


/* Private API */

static void
_create_widgets (FrogrDetailsDialog *self)
{
  FrogrDetailsDialogPrivate *priv = NULL;
  GtkWidget *main_vbox = NULL;
  GtkWidget *vbox = NULL;
  GtkWidget *hbox = NULL;
  GtkWidget *expander = NULL;
  GtkWidget *section_vbox = NULL;
  GtkWidget *internal_hbox = NULL;
  GtkWidget *visibility_vbox = NULL;
  GtkWidget *private_vbox = NULL;
  GtkWidget *content_type_hbox = NULL;
  GtkWidget *safety_level_hbox = NULL;
  GtkWidget *align = NULL;
  GtkWidget *widget = NULL;
  GtkWidget *table = NULL;
  GtkWidget *scroller = NULL;
  GtkEntryCompletion *completion = NULL;
  GtkTreeModel *model = NULL;
  gchar *markup = NULL;

  priv = FROGR_DETAILS_DIALOG_GET_PRIVATE (self);

#if GTK_CHECK_VERSION (2,14,0)
  main_vbox = gtk_dialog_get_content_area (GTK_DIALOG (self));
#else
  main_vbox = GTK_DIALOG (self)->vbox;
#endif

  hbox = gtk_hbox_new (FALSE, 0);
  vbox = gtk_vbox_new (FALSE, 6);

  /* Left side (image, radio buttons, checkboxes...) */

  widget = gtk_image_new ();
  gtk_widget_set_size_request (widget, PICTURE_WIDTH, -1);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 6);
  priv->picture_img = widget;

  /* Visibility */

  section_vbox = gtk_vbox_new (FALSE, 6);
  visibility_vbox = gtk_vbox_new (FALSE, 6);

  widget = gtk_label_new (NULL);
  gtk_label_set_use_markup (GTK_LABEL (widget), TRUE);
  markup = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>",
                                    _("Visibility"));
  gtk_label_set_markup (GTK_LABEL (widget), markup);
  g_free (markup);
  align = gtk_alignment_new (0, 0, 0, 0);
  gtk_container_add (GTK_CONTAINER (align), widget);
  gtk_box_pack_start (GTK_BOX (section_vbox), align, FALSE, FALSE, 0);

  internal_hbox = gtk_hbox_new (FALSE, 12);

  widget = gtk_radio_button_new (NULL);
  gtk_button_set_label (GTK_BUTTON (widget), _("Private"));
  gtk_box_pack_start (GTK_BOX (internal_hbox), widget, FALSE, FALSE, 0);
  priv->private_rb = widget;

  widget = gtk_radio_button_new_from_widget (GTK_RADIO_BUTTON (priv->private_rb));
  gtk_button_set_label (GTK_BUTTON (widget), _("Public"));
  gtk_box_pack_start (GTK_BOX (internal_hbox), widget, FALSE, FALSE, 0);
  priv->public_rb = widget;

  gtk_box_pack_start (GTK_BOX (visibility_vbox), internal_hbox, FALSE, FALSE, 0);

  private_vbox = gtk_vbox_new (FALSE, 6);

  widget = gtk_check_button_new_with_label (_("Visible to family"));
  gtk_box_pack_start (GTK_BOX (private_vbox), widget, FALSE, FALSE, 0);
  priv->family_cb = widget;

  widget = gtk_check_button_new_with_label (_("Visible to friends"));
  gtk_box_pack_start (GTK_BOX (private_vbox), widget, FALSE, FALSE, 0);
  priv->friend_cb = widget;

  internal_hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (internal_hbox), private_vbox, FALSE, FALSE, 12);
  gtk_box_pack_start (GTK_BOX (visibility_vbox), internal_hbox, FALSE, FALSE, 0);

  internal_hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (internal_hbox), visibility_vbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (section_vbox), internal_hbox, FALSE, FALSE, 0);

  internal_hbox = gtk_hbox_new (FALSE, 0);
  widget = gtk_check_button_new_with_label (_("Show up in global search results"));
  gtk_box_pack_start (GTK_BOX (internal_hbox), widget, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (section_vbox), internal_hbox, FALSE, FALSE, 0);
  priv->show_in_search_cb = widget;

  gtk_box_pack_start (GTK_BOX (vbox), section_vbox, FALSE, FALSE, 6);

  /* Content type */

  section_vbox = gtk_vbox_new (FALSE, 6);
  content_type_hbox = gtk_hbox_new (FALSE, 6);

  markup = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>",
                                    _("Content type"));
  expander = gtk_expander_new (markup);
  gtk_expander_set_use_markup (GTK_EXPANDER (expander), TRUE);
  g_free (markup);

  align = gtk_alignment_new (0, 0, 0, 0);
  gtk_container_add (GTK_CONTAINER (align), expander);
  gtk_box_pack_start (GTK_BOX (section_vbox), align, FALSE, FALSE, 0);

  widget = gtk_radio_button_new (NULL);
  gtk_button_set_label (GTK_BUTTON (widget), _("Photo"));
  gtk_box_pack_start (GTK_BOX (content_type_hbox), widget, FALSE, FALSE, 0);
  priv->photo_content_rb = widget;

  widget = gtk_radio_button_new_from_widget (GTK_RADIO_BUTTON (priv->photo_content_rb));
  gtk_button_set_label (GTK_BUTTON (widget), _("Screenshot"));
  gtk_box_pack_start (GTK_BOX (content_type_hbox), widget, FALSE, FALSE, 0);
  priv->sshot_content_rb = widget;

  widget = gtk_radio_button_new_from_widget (GTK_RADIO_BUTTON (priv->photo_content_rb));
  gtk_button_set_label (GTK_BUTTON (widget), _("Other"));
  gtk_box_pack_start (GTK_BOX (content_type_hbox), widget, FALSE, FALSE, 0);
  priv->other_content_rb = widget;

  gtk_container_add (GTK_CONTAINER (expander), content_type_hbox);

  gtk_box_pack_start (GTK_BOX (vbox), section_vbox, FALSE, FALSE, 6);

  /* Safety level */

  section_vbox = gtk_vbox_new (FALSE, 6);
  safety_level_hbox = gtk_hbox_new (FALSE, 6);

  markup = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>",
                                    _("Safety level"));
  expander = gtk_expander_new (markup);
  gtk_expander_set_use_markup (GTK_EXPANDER (expander), TRUE);
  g_free (markup);

  align = gtk_alignment_new (0, 0, 0, 0);
  gtk_container_add (GTK_CONTAINER (align), expander);
  gtk_box_pack_start (GTK_BOX (section_vbox), align, FALSE, FALSE, 0);

  widget = gtk_radio_button_new (NULL);
  gtk_button_set_label (GTK_BUTTON (widget), _("Safe"));
  gtk_box_pack_start (GTK_BOX (safety_level_hbox), widget, FALSE, FALSE, 0);
  priv->safe_rb = widget;

  widget = gtk_radio_button_new_from_widget (GTK_RADIO_BUTTON (priv->safe_rb));
  gtk_button_set_label (GTK_BUTTON (widget), _("Moderate"));
  gtk_box_pack_start (GTK_BOX (safety_level_hbox), widget, FALSE, FALSE, 0);
  priv->moderate_rb = widget;

  widget = gtk_radio_button_new_from_widget (GTK_RADIO_BUTTON (priv->safe_rb));
  gtk_button_set_label (GTK_BUTTON (widget), _("Restricted"));
  gtk_box_pack_start (GTK_BOX (safety_level_hbox), widget, FALSE, FALSE, 0);
  priv->restricted_rb = widget;

  gtk_container_add (GTK_CONTAINER (expander), safety_level_hbox);

  gtk_box_pack_start (GTK_BOX (vbox), section_vbox, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 6);

  /* Right side (text fields) */

  table = gtk_table_new (3, 2, FALSE);

  widget = gtk_label_new (_("Title:"));
  align = gtk_alignment_new (1, 0, 1, 0);
  gtk_container_add (GTK_CONTAINER (align), widget);
  gtk_table_attach (GTK_TABLE (table), align, 0, 1, 0, 1,
                    0, 0, 6, 6);
  widget = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), widget, 1, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, 0, 6, 6);
  priv->title_entry = widget;

  widget = gtk_label_new (_("Description:"));
  align = gtk_alignment_new (1, 0, 1, 0);
  gtk_container_add (GTK_CONTAINER (align), widget);
  gtk_table_attach (GTK_TABLE (table), align, 0, 1, 1, 2,
                    0, GTK_EXPAND | GTK_FILL, 6, 6);

  widget = gtk_text_view_new ();
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

  widget = gtk_label_new (_("Tags:"));
  align = gtk_alignment_new (1, 0, 1, 0);
  gtk_container_add (GTK_CONTAINER (align), widget);
  gtk_table_attach (GTK_TABLE (table), align, 0, 1, 2, 3,
                    0, 0, 6, 6);
  widget = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), widget, 1, 2, 2, 3,
                    GTK_EXPAND | GTK_FILL, 0, 6, 6);
  priv->tags_entry = widget;

  gtk_box_pack_start (GTK_BOX (hbox), table, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 6);


  /* Prepare auto completion for tags */
  completion = gtk_entry_completion_new ();
  gtk_entry_completion_set_text_column (GTK_ENTRY_COMPLETION (completion), TEXT_COL);
  gtk_entry_completion_set_inline_completion (GTK_ENTRY_COMPLETION (completion), TRUE);
  gtk_entry_completion_set_match_func (GTK_ENTRY_COMPLETION (completion),
                                       _tag_list_completion_func,
                                       self, NULL);
  gtk_entry_set_completion (GTK_ENTRY (priv->tags_entry), completion);

  model = GTK_TREE_MODEL (gtk_list_store_new (1, G_TYPE_STRING));
  gtk_entry_completion_set_model (GTK_ENTRY_COMPLETION (completion), model);
  priv->treemodel = model;

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

  g_signal_connect (G_OBJECT (completion), "match-selected",
                    G_CALLBACK (_completion_match_selected_cb), self);

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
  gchar *stripped_key = NULL;
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
  if (cursor_pos < g_utf8_strlen (key, -1))
    return FALSE;

  /* Look for the last token in 'key' */
  stripped_key = g_strstrip (g_strndup (key, cursor_pos));
  basetext = g_strrstr (stripped_key, " ");
  if (basetext)
    basetext++;
  else
    basetext = stripped_key;

  /* Downcase everything and compare */
  lc_basetext = g_utf8_strdown (basetext, -1);
  lc_tag = g_utf8_strdown (tag, -1);
  if (g_str_has_prefix (lc_tag, lc_basetext))
    matches = TRUE;

  g_free (stripped_key);
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
  priv = FROGR_DETAILS_DIALOG_GET_PRIVATE (data);
  gtk_tree_model_get (model, iter, TEXT_COL, &tag, -1);

  entry_text = gtk_entry_get_text (GTK_ENTRY (priv->tags_entry));
  matching_text = g_strrstr (entry_text, " ");
  if (matching_text)
    matching_text++;
  else
    matching_text = entry_text;

  entry_text_len = g_utf8_strlen (entry_text, -1);
  matching_text_len = g_utf8_strlen (matching_text, -1);

  base_text = g_strndup (entry_text, entry_text_len - matching_text_len);
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

static GdkPixbuf *
_get_scaled_pixbuf (GdkPixbuf *pixbuf)
{
  GdkPixbuf *scaled_pixbuf = NULL;
  gint width;
  gint height;
  gint new_width;
  gint new_height;

  /* Look for the right side to reduce */
  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);
  if (width > height)
    {
      new_width = PICTURE_WIDTH;
      new_height = (float)new_width * height / width;
    }
  else
    {
      new_height = PICTURE_HEIGHT;
      new_width = (float)new_height * width / height;
    }

  /* Scale the pixbuf to its best size */
  scaled_pixbuf = gdk_pixbuf_scale_simple (pixbuf,
                                           new_width, new_height,
                                           GDK_INTERP_TILES);

  return scaled_pixbuf;
}

static void
_fill_dialog_with_data (FrogrDetailsDialog *self)
{
  FrogrDetailsDialogPrivate *priv =
    FROGR_DETAILS_DIALOG_GET_PRIVATE (self);

  FrogrPicture *picture;
  GSList *item;
  guint n_pictures;
  gchar *title_val = NULL;
  gchar *desc_val = NULL;
  gchar *tags_val = NULL;
  gboolean is_public_val = FALSE;
  gboolean is_friend_val = FALSE;
  gboolean is_family_val = FALSE;
  gboolean show_in_search_val = FALSE;
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
      content_type_val = content_type;
      safety_level_val = safety_level;
    }

  /* Fill in with data */
  if (title_val != NULL)
    gtk_entry_set_text (GTK_ENTRY (priv->title_entry), title_val);

  if (desc_val != NULL)
    gtk_text_buffer_set_text (GTK_TEXT_BUFFER (priv->text_buffer),
                              desc_val, -1);
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

  n_pictures = g_slist_length (priv->pictures);
  if (n_pictures > 1)
    {
      GdkPixbuf *pixbuf;

      /* Set the image for editing multiple pictures */
      pixbuf = gdk_pixbuf_new_from_file (MPICTURES_IMAGE, NULL);
      gtk_image_set_from_pixbuf (GTK_IMAGE (priv->picture_img), pixbuf);
      g_object_unref (pixbuf);
    }
  else
    {
      /* Set pixbuf scaled to the right size */
      GdkPixbuf *pixbuf = frogr_picture_get_pixbuf (picture);
      GdkPixbuf *s_pixbuf = _get_scaled_pixbuf (pixbuf);
      gtk_image_set_from_pixbuf (GTK_IMAGE (priv->picture_img), s_pixbuf);
      g_object_unref (s_pixbuf);
    }

  /* Update UI */
  _update_ui (self);

  /* Initial widget to grab focus */
  gtk_widget_grab_focus (priv->title_entry);
  gtk_editable_set_position (GTK_EDITABLE (priv->title_entry), -1);
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

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->photo_content_rb)))
    content_type = FSP_CONTENT_TYPE_PHOTO;
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->sshot_content_rb)))
    content_type = FSP_CONTENT_TYPE_SCREENSHOT;
  else
    content_type = FSP_CONTENT_TYPE_OTHER;

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
          if (!g_str_equal (description, "") || (n_pictures <= 1))
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
  g_return_if_fail (GTK_IS_RADIO_BUTTON (tbutton));

  FrogrDetailsDialog *self = NULL;
  GtkWidget *button = NULL;
  GSList *buttons = NULL;
  GSList *item = NULL;

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

static void _dialog_response_cb (GtkDialog *dialog, gint response, gpointer data)
{
  FrogrDetailsDialog *self = FROGR_DETAILS_DIALOG (dialog);

  /* Try to save data if response is OK */
  if (response == GTK_RESPONSE_OK && _save_data (self) == FALSE)
      return;

  gtk_widget_destroy (GTK_WIDGET (self));
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
frogr_details_dialog_class_init (FrogrDetailsDialogClass *klass)
{
  GObjectClass *obj_class = (GObjectClass *)klass;
  GParamSpec *pspec;

  /* GObject signals */
  obj_class->set_property = _frogr_details_dialog_set_property;
  obj_class->get_property = _frogr_details_dialog_get_property;
  obj_class->dispose = _frogr_details_dialog_dispose;

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

  /* Show the UI */
  gtk_widget_show_all (GTK_WIDGET (self));
}


/* Public API */

void
frogr_details_dialog_show (GtkWindow *parent, GSList *fpictures, GSList *tags)
{
  FrogrDetailsDialog *self = NULL;
  gchar *title = NULL;
  guint n_pictures = 0;
  GObject *new = NULL;

  n_pictures = g_slist_length (fpictures);
  if (n_pictures > 1)
    {
      title = g_strdup_printf ("%s (%d Pictures)",
                               _("Edit Picture Details"),
                               n_pictures);
    }
  else
    {
      title = g_strdup (_("Edit Picture Details"));
    }

  new = g_object_new (FROGR_TYPE_DETAILS_DIALOG,
                      "modal", TRUE,
                      "pictures", fpictures,
                      "transient-for", parent,
                      "width-request", DIALOG_MIN_WIDTH,
                      "height-request", -1,
                      "resizable", TRUE,
                      "title", title,
                      NULL);
  g_free (title);

  self = FROGR_DETAILS_DIALOG (new);

  _fill_dialog_with_data (self);
  _populate_treemodel_with_tags (self, tags);

  gtk_widget_show_all (GTK_WIDGET (self));
}
