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
  N_PHOTOS_COL,
  ALBUM_COL
};


/* Prototypes */

static void _on_dialog_map_event (GtkWidget *widget, GdkEvent *event, gpointer user_data);

static GtkWidget *_create_tree_view (FrogrAddToAlbumDialog *self);

static void _populate_treemodel_with_albums (FrogrAddToAlbumDialog *self);

static void _album_toggled_cb (GtkCellRendererToggle *celltoggle, gchar *path_string,
                               GtkTreeView *treeview);

static void _dialog_response_cb (GtkDialog *dialog, gint response, gpointer data);

/* Private API */

static void
_on_dialog_map_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  FrogrAddToAlbumDialog *self = NULL;
  self = FROGR_ADD_TO_ALBUM_DIALOG (widget);
  _populate_treemodel_with_albums (self);
}

static GtkWidget *
_create_tree_view (FrogrAddToAlbumDialog *self)
{
  GtkWidget *treeview = NULL;
  GtkTreeViewColumn *col = NULL;
  GtkCellRenderer *rend = NULL;

  treeview = gtk_tree_view_new();

  /* Checkbox */
  rend = gtk_cell_renderer_toggle_new ();
  col = gtk_tree_view_column_new_with_attributes (_("Select"),
                                                  rend,
                                                  "active", CHECKBOX_COL,
                                                  NULL);
  gtk_tree_view_column_set_clickable (col, TRUE);
  g_signal_connect (rend, "toggled",
                    G_CALLBACK (_album_toggled_cb), treeview);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), col);

  /* Title */
  rend = gtk_cell_renderer_text_new ();
  col = gtk_tree_view_column_new_with_attributes (_("Album's title"),
                                                  rend,
                                                  "text", TITLE_COL,
                                                  NULL);
  gtk_tree_view_column_set_clickable (col, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), col);

  /* Description */
  rend = gtk_cell_renderer_text_new ();
  col = gtk_tree_view_column_new_with_attributes (_("Photos"),
                                                  rend,
                                                  "text", N_PHOTOS_COL,
                                                  NULL);
  gtk_tree_view_column_set_clickable (col, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), col);

  return treeview;
}

static void
_populate_treemodel_with_albums (FrogrAddToAlbumDialog *self)
{
  FrogrAddToAlbumDialogPrivate *priv = NULL;
  FrogrAlbum *album = NULL;
  GtkTreeIter iter;
  GSList *current = NULL;
  gchar *n_photos_str = NULL;

  priv = FROGR_ADD_TO_ALBUM_DIALOG_GET_PRIVATE (self);

  for (current = priv->albums; current; current = g_slist_next (current))
    {
      album = FROGR_ALBUM (current->data);
      n_photos_str = g_strdup_printf ("%d", frogr_album_get_n_photos (album));

      gtk_list_store_append (GTK_LIST_STORE (priv->treemodel), &iter);
      gtk_list_store_set (GTK_LIST_STORE (priv->treemodel), &iter,
                          CHECKBOX_COL, FALSE,
                          TITLE_COL, frogr_album_get_title (album),
                          N_PHOTOS_COL, n_photos_str,
                          ALBUM_COL, g_object_ref (album),
                          -1);
      g_free (n_photos_str);
    }
}

static void
_album_toggled_cb (GtkCellRendererToggle *celltoggle, gchar *path_string,
                   GtkTreeView *treeview)
{
  GtkTreeModel *model = NULL;
  GtkTreeModelSort *sort_model = NULL;
  GtkTreePath *path;
  GtkTreeIter iter;
  gboolean active = FALSE;

  g_return_if_fail (GTK_IS_TREE_VIEW (treeview));

  model = gtk_tree_view_get_model (treeview);
  if (model == NULL)
    return;

  path = gtk_tree_path_new_from_string (path_string);
  if (!gtk_tree_model_get_iter (model,
                                &iter, path))
    {
      g_warning ("%s: bad path?", G_STRLOC);
      return;
    }
  gtk_tree_path_free (path);

  gtk_tree_model_get (GTK_TREE_MODEL (model),
                      &iter,
                      CHECKBOX_COL,
                      &active,
                      -1);

  gtk_list_store_set (GTK_LIST_STORE (model),
                      &iter,
                      CHECKBOX_COL,
                      !active,
                      -1);

  g_print ("toggled!");
}

static void _dialog_response_cb (GtkDialog *dialog, gint response, gpointer data)
{
  if (response == GTK_RESPONSE_OK)
    {
      FrogrAddToAlbumDialog *self = NULL;
      FrogrAddToAlbumDialogPrivate *priv = NULL;

      self = FROGR_ADD_TO_ALBUM_DIALOG (dialog);
      priv = FROGR_ADD_TO_ALBUM_DIALOG_GET_PRIVATE (self);

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
    priv->pictures = NULL;

  if (priv->treemodel)
    {
      g_object_unref (priv->treemodel);
      priv->treemodel = NULL;
    }

  G_OBJECT_CLASS(frogr_add_to_album_dialog_parent_class)->dispose (object);
}

static void
_frogr_add_to_album_dialog_finalize (GObject *object)
{
  FrogrAddToAlbumDialogPrivate *priv = FROGR_ADD_TO_ALBUM_DIALOG_GET_PRIVATE (object);

  G_OBJECT_CLASS(frogr_add_to_album_dialog_parent_class)->finalize (object);
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
  obj_class->finalize = _frogr_add_to_album_dialog_finalize;

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
  GtkTreeViewColumn *col = NULL;
  GtkCellRenderer *rend = NULL;

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

  g_signal_connect (G_OBJECT (self), "map-event",
                    G_CALLBACK (_on_dialog_map_event), NULL);

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
  GtkWidget *dialog = NULL;

  dialog = GTK_WIDGET (g_object_new (FROGR_TYPE_ADD_TO_ALBUM_DIALOG,
                                     "title", _("Add to album"),
                                     "modal", TRUE,
                                     "pictures", pictures,
                                     "albums", albums,
                                     "transient-for", parent,
                                     "resizable", TRUE,
                                     "has-separator", FALSE,
                                     NULL));

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (_dialog_response_cb), NULL);

  gtk_widget_show_all (dialog);
}
