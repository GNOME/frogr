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

#define GTKBUILDER_FILE                                 \
  APP_DATA_DIR "/gtkbuilder/frogr-details-dialog.xml"

#define PICTURE_WIDTH 150
#define PICTURE_HEIGHT 150

#define FROGR_DETAILS_DIALOG_GET_PRIVATE(object)                   \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),                          \
                                FROGR_TYPE_DETAILS_DIALOG,         \
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
  FrogrPicture *fpicture;
} FrogrDetailsDialogPrivate;

/* Properties */
enum  {
  PROP_0,
  PROP_PICTURE
};

/* Private API */

static void
_update_ui (FrogrDetailsDialog *fdetailsdialog)
{
  FrogrDetailsDialogPrivate *priv =
    FROGR_DETAILS_DIALOG_GET_PRIVATE (fdetailsdialog);
  gboolean active;

  /* Adjust sensitiveness for check buttons */
  active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv -> public_rb));
  gtk_widget_set_sensitive (priv -> friend_cb, !active);
  gtk_widget_set_sensitive (priv -> family_cb, !active);
}

static GdkPixbuf *
_get_scaled_pixbuf (const gchar *filepath)
{
  GdkPixbuf *pixbuf = NULL;
  GdkPixbuf *scaled_pixbuf = NULL;
  gint width;
  gint height;
  gint new_width;
  gint new_height;

  /* Build the image */
  pixbuf = gdk_pixbuf_new_from_file (filepath, NULL);

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
  /* Free */
  g_object_unref (pixbuf);

  return scaled_pixbuf;
}

static void
_fill_dialog_with_data (FrogrDetailsDialog *fdetailsdialog)
{
  FrogrDetailsDialogPrivate *priv =
    FROGR_DETAILS_DIALOG_GET_PRIVATE (fdetailsdialog);

  GdkPixbuf *pixbuf;
  gchar *filepath = NULL;
  gchar *title = NULL;
  gchar *description = NULL;
  gchar *tags = NULL;
  gboolean is_public;
  gboolean is_friend;
  gboolean is_family;

  /* Retrieve needed data */
  filepath = (gchar *)frogr_picture_get_filepath (priv -> fpicture);
  title = (gchar *)frogr_picture_get_title (priv -> fpicture);
  description = (gchar *)frogr_picture_get_description (priv -> fpicture);
  tags = (gchar *)frogr_picture_get_tags (priv -> fpicture);
  is_public = frogr_picture_is_public (priv -> fpicture);
  is_friend = frogr_picture_is_friend (priv -> fpicture);
  is_family = frogr_picture_is_family (priv -> fpicture);

  /* Fill in with data */
  if (title != NULL)
    gtk_entry_set_text (GTK_ENTRY (priv -> title_entry),title);

  if (description != NULL)
    gtk_text_buffer_set_text (GTK_TEXT_BUFFER (priv -> text_buffer),
                              description, -1);
  if (tags != NULL)
    gtk_entry_set_text (GTK_ENTRY (priv -> tags_entry), tags);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv -> public_rb),
                                is_public);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv -> friend_cb),
                                is_friend);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv -> family_cb),
                                is_family);

  /* Set resized pixbuf */
  pixbuf = _get_scaled_pixbuf (filepath);
  gtk_image_set_from_pixbuf (GTK_IMAGE (priv -> picture_img), pixbuf);
  g_object_unref (pixbuf);

  /* Update UI */
  _update_ui (fdetailsdialog);
}

static gboolean
_validate_dialog_data (FrogrDetailsDialog *fdetailsdialog)
{
  FrogrDetailsDialogPrivate *priv =
    FROGR_DETAILS_DIALOG_GET_PRIVATE (fdetailsdialog);
  gchar *title;

  /* Mandatory fields */
  title = (gchar *)gtk_entry_get_text (GTK_ENTRY (priv -> title_entry));

  if ((title == NULL) || g_str_equal (g_strstrip (title), ""))
    return FALSE;

  /* Validated if reached */
  return TRUE;
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
  title = (gchar *)gtk_entry_get_text (GTK_ENTRY (priv -> title_entry));
  tags = (gchar *)gtk_entry_get_text (GTK_ENTRY (priv -> tags_entry));

  gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (priv -> text_buffer),
                              &start, &end);
  description =
    (gchar *)gtk_text_buffer_get_text (GTK_TEXT_BUFFER (priv -> text_buffer),
                                       &start, &end, FALSE);
  is_public =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv -> public_rb));

  if (!is_public)
    {
      is_friend =
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv -> friend_cb));
      is_family =
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv -> family_cb));
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
      frogr_picture_set_title (priv -> fpicture, title ? title : "");
      frogr_picture_set_description (priv -> fpicture,
                                     description ? description : "");
      frogr_picture_set_tags (priv -> fpicture, tags ? tags : "");

      frogr_picture_set_public (priv -> fpicture, is_public);
      frogr_picture_set_friend (priv -> fpicture, is_friend);
      frogr_picture_set_family (priv -> fpicture, is_family);

      /* Everything went fine */
      result = TRUE;
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

  /* Return result */
  return result;
}

/* Event handlers */

void
_on_public_rbutton_toggled (GtkToggleButton *tbutton,
                            gpointer data)
{
  FrogrDetailsDialog *fdetailsdialog = FROGR_DETAILS_DIALOG (data);

  /* Just update the UI */
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
    case PROP_PICTURE:
      priv -> fpicture = FROGR_PICTURE (g_value_get_object (value));
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
    case PROP_PICTURE:
      g_value_set_object (value, priv -> fpicture);
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
  g_object_unref (priv -> fpicture);
  G_OBJECT_CLASS(frogr_details_dialog_parent_class) -> finalize (object);
}

static void
frogr_details_dialog_class_init (FrogrDetailsDialogClass *klass)
{
  GObjectClass *obj_class = (GObjectClass *)klass;

  /* GtkObject signals */
  obj_class->set_property = _frogr_details_dialog_set_property;
  obj_class->get_property = _frogr_details_dialog_get_property;
  obj_class -> finalize = _frogr_details_dialog_finalize;

  /* Install properties */
  g_object_class_install_property (obj_class,
                                   PROP_PICTURE,
                                   g_param_spec_object ("picture",
							"picture",
							"The FrogrPicture for "
                                                        "the details dialog",
							FROGR_TYPE_PICTURE,
							G_PARAM_READWRITE));

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
                       GTK_WIDGET (GTK_DIALOG (fdetailsdialog) -> vbox));

  gtk_dialog_add_buttons (GTK_DIALOG (fdetailsdialog),
                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                          GTK_STOCK_OK, GTK_RESPONSE_OK,
                          NULL);

  /* Get widgets */
  priv -> title_entry =
    GTK_WIDGET (gtk_builder_get_object (builder, "title_entry"));
  priv -> desc_tview =
    GTK_WIDGET (gtk_builder_get_object (builder, "description_tview"));
  priv -> tags_entry =
    GTK_WIDGET (gtk_builder_get_object (builder, "tags_entry"));
  priv -> text_buffer =
    gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv -> desc_tview));
  priv -> public_rb =
    GTK_WIDGET (gtk_builder_get_object (builder, "public_rbutton"));
  priv -> private_rb =
    GTK_WIDGET (gtk_builder_get_object (builder, "private_rbutton"));
  priv -> friend_cb =
    GTK_WIDGET (gtk_builder_get_object (builder, "friend_cbutton"));
  priv -> family_cb =
    GTK_WIDGET (gtk_builder_get_object (builder, "family_cbutton"));
  priv -> picture_img =
    GTK_WIDGET (gtk_builder_get_object (builder, "picture_img"));

  /* Connect signals */
  gtk_builder_connect_signals (builder, fdetailsdialog);

  /* Show the UI */
  gtk_widget_show_all (GTK_WIDGET(fdetailsdialog));

  /* Free */
  g_object_unref (G_OBJECT (builder));
}


/* Public API */

FrogrDetailsDialog *
frogr_details_dialog_new (GtkWindow *parent, FrogrPicture *fpicture)
{
  GObject *new = g_object_new (FROGR_TYPE_DETAILS_DIALOG,
                               "modal", TRUE,
                               "picture", fpicture,
                               "transient-for", parent,
                               "resizable", FALSE,
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
