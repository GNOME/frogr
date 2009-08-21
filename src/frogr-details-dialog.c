/*
 * frogr-details-dialog.c -- Picture details dialog
 *
 * Copyright (C) 2009 Mario Sanchez Prada
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

#include <config.h>
#include "frogr-details-dialog.h"
#include "frogr-controller.h"
#include "frogr-picture.h"

#define GTKBUILDER_FILE APP_DATA_DIR "/gtkbuilder/frogr-details-dialog.xml"
#define MPICTURES_IMAGE APP_DATA_DIR "/images/mpictures.png"

#define PICTURE_WIDTH 150
#define PICTURE_HEIGHT 150

#define FROGR_DETAILS_DIALOG_GET_PRIVATE(object)            \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),                   \
                                FROGR_TYPE_DETAILS_DIALOG,  \
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
  GtkTextBuffer *text_buffer;
  GtkWidget *picture_img;
  GtkWidget *mpictures_label;
  GSList *fpictures;
} FrogrDetailsDialogPrivate;

/* Properties */
enum  {
  PROP_0,
  PROP_PICTURES
};


/* Prototypes */

static void _update_ui (FrogrDetailsDialog *fdetailsdialog);
static GdkPixbuf *_get_scaled_pixbuf (GdkPixbuf *pixbuf);
static void _fill_dialog_with_data (FrogrDetailsDialog *fdetailsdialog);
static gboolean _validate_dialog_data (FrogrDetailsDialog *fdetailsdialog);
static gboolean _save_data (FrogrDetailsDialog *fdetailsdialog);

void _on_public_private_rbutton_toggled (GtkToggleButton *tbutton,
                                         gpointer data);
void _on_family_friend_cbutton_toggled (GtkToggleButton *tbutton,
                                        gpointer data);

/* Private API */

static void
_update_ui (FrogrDetailsDialog *fdetailsdialog)
{
  FrogrDetailsDialogPrivate *priv =
    FROGR_DETAILS_DIALOG_GET_PRIVATE (fdetailsdialog);
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
_fill_dialog_with_data (FrogrDetailsDialog *fdetailsdialog)
{
  FrogrDetailsDialogPrivate *priv =
    FROGR_DETAILS_DIALOG_GET_PRIVATE (fdetailsdialog);

  FrogrPicture *fpicture;
  GSList *item;
  guint n_pictures;
  gchar *title_val = NULL;
  gchar *desc_val = NULL;
  gchar *tags_val = NULL;
  gboolean is_public_val = FALSE;
  gboolean is_friend_val = FALSE;
  gboolean is_family_val = FALSE;

  /* Take first element values */
  item = priv->fpictures;
  fpicture = FROGR_PICTURE (item->data);

  title_val = (gchar *)frogr_picture_get_title (fpicture);
  desc_val = (gchar *)frogr_picture_get_description (fpicture);
  tags_val = (gchar *)frogr_picture_get_tags (fpicture);
  is_public_val = frogr_picture_is_public (fpicture);
  is_friend_val = frogr_picture_is_friend (fpicture);
  is_family_val = frogr_picture_is_family (fpicture);

  /* Iterate over the rest of elements */
  for (item = g_slist_next (item); item; item = g_slist_next (item))
    {
      gchar *title = NULL;
      gchar *desc = NULL;
      gchar *tags = NULL;
      gboolean is_public = FALSE;
      gboolean is_friend = FALSE;
      gboolean is_family = FALSE;

      /* Retrieve needed data */
      fpicture = FROGR_PICTURE (item->data);

      /* Only retrieve title and desc when needed */
      if (title_val != NULL)
        title = (gchar *)frogr_picture_get_title (fpicture);
      if (desc_val != NULL)
        desc = (gchar *)frogr_picture_get_description (fpicture);
      if (tags_val != NULL)
        tags = (gchar *)frogr_picture_get_tags (fpicture);

      /* boolean properties */
      is_public = frogr_picture_is_public (fpicture);
      is_friend = frogr_picture_is_friend (fpicture);
      is_family = frogr_picture_is_family (fpicture);

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
          gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (priv->public_rb),
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

      /* Update merged value */
      is_public_val &= is_public;
      is_friend_val &= is_friend;
      is_family_val &= is_family;
    }

  /* Fill in with data */
  if (title_val != NULL)
    gtk_entry_set_text (GTK_ENTRY (priv->title_entry), title_val);

  if (desc_val != NULL)
    gtk_text_buffer_set_text (GTK_TEXT_BUFFER (priv->text_buffer),
                              desc_val, -1);
  if (tags_val != NULL)
    gtk_entry_set_text (GTK_ENTRY (priv->tags_entry), tags_val);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->public_rb),
                                is_public_val);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->private_rb),
                                !is_public_val);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->friend_cb),
                                is_friend_val);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->family_cb),
                                is_family_val);

  n_pictures = g_slist_length (priv->fpictures);
  if (n_pictures > 1)
    {
      GdkPixbuf *pixbuf;
      gchar *text;

      /* Set the image for editing multiple pictures */
      pixbuf = gdk_pixbuf_new_from_file (MPICTURES_IMAGE, NULL);
      gtk_image_set_from_pixbuf (GTK_IMAGE (priv->picture_img), pixbuf);
      g_object_unref (pixbuf);

      /* Show the hidden label */
      gtk_widget_show (priv->mpictures_label);
      text = g_strdup_printf ("(Editing %d pictures)", n_pictures);
      gtk_label_set_text (GTK_LABEL (priv->mpictures_label), text);
      g_free (text);
    }
  else
    {
      /* Set pixbuf scaled to the right size */
      GdkPixbuf *pixbuf = frogr_picture_get_pixbuf (fpicture);
      GdkPixbuf *s_pixbuf = _get_scaled_pixbuf (pixbuf);
      gtk_image_set_from_pixbuf (GTK_IMAGE (priv->picture_img), s_pixbuf);
      g_object_unref (s_pixbuf);

      /* Hide multiple pictures label (unused) */
      gtk_widget_hide (priv->mpictures_label);
    }

  /* Update UI */
  _update_ui (fdetailsdialog);
}

static gboolean
_validate_dialog_data (FrogrDetailsDialog *fdetailsdialog)
{
  FrogrDetailsDialogPrivate *priv =
    FROGR_DETAILS_DIALOG_GET_PRIVATE (fdetailsdialog);

  gboolean result = TRUE;

  /* Mandatory fields (only if editing a single picture) */
  if (g_slist_length (priv->fpictures) <= 1)
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
_save_data (FrogrDetailsDialog *fdetailsdialog)
{
  FrogrDetailsDialogPrivate *priv =
    FROGR_DETAILS_DIALOG_GET_PRIVATE (fdetailsdialog);

  GtkTextIter start;
  GtkTextIter end;
  gchar *title = NULL;
  gchar *description = NULL;
  gchar *tags = NULL;
  gboolean is_public;
  gboolean is_friend;
  gboolean is_family;
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

  /* validate dialog */
  if (_validate_dialog_data (fdetailsdialog))
    {
      FrogrPicture *fpicture;
      GSList *item;
      guint n_pictures;

      /* Iterate over the rest of elements */
      n_pictures = g_slist_length (priv->fpictures);
      for (item = priv->fpictures; item; item = g_slist_next (item))
        {
          fpicture = FROGR_PICTURE (item->data);

          if (!g_str_equal (title, "") || (n_pictures <= 1))
            frogr_picture_set_title (fpicture, title);
          if (!g_str_equal (description, "") || (n_pictures <= 1))
            frogr_picture_set_description (fpicture, description);
          if (!g_str_equal (tags, "") || (n_pictures <= 1))
            frogr_picture_set_tags (fpicture, tags);

          frogr_picture_set_public (fpicture, is_public);
          frogr_picture_set_friend (fpicture, is_friend);
          frogr_picture_set_family (fpicture, is_family);

          /* Everything went fine */
          result = TRUE;
        }
    }
  else
    {
      /* Show alert */
      GtkWindow *parent_win =
        gtk_window_get_transient_for (GTK_WINDOW (fdetailsdialog));

      GtkWidget *dialog =
        gtk_message_dialog_new (parent_win,
                                GTK_DIALOG_MODAL,
                                GTK_MESSAGE_WARNING,
                                GTK_BUTTONS_CLOSE,
                                "Missing data required");
      /* Run alert dialog */
      gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);
    }

  /* free */
  g_free (title);
  g_free (description);
  g_free (tags);

  /* Return result */
  return result;
}

/* Event handlers */

void
_on_public_private_rbutton_toggled (GtkToggleButton *tbutton,
                                    gpointer data)
{
  FrogrDetailsDialog *fdetailsdialog = FROGR_DETAILS_DIALOG (data);
  FrogrDetailsDialogPrivate *priv = FROGR_DETAILS_DIALOG_GET_PRIVATE (data);

  /* Reset consistence and update UI */
  gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (priv->public_rb),
                                      FALSE);
  gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (priv->private_rb),
                                      FALSE);
  _update_ui (fdetailsdialog);
}

void
_on_family_friend_cbutton_toggled (GtkToggleButton *tbutton,
                                   gpointer data)
{
  FrogrDetailsDialog *fdetailsdialog = FROGR_DETAILS_DIALOG (data);

  /* Reset consistence and update UI */
  gtk_toggle_button_set_inconsistent (tbutton, FALSE);
  _update_ui (fdetailsdialog);
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
      priv->fpictures = (GSList *) g_value_get_pointer (value);
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
      g_value_set_pointer (value, priv->fpictures);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
_frogr_details_dialog_finalize (GObject *object)
{
  FrogrDetailsDialogPrivate *priv = FROGR_DETAILS_DIALOG_GET_PRIVATE (object);
  g_slist_foreach (priv->fpictures, (GFunc)g_object_unref, NULL);
  g_slist_free (priv->fpictures);
  G_OBJECT_CLASS(frogr_details_dialog_parent_class)->finalize (object);
}

static void
frogr_details_dialog_class_init (FrogrDetailsDialogClass *klass)
{
  GObjectClass *obj_class = (GObjectClass *)klass;
  GParamSpec *pspec;

  /* GtkObject signals */
  obj_class->set_property = _frogr_details_dialog_set_property;
  obj_class->get_property = _frogr_details_dialog_get_property;
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
frogr_details_dialog_init (FrogrDetailsDialog *fdetailsdialog)
{
  FrogrDetailsDialogPrivate *priv =
    FROGR_DETAILS_DIALOG_GET_PRIVATE (fdetailsdialog);

  GtkBuilder *builder;
  GtkWidget *main_vbox;

  /* Create widgets */
  builder = gtk_builder_new ();
  gtk_builder_add_from_file (builder, GTKBUILDER_FILE, NULL);

  main_vbox = GTK_WIDGET (gtk_builder_get_object (builder, "main-vbox"));
  gtk_widget_reparent (main_vbox,
                       GTK_WIDGET (GTK_DIALOG (fdetailsdialog)->vbox));

  gtk_dialog_add_buttons (GTK_DIALOG (fdetailsdialog),
                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                          GTK_STOCK_OK, GTK_RESPONSE_OK,
                          NULL);

  /* Get widgets */
  priv->title_entry =
    GTK_WIDGET (gtk_builder_get_object (builder, "title_entry"));
  priv->desc_tview =
    GTK_WIDGET (gtk_builder_get_object (builder, "description_tview"));
  priv->tags_entry =
    GTK_WIDGET (gtk_builder_get_object (builder, "tags_entry"));
  priv->text_buffer =
    gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->desc_tview));
  priv->public_rb =
    GTK_WIDGET (gtk_builder_get_object (builder, "public_rbutton"));
  priv->private_rb =
    GTK_WIDGET (gtk_builder_get_object (builder, "private_rbutton"));
  priv->friend_cb =
    GTK_WIDGET (gtk_builder_get_object (builder, "friend_cbutton"));
  priv->family_cb =
    GTK_WIDGET (gtk_builder_get_object (builder, "family_cbutton"));
  priv->picture_img =
    GTK_WIDGET (gtk_builder_get_object (builder, "picture_img"));
  priv->mpictures_label =
    GTK_WIDGET (gtk_builder_get_object (builder, "mpictures_label"));

  /* Connect signals */
  gtk_builder_connect_signals (builder, fdetailsdialog);

  /* Show the UI */
  gtk_widget_show_all (GTK_WIDGET (fdetailsdialog));

  /* Free */
  g_object_unref (G_OBJECT (builder));
}


/* Public API */

FrogrDetailsDialog *
frogr_details_dialog_new (GtkWindow *parent, GSList *fpictures)
{
  GObject *new = g_object_new (FROGR_TYPE_DETAILS_DIALOG,
                               "modal", TRUE,
                               "pictures", fpictures,
                               "transient-for", parent,
                               "resizable", FALSE,
                               "title", "Edit picture details",
                               NULL);
  return FROGR_DETAILS_DIALOG (new);
}

void
frogr_details_dialog_show (FrogrDetailsDialog *fdetailsdialog)
{
  FrogrDetailsDialogPrivate *priv =
    FROGR_DETAILS_DIALOG_GET_PRIVATE (fdetailsdialog);
  gint response;
  gboolean saved;

  /* Fill dialog's widgets with data */
  _fill_dialog_with_data (fdetailsdialog);

  /* Run the dialog */
  do
    {
      response = gtk_dialog_run (GTK_DIALOG (fdetailsdialog));

      /* Try to save data if response is OK */
      if (response == GTK_RESPONSE_OK)
        saved =_save_data (fdetailsdialog);

    } while ((response == GTK_RESPONSE_OK) && !saved);

  /* Destroy the dialog */
  gtk_widget_destroy (GTK_WIDGET (fdetailsdialog));
}
