/*
 * frogr-create-new-album-dialog.c -- 'Create new album' dialog
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

#include "frogr-create-new-album-dialog.h"

#include "frogr-album.h"
#include "frogr-controller.h"
#include "frogr-main-view-model.h"
#include "frogr-main-view.h"
#include "frogr-picture.h"
#include "frogr-util.h"

#include <config.h>
#include <glib/gi18n.h>

#define MINIMUM_WINDOW_WIDTH 450
#define MINIMUM_WINDOW_HEIGHT 280

#define FROGR_CREATE_NEW_ALBUM_DIALOG_GET_PRIVATE(object)               \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),                               \
                                FROGR_TYPE_CREATE_NEW_ALBUM_DIALOG,     \
                                FrogrCreateNewAlbumDialogPrivate))

G_DEFINE_TYPE (FrogrCreateNewAlbumDialog, frogr_create_new_album_dialog, GTK_TYPE_DIALOG);

typedef struct _FrogrCreateNewAlbumDialogPrivate {
  GtkWidget *title_entry;
  GtkWidget *description_tv;
  GtkTextBuffer *description_buffer;

  GSList *pictures;
  GSList *albums;
} FrogrCreateNewAlbumDialogPrivate;

/* Properties */
enum  {
  PROP_0,
  PROP_PICTURES,
  PROP_ALBUMS
};


/* Prototypes */

static gboolean _validate_dialog_data (FrogrCreateNewAlbumDialog *self);

static gboolean _save_data (FrogrCreateNewAlbumDialog *self);

static gboolean _update_model (FrogrCreateNewAlbumDialog *self,
                               const gchar *title,
                               const gchar *description);

static void _dialog_response_cb (GtkDialog *dialog, gint response, gpointer data);

/* Private API */

static gboolean
_validate_dialog_data (FrogrCreateNewAlbumDialog *self)
{
  FrogrCreateNewAlbumDialogPrivate *priv = NULL;
  gchar *title = NULL;
  gboolean result = TRUE;

  priv = FROGR_CREATE_NEW_ALBUM_DIALOG_GET_PRIVATE (self);

  /* Validate album's title */
  title = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->title_entry)));
  if ((title == NULL) || g_str_equal (g_strstrip (title), ""))
    result = FALSE;
  g_free (title);

  return result;
}

static gboolean
_save_data (FrogrCreateNewAlbumDialog *self)
{
  FrogrCreateNewAlbumDialogPrivate *priv = NULL;
  GtkTextIter start;
  GtkTextIter end;
  gchar *title = NULL;
  gchar *description = NULL;
  gboolean result = FALSE;

  priv = FROGR_CREATE_NEW_ALBUM_DIALOG_GET_PRIVATE (self);

  /* Save data */
  title = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->title_entry)));
  title = g_strstrip (title);

  gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (priv->description_buffer),
                              &start, &end);
  description = gtk_text_buffer_get_text (GTK_TEXT_BUFFER (priv->description_buffer),
                                          &start, &end, FALSE);
  description = g_strstrip (description);

  /* validate dialog */
  if (_validate_dialog_data (self))
    result = _update_model (self, title, description);
  else
    frogr_util_show_error_dialog (GTK_WINDOW (self), _("Missing data required"));

  /* free */
  g_free (title);
  g_free (description);

  /* Return result */
  return result;
}

static gboolean
_update_model (FrogrCreateNewAlbumDialog *self,
               const gchar *title,
               const gchar *description)
{
  FrogrCreateNewAlbumDialogPrivate *priv = NULL;
  FrogrController *controller = NULL;
  FrogrMainView *mainview = NULL;
  FrogrMainViewModel *mainview_model = NULL;
  FrogrAlbum *new_album = NULL;
  FrogrPicture *picture = NULL;
  GSList *item = NULL;
  gboolean result = FALSE;

  priv = FROGR_CREATE_NEW_ALBUM_DIALOG_GET_PRIVATE (self);
  controller = frogr_controller_get_instance ();
  mainview = frogr_controller_get_main_view (controller);
  mainview_model = frogr_main_view_get_model (mainview);

  /* Add the album to the model */
  new_album = frogr_album_new (title, description);
  frogr_main_view_model_add_album (mainview_model, new_album);

  /* Add the album to the list of albums for each picture */
  for (item = priv->pictures; item; item = g_slist_next (item))
    {
      picture = FROGR_PICTURE (item->data);
      frogr_picture_add_album (picture, new_album);
      result = TRUE;
    }

  return result;
}

static void
_dialog_response_cb (GtkDialog *dialog, gint response, gpointer data)
{
  FrogrCreateNewAlbumDialog *self = NULL;

  /* Try to save data if response is OK */
  self = FROGR_CREATE_NEW_ALBUM_DIALOG (dialog);
  if (response == GTK_RESPONSE_OK && _save_data (self) == FALSE)
      return;

  /* Destroy the dialog */
  gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
_frogr_create_new_album_dialog_set_property (GObject *object,
                                             guint prop_id,
                                             const GValue *value,
                                             GParamSpec *pspec)
{
  FrogrCreateNewAlbumDialogPrivate *priv = FROGR_CREATE_NEW_ALBUM_DIALOG_GET_PRIVATE (object);

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
_frogr_create_new_album_dialog_get_property (GObject *object,
                                             guint prop_id,
                                             GValue *value,
                                             GParamSpec *pspec)
{
  FrogrCreateNewAlbumDialogPrivate *priv = FROGR_CREATE_NEW_ALBUM_DIALOG_GET_PRIVATE (object);

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
_frogr_create_new_album_dialog_dispose (GObject *object)
{
  FrogrCreateNewAlbumDialogPrivate *priv = FROGR_CREATE_NEW_ALBUM_DIALOG_GET_PRIVATE (object);

  if (priv->pictures)
    {
      g_slist_foreach (priv->pictures, (GFunc)g_object_unref, NULL);
      g_slist_free (priv->pictures);
      priv->pictures = NULL;
    }

  if (priv->albums)
    priv->albums = NULL;

  G_OBJECT_CLASS(frogr_create_new_album_dialog_parent_class)->dispose (object);
}

static void
frogr_create_new_album_dialog_class_init (FrogrCreateNewAlbumDialogClass *klass)
{
  GObjectClass *obj_class = (GObjectClass *)klass;
  GParamSpec *pspec;

  /* GObject signals */
  obj_class->set_property = _frogr_create_new_album_dialog_set_property;
  obj_class->get_property = _frogr_create_new_album_dialog_get_property;
  obj_class->dispose = _frogr_create_new_album_dialog_dispose;

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

  g_type_class_add_private (obj_class, sizeof (FrogrCreateNewAlbumDialogPrivate));
}

static void
frogr_create_new_album_dialog_init (FrogrCreateNewAlbumDialog *self)
{
  FrogrCreateNewAlbumDialogPrivate *priv = NULL;
  GtkWidget *vbox = NULL;
  GtkWidget *table = NULL;
  GtkWidget *align = NULL;
  GtkWidget *scroller = NULL;
  GtkWidget *widget = NULL;
  GdkGeometry hints;

  priv = FROGR_CREATE_NEW_ALBUM_DIALOG_GET_PRIVATE (self);
  priv->pictures = NULL;
  priv->albums = NULL;

  /* Create widgets */
  gtk_dialog_add_buttons (GTK_DIALOG (self),
                          GTK_STOCK_OK,
                          GTK_RESPONSE_OK,
                          GTK_STOCK_CANCEL,
                          GTK_RESPONSE_CANCEL,
                          NULL);
  gtk_container_set_border_width (GTK_CONTAINER (self), 6);

#if GTK_CHECK_VERSION (2,14,0)
  vbox = gtk_dialog_get_content_area (GTK_DIALOG (self));
#else
  vbox = GTK_DIALOG (self)->vbox;
#endif

  table = gtk_table_new (2, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 6);

  widget = gtk_label_new (_("Album's title:"));
  align = gtk_alignment_new (1, 0, 1, 0);
  gtk_container_add (GTK_CONTAINER (align), widget);
  gtk_table_attach (GTK_TABLE (table), align, 0, 1, 0, 1,
                    0, 0, 6, 6);

  widget = gtk_entry_new ();
  align = gtk_alignment_new (1, 0, 1, 0);
  gtk_container_add (GTK_CONTAINER (align), widget);
  gtk_table_attach (GTK_TABLE (table), align, 1, 2, 0, 1,
                    GTK_EXPAND | GTK_FILL, 0, 6, 6);
  priv->title_entry = widget;

  widget = gtk_label_new (_("Album's description:"));
  align = gtk_alignment_new (1, 0, 1, 0);
  gtk_container_add (GTK_CONTAINER (align), widget);
  gtk_table_attach (GTK_TABLE (table), align, 0, 1, 1, 2,
                    0, GTK_EXPAND | GTK_FILL, 6, 6);

  widget = gtk_text_view_new ();
  scroller = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroller),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroller),
                                       GTK_SHADOW_ETCHED_IN);
  gtk_container_add (GTK_CONTAINER (scroller), widget);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (widget), GTK_WRAP_WORD);
  align = gtk_alignment_new (1, 0, 1, 1);
  gtk_container_add (GTK_CONTAINER (align), scroller);
  gtk_table_attach (GTK_TABLE (table), align, 1, 2, 1, 2,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 6, 6);

  priv->description_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
  priv->description_tv = widget;

  /* Set minimum size */
  hints.min_width = MINIMUM_WINDOW_WIDTH;
  hints.min_height = MINIMUM_WINDOW_HEIGHT;
  gtk_window_set_geometry_hints (GTK_WINDOW (self), NULL,
                                 &hints, GDK_HINT_MIN_SIZE);
  /* Show the UI */
  gtk_widget_show_all (GTK_WIDGET (self));
}

/* Public API */

void
frogr_create_new_album_dialog_show (GtkWindow *parent, GSList *pictures, GSList *albums)
{
  GtkWidget *dialog = NULL;
  dialog = GTK_WIDGET (g_object_new (FROGR_TYPE_CREATE_NEW_ALBUM_DIALOG,
                                     "title", _("Create new Album"),
                                     "modal", TRUE,
                                     "pictures", pictures,
                                     "albums", albums,
                                     "transient-for", parent,
                                     "resizable", FALSE,
                                     "has-separator", FALSE,
                                     NULL));

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (_dialog_response_cb), NULL);

  gtk_widget_show_all (dialog);
}
