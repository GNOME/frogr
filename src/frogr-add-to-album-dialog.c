/*
 * frogr-add-to-album-dialog.c -- 'Add to album' dialog
 *
 * Copyright (C) 2010 Mario Sanchez Prada
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

#include "frogr-add-to-album-dialog.h"

#include "frogr-album.h"
#include "frogr-picture.h"

#include <config.h>
#include <glib/gi18n.h>

#define MINIMUM_WINDOW_WIDTH 540
#define MINIMUM_WINDOW_HEIGHT 420

#define FROGR_ADD_TO_ALBUM_DIALOG_GET_PRIVATE(object)           \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),                       \
                                FROGR_TYPE_ADD_TO_ALBUM_DIALOG, \
                                FrogrAddToAlbumDialogPrivate))

G_DEFINE_TYPE (FrogrAddToAlbumDialog, frogr_add_to_album_dialog, GTK_TYPE_DIALOG);

typedef struct _FrogrAddToAlbumDialogPrivate {
  GtkWidget *treeview;
  GtkTreeModel *treemodel;

  GtkTreeViewColumn *checkbox_col;
  GtkTreeViewColumn *title_col;
  GtkTreeViewColumn *n_elements_col;

  GSList *pictures;
  GSList *albums;
} FrogrAddToAlbumDialogPrivate;

/* Properties */
enum  {
  PROP_0,
  PROP_PICTURES,
  PROP_ALBUMS
};


/* Tree view columns */
enum {
  CHECKBOX_COL,
  TITLE_COL,
  N_ELEMENTS_COL,
  ALBUM_COL,
  N_COLS
};


/* Prototypes */

static GtkWidget *_create_tree_view (FrogrAddToAlbumDialog *self);

static void _column_clicked_cb (GtkTreeViewColumn *col, gpointer data);

static void _toggle_column_sort_order (GtkTreeSortable *sortable,
                                       GtkTreeViewColumn *col,
                                       gint col_id);

static gint _tree_iter_compare_n_elements_func (GtkTreeModel *model,
                                                GtkTreeIter *a,
                                                GtkTreeIter *b,
                                                gpointer data);

static void _populate_treemodel_with_albums (FrogrAddToAlbumDialog *self);

static void _fill_dialog_with_data (FrogrAddToAlbumDialog *self);

static void _album_toggled_cb (GtkCellRendererToggle *celltoggle,
                               gchar *path_string,
                               GtkTreeView *treeview);

static GSList *_get_selected_albums (FrogrAddToAlbumDialog *self);

static void _update_pictures (FrogrAddToAlbumDialog *self);

static void _dialog_response_cb (GtkDialog *dialog, gint response, gpointer data);

/* Private API */

static GtkWidget *
_create_tree_view (FrogrAddToAlbumDialog *self)
{
  FrogrAddToAlbumDialogPrivate *priv = NULL;
  GtkWidget *treeview = NULL;
  GtkTreeViewColumn *col = NULL;
  GtkCellRenderer *rend = NULL;

  priv = FROGR_ADD_TO_ALBUM_DIALOG_GET_PRIVATE (self);
  treeview = gtk_tree_view_new();

  /* Checkbox */
  rend = gtk_cell_renderer_toggle_new ();
  col = gtk_tree_view_column_new_with_attributes (NULL,
                                                  rend,
                                                  "active", CHECKBOX_COL,
                                                  NULL);
  gtk_tree_view_column_set_clickable (col, TRUE);
  gtk_tree_view_column_set_sort_order (col, GTK_SORT_ASCENDING);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), col);

  g_signal_connect (rend, "toggled",
                    G_CALLBACK (_album_toggled_cb), treeview);

  g_signal_connect (col, "clicked",
                    G_CALLBACK (_column_clicked_cb), self);

  priv->checkbox_col = col;

  /* Title */
  rend = gtk_cell_renderer_text_new ();
  col = gtk_tree_view_column_new_with_attributes (_("Album Title"),
                                                  rend,
                                                  "text", TITLE_COL,
                                                  NULL);
  gtk_tree_view_column_set_clickable (col, TRUE);
  gtk_tree_view_column_set_sort_order (col, GTK_SORT_DESCENDING);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), col);

  g_signal_connect (col, "clicked",
                    G_CALLBACK (_column_clicked_cb), self);

  priv->title_col = col;

  /* Description */
  rend = gtk_cell_renderer_text_new ();
  col = gtk_tree_view_column_new_with_attributes (_("Elements"),
                                                  rend,
                                                  "text", N_ELEMENTS_COL,
                                                  NULL);
  gtk_tree_view_column_set_clickable (col, TRUE);
  gtk_tree_view_column_set_sort_order (col, GTK_SORT_DESCENDING);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), col);

  g_signal_connect (col, "clicked",
                    G_CALLBACK (_column_clicked_cb), self);

  priv->n_elements_col = col;

  return treeview;
}

static void
_column_clicked_cb (GtkTreeViewColumn *col, gpointer data)
{
  FrogrAddToAlbumDialog *self = NULL;
  FrogrAddToAlbumDialogPrivate *priv = NULL;
  GtkTreeModel *model = NULL;

  self = FROGR_ADD_TO_ALBUM_DIALOG (data);
  priv = FROGR_ADD_TO_ALBUM_DIALOG_GET_PRIVATE (self);

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->treeview));
  if (!GTK_IS_TREE_SORTABLE (model))
    return;

  if (col == priv->checkbox_col)
    _toggle_column_sort_order (GTK_TREE_SORTABLE (model), col, CHECKBOX_COL);
  else if (col == priv->title_col)
    _toggle_column_sort_order (GTK_TREE_SORTABLE (model), col, TITLE_COL);
  else if (col == priv->n_elements_col)
    _toggle_column_sort_order (GTK_TREE_SORTABLE (model), col, N_ELEMENTS_COL);
  else
    g_assert_not_reached ();
}

static void
_toggle_column_sort_order (GtkTreeSortable *sortable,
                           GtkTreeViewColumn *col,
                           gint col_id)
{
  g_return_if_fail (GTK_IS_TREE_SORTABLE (sortable));
  g_return_if_fail (GTK_IS_TREE_VIEW_COLUMN (col));
  g_return_if_fail (col_id >= 0 && col_id < N_COLS);

  GtkSortType current;
  GtkSortType new;

  current = gtk_tree_view_column_get_sort_order (col);
  if (current == GTK_SORT_ASCENDING)
    new = GTK_SORT_DESCENDING;
  else
    new = GTK_SORT_ASCENDING;

  gtk_tree_view_column_set_sort_order (col, new);
  gtk_tree_sortable_set_sort_column_id (sortable, col_id, new);
}

static gint
_tree_iter_compare_n_elements_func (GtkTreeModel *model,
                                    GtkTreeIter *a,
                                    GtkTreeIter *b,
                                    gpointer data)
{
  g_return_val_if_fail (GTK_IS_TREE_MODEL (model), 0);

  gchar *a_str = NULL;
  gchar *b_str = NULL;
  gint a_value;
  gint b_value;

  gtk_tree_model_get (GTK_TREE_MODEL (model), a, N_ELEMENTS_COL, &a_str, -1);
  gtk_tree_model_get (GTK_TREE_MODEL (model), b, N_ELEMENTS_COL, &b_str, -1);

  a_value = g_ascii_strtoll (a_str, NULL, 10);
  b_value = g_ascii_strtoll (b_str, NULL, 10);

  g_free (a_str);
  g_free (b_str);

  return (a_value - b_value);
}

static void
_populate_treemodel_with_albums (FrogrAddToAlbumDialog *self)
{
  FrogrAddToAlbumDialogPrivate *priv = NULL;
  FrogrAlbum *album = NULL;
  GtkTreeIter iter;
  GSList *current = NULL;
  gchar *n_elements_str = NULL;

  priv = FROGR_ADD_TO_ALBUM_DIALOG_GET_PRIVATE (self);

  for (current = priv->albums; current; current = g_slist_next (current))
    {
      if (!FROGR_IS_ALBUM (current->data))
        continue;

      album = FROGR_ALBUM (current->data);
      n_elements_str = g_strdup_printf ("%d", frogr_album_get_n_photos (album));

      gtk_list_store_append (GTK_LIST_STORE (priv->treemodel), &iter);
      gtk_list_store_set (GTK_LIST_STORE (priv->treemodel), &iter,
                          CHECKBOX_COL, FALSE,
                          TITLE_COL, frogr_album_get_title (album),
                          N_ELEMENTS_COL, n_elements_str,
                          ALBUM_COL, album,
                          -1);
      g_free (n_elements_str);
    }
}

static void
_fill_dialog_with_data (FrogrAddToAlbumDialog *self)
{
  FrogrAddToAlbumDialogPrivate *priv = NULL;
  GtkTreeIter iter;
  gint n_albums;
  gint n_pictures;

  priv = FROGR_ADD_TO_ALBUM_DIALOG_GET_PRIVATE (self);
  n_albums = g_slist_length (priv->albums);
  n_pictures = g_slist_length (priv->pictures);

  /* No albums, nothing to do */
  if (n_albums == 0 || n_pictures == 0)
    return;

  /* Iterate over all the items */
  gtk_tree_model_get_iter_first (priv->treemodel, &iter);
  do
    {
      FrogrAlbum *album = NULL;
      GSList *p_item = NULL;
      gboolean do_check = TRUE;

      gtk_tree_model_get (GTK_TREE_MODEL (priv->treemodel), &iter,
                          ALBUM_COL, &album, -1);

      for (p_item = priv->pictures; p_item; p_item = g_slist_next (p_item))
        {
          FrogrPicture *picture = NULL;

          picture = FROGR_PICTURE (p_item->data);
          if (!frogr_picture_in_album (picture, album))
            {
              do_check = FALSE;
              break;
            }
        }

      gtk_list_store_set (GTK_LIST_STORE (priv->treemodel), &iter,
                          CHECKBOX_COL, do_check, -1);
    }
  while (gtk_tree_model_iter_next (priv->treemodel, &iter));
}

static void
_album_toggled_cb (GtkCellRendererToggle *celltoggle,
                   gchar *path_string,
                   GtkTreeView *treeview)
{
  GtkTreeModel *model = NULL;
  GtkTreePath *path;
  GtkTreeIter iter;
  gboolean active = FALSE;

  g_return_if_fail (GTK_IS_TREE_VIEW (treeview));

  model = gtk_tree_view_get_model (treeview);
  path = gtk_tree_path_new_from_string (path_string);

  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_path_free (path);

  gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
                      CHECKBOX_COL, &active, -1);

  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                      CHECKBOX_COL, !active, -1);
}

static GSList *
_get_selected_albums (FrogrAddToAlbumDialog *self)
{
  FrogrAddToAlbumDialogPrivate *priv = NULL;
  GtkTreeIter iter;
  gboolean selected = FALSE;
  FrogrAlbum *album = NULL;
  GSList *selected_albums = NULL;

  priv = FROGR_ADD_TO_ALBUM_DIALOG_GET_PRIVATE (self);

  /* No albums, nothing to do */
  if (g_slist_length (priv->albums) == 0)
    return NULL;

  /* Iterate over all the items */
  gtk_tree_model_get_iter_first (priv->treemodel, &iter);
  do
    {
      gtk_tree_model_get (GTK_TREE_MODEL (priv->treemodel), &iter,
                          CHECKBOX_COL, &selected, -1);
      if (!selected)
        continue;

      gtk_tree_model_get (GTK_TREE_MODEL (priv->treemodel), &iter,
                          ALBUM_COL, &album, -1);

      if (FROGR_IS_ALBUM (album))
        {
          selected_albums = g_slist_append (selected_albums, album);
          g_object_ref (album);
        }
    }
  while (gtk_tree_model_iter_next (priv->treemodel, &iter));

  return selected_albums;
}

static void
_update_pictures (FrogrAddToAlbumDialog *self)
{
  FrogrAddToAlbumDialogPrivate *priv = NULL;
  FrogrPicture *picture = NULL;
  GSList *selected_albums = NULL;
  GSList *item = NULL;

  priv = FROGR_ADD_TO_ALBUM_DIALOG_GET_PRIVATE (self);

  selected_albums = _get_selected_albums (self);
  for (item = priv->pictures; item; item = g_slist_next (item))
    {
      picture = FROGR_PICTURE (item->data);
      frogr_picture_set_albums (picture, selected_albums);
    }

  g_slist_foreach (selected_albums, (GFunc)g_object_unref, NULL);
  g_slist_free (selected_albums);
}

static void
_dialog_response_cb (GtkDialog *dialog, gint response, gpointer data)
{
  if (response == GTK_RESPONSE_OK)
    {
      FrogrAddToAlbumDialog *self = NULL;

      self = FROGR_ADD_TO_ALBUM_DIALOG (dialog);
      _update_pictures (self);
    }

  /* Destroy the dialog */
  gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
_frogr_add_to_album_dialog_set_property (GObject *object,
                                         guint prop_id,
                                         const GValue *value,
                                         GParamSpec *pspec)
{
  FrogrAddToAlbumDialogPrivate *priv = FROGR_ADD_TO_ALBUM_DIALOG_GET_PRIVATE (object);

  switch (prop_id)
    {
    case PROP_PICTURES:
      priv->pictures = (GSList *) g_value_get_pointer (value);
      break;
    case PROP_ALBUMS:
      priv->albums = (GSList *) g_value_get_pointer (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
_frogr_add_to_album_dialog_get_property (GObject *object,
                                         guint prop_id,
                                         GValue *value,
                                         GParamSpec *pspec)
{
  FrogrAddToAlbumDialogPrivate *priv = FROGR_ADD_TO_ALBUM_DIALOG_GET_PRIVATE (object);

  switch (prop_id)
    {
    case PROP_PICTURES:
      g_value_set_pointer (value, priv->pictures);
      break;
    case PROP_ALBUMS:
      g_value_set_pointer (value, priv->albums);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
_frogr_add_to_album_dialog_dispose (GObject *object)
{
  FrogrAddToAlbumDialogPrivate *priv = FROGR_ADD_TO_ALBUM_DIALOG_GET_PRIVATE (object);

  if (priv->pictures)
    {
      g_slist_foreach (priv->pictures, (GFunc)g_object_unref, NULL);
      g_slist_free (priv->pictures);
      priv->pictures = NULL;
    }

  if (priv->albums)
    priv->albums = NULL;

  if (priv->treemodel)
    {
      g_object_unref (priv->treemodel);
      priv->treemodel = NULL;
    }

  G_OBJECT_CLASS(frogr_add_to_album_dialog_parent_class)->dispose (object);
}

static void
frogr_add_to_album_dialog_class_init (FrogrAddToAlbumDialogClass *klass)
{
  GObjectClass *obj_class = (GObjectClass *)klass;
  GParamSpec *pspec;

  /* GObject signals */
  obj_class->set_property = _frogr_add_to_album_dialog_set_property;
  obj_class->get_property = _frogr_add_to_album_dialog_get_property;
  obj_class->dispose = _frogr_add_to_album_dialog_dispose;

  /* Install properties */
  pspec = g_param_spec_pointer ("pictures",
                                "pictures",
                                "List of pictures for "
                                "the 'add to album' dialog",
                                G_PARAM_READWRITE
                                | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (obj_class, PROP_PICTURES, pspec);

  pspec = g_param_spec_pointer ("albums",
                                "albums",
                                "List of albums currently available "
                                "for the 'add to album' dialog",
                                G_PARAM_READWRITE
                                | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (obj_class, PROP_ALBUMS, pspec);

  g_type_class_add_private (obj_class, sizeof (FrogrAddToAlbumDialogPrivate));
}

static void
frogr_add_to_album_dialog_init (FrogrAddToAlbumDialog *self)
{
  FrogrAddToAlbumDialogPrivate *priv = NULL;
  GtkWidget *vbox = NULL;
  GtkWidget *widget = NULL;

  priv = FROGR_ADD_TO_ALBUM_DIALOG_GET_PRIVATE (self);
  priv->pictures = NULL;
  priv->albums = NULL;

  /* Create widgets */
  gtk_dialog_add_buttons (GTK_DIALOG (self),
                          GTK_STOCK_OK,
                          GTK_RESPONSE_OK,
                          GTK_STOCK_CANCEL,
                          GTK_RESPONSE_CANCEL,
                          NULL);
  gtk_container_set_border_width (GTK_CONTAINER (self), 12);

#if GTK_CHECK_VERSION (2,14,0)
  vbox = gtk_dialog_get_content_area (GTK_DIALOG (self));
#else
  vbox = GTK_DIALOG (self)->vbox;
#endif

  widget = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (widget),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (widget),
                                       GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (vbox), widget, TRUE, TRUE, 6);

  priv->treeview = _create_tree_view (self);
  gtk_container_add (GTK_CONTAINER (widget), priv->treeview);

  priv->treemodel =
    GTK_TREE_MODEL (gtk_list_store_new (4, G_TYPE_BOOLEAN,
                                        G_TYPE_STRING, G_TYPE_STRING,
                                        G_TYPE_POINTER));
  gtk_tree_view_set_model (GTK_TREE_VIEW (priv->treeview), priv->treemodel);

  /* Sorting function for the number of elements column */
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (priv->treemodel),
                                   N_ELEMENTS_COL,
                                   _tree_iter_compare_n_elements_func,
                                   self, NULL);

  g_signal_connect (G_OBJECT (self), "response",
                    G_CALLBACK (_dialog_response_cb), NULL);

  gtk_window_set_default_size (GTK_WINDOW (self),
                               MINIMUM_WINDOW_WIDTH,
                               MINIMUM_WINDOW_HEIGHT);

  /* Show the UI */
  gtk_widget_show_all (GTK_WIDGET (self));
}

/* Public API */

void
frogr_add_to_album_dialog_show (GtkWindow *parent, GSList *pictures, GSList *albums)
{
  FrogrAddToAlbumDialog *self = NULL;
  GObject *new = NULL;

  new = g_object_new (FROGR_TYPE_ADD_TO_ALBUM_DIALOG,
                      "title", _("Add to Album"),
                      "modal", TRUE,
                      "pictures", pictures,
                      "albums", albums,
                      "transient-for", parent,
                      "resizable", TRUE,
                      NULL);

  self = FROGR_ADD_TO_ALBUM_DIALOG (new);

  _populate_treemodel_with_albums (self);
  _fill_dialog_with_data (self);

  gtk_widget_show_all (GTK_WIDGET (self));
}
