/*
 * frogr-live-entry.c -- Frogr 'live' entry (autocompletion enabled)
 *
 * Copyright (C) 2011 Mario Sanchez Prada
 * Authors: Mario Sanchez Prada <msanchez@gnome.org>
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

#include "frogr-live-entry.h"

#include "frogr-global-defs.h"
#include "frogr-picture.h"

#include <config.h>
#include <glib/gi18n.h>

#define FROGR_LIVE_ENTRY_GET_PRIVATE(object)            \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),               \
                                FROGR_TYPE_LIVE_ENTRY,  \
                                FrogrLiveEntryPrivate))

G_DEFINE_TYPE (FrogrLiveEntry, frogr_live_entry, GTK_TYPE_ENTRY)

typedef struct _FrogrLiveEntryPrivate {
  GtkEntryCompletion *entry_completion;
  GtkTreeModel *treemodel;
  gboolean auto_completion;
} FrogrLiveEntryPrivate;


/* Tree view columns */
enum {
  TEXT_COL,
  N_COLS
};


/* Prototypes */

static void _populate_treemodel_with_data (GtkTreeModel *treemodel, GSList *entries);

static gboolean _entry_list_completion_func (GtkEntryCompletion *completion, const gchar *key,
                                             GtkTreeIter *iter, gpointer data);

static gboolean _completion_match_selected_cb (GtkEntryCompletion *widget, GtkTreeModel *model,
                                               GtkTreeIter *iter, gpointer data);

/* Private API */

static void
_populate_treemodel_with_data (GtkTreeModel *treemodel, GSList *entries)
{
  if (treemodel && entries)
    {
      GSList *item = NULL;
      gchar *entry = NULL;
      GtkTreeIter iter;

      /* Initialize the list store */
      for (item = entries; item; item = g_slist_next (item))
        {
          entry = (gchar *) item->data;
          gtk_list_store_append (GTK_LIST_STORE (treemodel), &iter);
          gtk_list_store_set (GTK_LIST_STORE (treemodel), &iter,
                              0, entry, -1);
        }
    }
  else if (treemodel)
    {
      /* Clear the list store */
      gtk_list_store_clear (GTK_LIST_STORE (treemodel));
    }
}

static gboolean
_entry_list_completion_func (GtkEntryCompletion *completion, const gchar *key,
                             GtkTreeIter *iter, gpointer data)
{
  FrogrLiveEntry *self = NULL;
  FrogrLiveEntryPrivate *priv = NULL;
  gchar *stripped_entry_text = NULL;
  gchar *basetext = NULL;
  gchar *entry = NULL;
  gchar *lc_basetext = NULL;
  gchar *lc_entry = NULL;
  gint cursor_pos = 0;
  gboolean matches = FALSE;

  self = FROGR_LIVE_ENTRY (data);
  priv = FROGR_LIVE_ENTRY_GET_PRIVATE (self);
  gtk_tree_model_get (priv->treemodel, iter, TEXT_COL, &entry, -1);

  /* Do nothing if not a valid entry */
  if (!entry)
    return FALSE;

  /* Do nothing if the cursor is not in the last position */
  cursor_pos = gtk_editable_get_position (GTK_EDITABLE (self));
  if (cursor_pos < gtk_entry_get_text_length (GTK_ENTRY (self)))
    return FALSE;

  /* Look for the last token in 'key' */
  stripped_entry_text = gtk_editable_get_chars (GTK_EDITABLE (self), 0, cursor_pos);
  stripped_entry_text = g_strstrip (stripped_entry_text);
  basetext = g_strrstr (stripped_entry_text, " ");
  if (basetext)
    basetext++;
  else
    basetext = stripped_entry_text;

  /* Downcase everything and compare */
  lc_basetext = g_utf8_strdown (basetext, -1);
  lc_entry = g_utf8_strdown (entry, -1);
  if (g_str_has_prefix (lc_entry, lc_basetext))
    matches = TRUE;

  g_free (stripped_entry_text);
  g_free (entry);
  g_free (lc_basetext);
  g_free (lc_entry);

  return matches;
}

static gboolean
_completion_match_selected_cb (GtkEntryCompletion *widget, GtkTreeModel *model,
                               GtkTreeIter *iter, gpointer data)
{
  FrogrLiveEntry *self = NULL;
  gchar *entry = NULL;
  const gchar *entry_text = NULL;
  const gchar *matching_text = NULL;
  gchar *base_text = NULL;
  gchar *new_text = NULL;
  glong entry_text_len = 0;
  glong matching_text_len = 0;

  self = FROGR_LIVE_ENTRY (data);
  gtk_tree_model_get (model, iter, TEXT_COL, &entry, -1);

  entry_text = gtk_entry_get_text (GTK_ENTRY (self));
  matching_text = g_strrstr (entry_text, " ");
  if (matching_text)
    matching_text++;
  else
    matching_text = entry_text;

  entry_text_len = gtk_entry_get_text_length (GTK_ENTRY (self));
  matching_text_len = g_utf8_strlen (matching_text, -1);

  base_text = gtk_editable_get_chars (GTK_EDITABLE (self), 0, entry_text_len - matching_text_len);
  new_text = g_strdup_printf ("%s%s ", base_text, entry);

  gtk_entry_set_text (GTK_ENTRY (self), new_text);
  gtk_editable_set_position (GTK_EDITABLE (self), -1);

  g_free (entry);
  g_free (base_text);
  g_free (new_text);

  return TRUE;
}

static void
_frogr_live_entry_dispose (GObject *object)
{
  FrogrLiveEntryPrivate *priv = FROGR_LIVE_ENTRY_GET_PRIVATE (object);

  if (priv->treemodel)
    {
      g_object_unref (priv->treemodel);
      priv->treemodel = NULL;
    }

  G_OBJECT_CLASS(frogr_live_entry_parent_class)->dispose (object);
}

static void
frogr_live_entry_class_init (FrogrLiveEntryClass *klass)
{
  GObjectClass *obj_class = (GObjectClass *)klass;

  /* GObject signals */
  obj_class->dispose = _frogr_live_entry_dispose;

  g_type_class_add_private (obj_class, sizeof (FrogrLiveEntryPrivate));
}

static void
frogr_live_entry_init (FrogrLiveEntry *self)
{
  FrogrLiveEntryPrivate *priv = FROGR_LIVE_ENTRY_GET_PRIVATE (self);

  priv->entry_completion = NULL;
  priv->treemodel = NULL;
  priv->auto_completion = FALSE;
}

/* Public API */

GtkWidget *
frogr_live_entry_new (void)
{
  return GTK_WIDGET (g_object_new (FROGR_TYPE_LIVE_ENTRY, NULL));
}

void
frogr_live_entry_set_auto_completion (FrogrLiveEntry *self, GSList *data)
{
  FrogrLiveEntryPrivate *priv = NULL;

  priv = FROGR_LIVE_ENTRY_GET_PRIVATE (self);
  priv->auto_completion = data ? TRUE : FALSE;

  /* Initialize the auto-completion stuff if it's the first time */
  if (priv->auto_completion && priv->entry_completion == NULL)
    {
      GtkTreeModel *model = NULL;

      model = GTK_TREE_MODEL (gtk_list_store_new (1, G_TYPE_STRING));
      priv->treemodel = model;

      priv->entry_completion = gtk_entry_completion_new ();
      gtk_entry_completion_set_text_column (GTK_ENTRY_COMPLETION (priv->entry_completion), TEXT_COL);
      gtk_entry_completion_set_inline_completion (GTK_ENTRY_COMPLETION (priv->entry_completion), TRUE);
      gtk_entry_completion_set_match_func (GTK_ENTRY_COMPLETION (priv->entry_completion),
                                           _entry_list_completion_func,
                                           self, NULL);

      gtk_entry_completion_set_model (GTK_ENTRY_COMPLETION (priv->entry_completion), model);

      g_signal_connect (G_OBJECT (priv->entry_completion), "match-selected",
                        G_CALLBACK (_completion_match_selected_cb), self);
    }

  /* Enable or disable auto-completion as needed */
  gtk_entry_set_completion (GTK_ENTRY (self), data ? priv->entry_completion: NULL);

  /* Populate the tree model with the data (or 'no data) passed */
  if (priv->treemodel)
    _populate_treemodel_with_data (priv->treemodel, data);
}
