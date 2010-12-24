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
  GSList *pictures;
  GtkWidget *entry;
  gchar *tags;
} FrogrAddTagsDialogPrivate;

/* Properties */
enum  {
  PROP_0,
  PROP_PICTURES
};

/* Prototypes */

static void _dialog_response_cb (GtkDialog *dialog, gint response, gpointer data);

/* Private API */

static void _dialog_response_cb (GtkDialog *dialog, gint response, gpointer data)
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
  GtkWidget *vbox;

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

  priv->entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (vbox), priv->entry, TRUE, FALSE, 6);
  gtk_widget_set_size_request (GTK_WIDGET (self), 300, -1);

  /* Show the UI */
  gtk_widget_show_all (GTK_WIDGET (self));
}

/* Public API */

void
frogr_add_tags_dialog_show (GtkWindow *parent, GSList *pictures)
{
  GtkWidget *dialog = NULL;

  dialog = GTK_WIDGET (g_object_new (FROGR_TYPE_ADD_TAGS_DIALOG,
                                     "title", _("Add Tags"),
                                     "modal", TRUE,
                                     "pictures", pictures,
                                     "transient-for", parent,
                                     "resizable", FALSE,
                                     "has-separator", FALSE,
                                     NULL));

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (_dialog_response_cb), NULL);

  gtk_widget_show_all (dialog);
}
