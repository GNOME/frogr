/*
 * frogr-add-tags-dialog.c -- Frogr 'add tags' dialog
 *
 * Copyright (C) 2009, 2010 Mario Sanchez Prada
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

#include "frogr-add-tags-dialog.h"

#include "frogr-picture.h"

#include <config.h>
#include <glib/gi18n.h>

#define FROGR_ADD_TAGS_DIALOG_GET_PRIVATE(object)               \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),                       \
                                FROGR_TYPE_ADD_TAGS_DIALOG,     \
                                FrogrAddTagsDialogPrivate))

G_DEFINE_TYPE (FrogrAddTagsDialog, frogr_add_tags_dialog, GTK_TYPE_DIALOG);

typedef struct _FrogrAddTagsDialogPrivate {
  GtkWidget *entry;
  GtkTreeModel *treemodel;

  GSList *pictures;
  gchar *tags;
} FrogrAddTagsDialogPrivate;

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

static void _populate_treemodel_with_tags (FrogrAddTagsDialog *self, GSList *tags);

static gboolean _tag_list_completion_func (GtkEntryCompletion *completion, const gchar *key,
                                           GtkTreeIter *iter, gpointer data);

static gboolean _completion_match_selected_cb (GtkEntryCompletion *widget, GtkTreeModel *model,
                                               GtkTreeIter *iter, gpointer data);

static void _dialog_response_cb (GtkDialog *dialog, gint response, gpointer data);

/* Private API */

static void
_populate_treemodel_with_tags (FrogrAddTagsDialog *self, GSList *tags)
{
  FrogrAddTagsDialogPrivate *priv = NULL;
  GSList *item = NULL;
  gchar *tag = NULL;
  GtkTreeIter iter;

  priv = FROGR_ADD_TAGS_DIALOG_GET_PRIVATE (self);

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
  FrogrAddTagsDialog *self = NULL;
  FrogrAddTagsDialogPrivate *priv = NULL;
  gchar *stripped_key = NULL;
  gchar *basetext = NULL;
  gchar *tag = NULL;
  gchar *lc_basetext = NULL;
  gchar *lc_tag = NULL;
  gint cursor_pos = 0;
  gboolean matches = FALSE;

  self = FROGR_ADD_TAGS_DIALOG (data);
  priv = FROGR_ADD_TAGS_DIALOG_GET_PRIVATE (self);
  gtk_tree_model_get (priv->treemodel, iter, TEXT_COL, &tag, -1);

  /* Do nothing if not a valid tag */
  if (!tag)
    return FALSE;

  /* Do nothing if the cursor is not in the last position */
  cursor_pos = gtk_editable_get_position (GTK_EDITABLE (priv->entry));
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
  FrogrAddTagsDialog *self = NULL;
  FrogrAddTagsDialogPrivate *priv = NULL;
  gchar *tag = NULL;
  const gchar *entry_text = NULL;
  const gchar *matching_text = NULL;
  gchar *base_text = NULL;
  gchar *new_text = NULL;
  glong entry_text_len = 0;
  glong matching_text_len = 0;

  self = FROGR_ADD_TAGS_DIALOG (data);
  priv = FROGR_ADD_TAGS_DIALOG_GET_PRIVATE (data);
  gtk_tree_model_get (model, iter, TEXT_COL, &tag, -1);

  entry_text = gtk_entry_get_text (GTK_ENTRY (priv->entry));
  matching_text = g_strrstr (entry_text, " ");
  if (matching_text)
    matching_text++;
  else
    matching_text = entry_text;

  entry_text_len = g_utf8_strlen (entry_text, -1);
  matching_text_len = g_utf8_strlen (matching_text, -1);

  base_text = g_strndup (entry_text, entry_text_len - matching_text_len);
  new_text = g_strdup_printf ("%s%s", base_text, tag);

  gtk_entry_set_text (GTK_ENTRY (priv->entry), new_text);
  gtk_editable_set_position (GTK_EDITABLE (priv->entry), -1);

  g_free (tag);
  g_free (base_text);
  g_free (new_text);

  return TRUE;
}

static void
_dialog_response_cb (GtkDialog *dialog, gint response, gpointer data)
{
  if (response == GTK_RESPONSE_OK)
    {
      FrogrAddTagsDialog *self = NULL;
      FrogrAddTagsDialogPrivate *priv = NULL;
      gchar *tags = NULL;

      self = FROGR_ADD_TAGS_DIALOG (dialog);
      priv = FROGR_ADD_TAGS_DIALOG_GET_PRIVATE (self);

      /* Update pictures data */
      tags = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->entry)));
      tags = g_strstrip (tags);

      /* Check if there's something to add */
      if (tags && !g_str_equal (tags, ""))
        {
          FrogrPicture *picture;
          GSList *item;
          guint n_pictures;

          g_debug ("Adding tags to picture(s): %s", tags);

          /* Iterate over the rest of elements */
          n_pictures = g_slist_length (priv->pictures);
          for (item = priv->pictures; item; item = g_slist_next (item))
            {
              picture = FROGR_PICTURE (item->data);
              frogr_picture_add_tags (picture, tags);
            }
        }

      /* Free */
      g_free (tags);
    }

  /* Destroy the dialog */
  gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
_frogr_add_tags_dialog_set_property (GObject *object,
                                     guint prop_id,
                                     const GValue *value,
                                     GParamSpec *pspec)
{
  FrogrAddTagsDialogPrivate *priv = FROGR_ADD_TAGS_DIALOG_GET_PRIVATE (object);

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
_frogr_add_tags_dialog_get_property (GObject *object,
                                     guint prop_id,
                                     GValue *value,
                                     GParamSpec *pspec)
{
  FrogrAddTagsDialogPrivate *priv = FROGR_ADD_TAGS_DIALOG_GET_PRIVATE (object);

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
_frogr_add_tags_dialog_dispose (GObject *object)
{
  FrogrAddTagsDialogPrivate *priv = FROGR_ADD_TAGS_DIALOG_GET_PRIVATE (object);

  if (priv->treemodel)
    {
      g_object_unref (priv->treemodel);
      priv->treemodel = NULL;
    }

  if (priv->pictures)
    {
      g_slist_foreach (priv->pictures, (GFunc)g_object_unref, NULL);
      g_slist_free (priv->pictures);
      priv->pictures = NULL;
    }

  G_OBJECT_CLASS(frogr_add_tags_dialog_parent_class)->dispose (object);
}

static void
_frogr_add_tags_dialog_finalize (GObject *object)
{
  FrogrAddTagsDialogPrivate *priv = FROGR_ADD_TAGS_DIALOG_GET_PRIVATE (object);

  g_free (priv->tags);

  G_OBJECT_CLASS(frogr_add_tags_dialog_parent_class)->finalize (object);
}

static void
frogr_add_tags_dialog_class_init (FrogrAddTagsDialogClass *klass)
{
  GObjectClass *obj_class = (GObjectClass *)klass;
  GParamSpec *pspec;

  /* GObject signals */
  obj_class->set_property = _frogr_add_tags_dialog_set_property;
  obj_class->get_property = _frogr_add_tags_dialog_get_property;
  obj_class->dispose = _frogr_add_tags_dialog_dispose;
  obj_class->finalize = _frogr_add_tags_dialog_finalize;

  /* Install properties */
  pspec = g_param_spec_pointer ("pictures",
                                "pictures",
                                "List of pictures for "
                                "the 'add tags' dialog",
                                G_PARAM_READWRITE
                                | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (obj_class, PROP_PICTURES, pspec);

  g_type_class_add_private (obj_class, sizeof (FrogrAddTagsDialogPrivate));
}

static void
frogr_add_tags_dialog_init (FrogrAddTagsDialog *self)
{
  FrogrAddTagsDialogPrivate *priv = FROGR_ADD_TAGS_DIALOG_GET_PRIVATE (self);
  GtkWidget *vbox = NULL;
  GtkWidget *align = NULL;
  GtkWidget *label = NULL;
  GtkEntryCompletion *completion = NULL;
  GtkTreeModel *model = NULL;

  /* Create widgets */
  gtk_dialog_add_buttons (GTK_DIALOG (self),
                          GTK_STOCK_OK,
                          GTK_RESPONSE_OK,
                          GTK_STOCK_CANCEL,
                          GTK_RESPONSE_CANCEL,
                          NULL);
  gtk_container_set_border_width (GTK_CONTAINER (self), 6);

  /* Add Entry */
#if GTK_CHECK_VERSION (2,14,0)
  vbox = gtk_dialog_get_content_area (GTK_DIALOG (self));
#else
  vbox = GTK_DIALOG (self)->vbox;
#endif

  label = gtk_label_new (_("Enter a spaces separated list of tags:"));
  align = gtk_alignment_new (0, 0, 0, 1);
  gtk_container_add (GTK_CONTAINER (align), label);
  gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 6);

  completion = gtk_entry_completion_new ();
  gtk_entry_completion_set_text_column (GTK_ENTRY_COMPLETION (completion), TEXT_COL);
  gtk_entry_completion_set_inline_completion (GTK_ENTRY_COMPLETION (completion), TRUE);
  gtk_entry_completion_set_match_func (GTK_ENTRY_COMPLETION (completion),
                                       _tag_list_completion_func,
                                       self, NULL);

  model = GTK_TREE_MODEL (gtk_list_store_new (1, G_TYPE_STRING));
  gtk_entry_completion_set_model (GTK_ENTRY_COMPLETION (completion), model);
  priv->treemodel = model;

  priv->entry = gtk_entry_new ();
  gtk_entry_set_completion (GTK_ENTRY (priv->entry), completion);

  gtk_box_pack_start (GTK_BOX (vbox), priv->entry, TRUE, FALSE, 6);
  gtk_widget_set_size_request (GTK_WIDGET (self), 300, -1);

  g_signal_connect (G_OBJECT (completion), "match-selected",
                    G_CALLBACK (_completion_match_selected_cb), self);

  g_signal_connect (G_OBJECT (self), "response",
                    G_CALLBACK (_dialog_response_cb), NULL);

  /* Show the UI */
  gtk_widget_show_all (GTK_WIDGET (self));
}

/* Public API */

void
frogr_add_tags_dialog_show (GtkWindow *parent, GSList *pictures, GSList *tags)
{
  FrogrAddTagsDialog *self = NULL;
  GObject *new = NULL;

  new = g_object_new (FROGR_TYPE_ADD_TAGS_DIALOG,
                      "title", _("Add Tags"),
                      "modal", TRUE,
                      "pictures", pictures,
                      "transient-for", parent,
                      "resizable", FALSE,
                      "has-separator", FALSE,
                      NULL);

  self = FROGR_ADD_TAGS_DIALOG (new);
  _populate_treemodel_with_tags (self, tags);

  gtk_widget_show_all (GTK_WIDGET (self));
}
