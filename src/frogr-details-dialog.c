/*
 * frogr-details-dialog.c -- Picture details dialog
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

#include "frogr-details-dialog.h"

#include "frogr-controller.h"
#include "frogr-picture.h"
#include "frogr-util.h"

#include <config.h>
#include <glib/gi18n.h>

#define GTKBUILDER_FILE APP_DATA_DIR "/gtkbuilder/frogr-details-dialog.xml"
#define MPICTURES_IMAGE APP_DATA_DIR "/images/mpictures.png"

#define DIALOG_MIN_WIDTH 540
#define DIALOG_MIN_HEIGHT 420

#define PICTURE_WIDTH 150
#define PICTURE_HEIGHT 150

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
  GtkTextBuffer *text_buffer;
  GtkWidget *picture_img;
  GtkWidget *mpictures_label;
  GSList *pictures;
} FrogrDetailsDialogPrivate;

/* Properties */
enum  {
  PROP_0,
  PROP_PICTURES
};


/* Prototypes */

static void _update_ui (FrogrDetailsDialog *self);
static GdkPixbuf *_get_scaled_pixbuf (GdkPixbuf *pixbuf);
static void _fill_dialog_with_data (FrogrDetailsDialog *self);
static gboolean _validate_dialog_data (FrogrDetailsDialog *self);
static gboolean _save_data (FrogrDetailsDialog *self);

void _on_public_private_rbutton_toggled (GtkToggleButton *tbutton,
                                         gpointer data);
void _on_family_friend_cbutton_toggled (GtkToggleButton *tbutton,
                                        gpointer data);

static void _dialog_response_cb (GtkDialog *dialog, gint response, gpointer data);


/* Private API */

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

  /* Initial widget to grab focus */
  gtk_widget_grab_focus (priv->title_entry);
  gtk_editable_set_position (GTK_EDITABLE (priv->title_entry), -1);
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

  /* Take first element values */
  item = priv->pictures;
  picture = FROGR_PICTURE (item->data);

  title_val = (gchar *)frogr_picture_get_title (picture);
  desc_val = (gchar *)frogr_picture_get_description (picture);
  tags_val = (gchar *)frogr_picture_get_tags (picture);
  is_public_val = frogr_picture_is_public (picture);
  is_friend_val = frogr_picture_is_friend (picture);
  is_family_val = frogr_picture_is_family (picture);

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

  n_pictures = g_slist_length (priv->pictures);
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
      text = g_strdup_printf (_("(Editing %d pictures)"), n_pictures);
      gtk_label_set_text (GTK_LABEL (priv->mpictures_label), text);
      g_free (text);
    }
  else
    {
      /* Set pixbuf scaled to the right size */
      GdkPixbuf *pixbuf = frogr_picture_get_pixbuf (picture);
      GdkPixbuf *s_pixbuf = _get_scaled_pixbuf (pixbuf);
      gtk_image_set_from_pixbuf (GTK_IMAGE (priv->picture_img), s_pixbuf);
      g_object_unref (s_pixbuf);

      /* Hide multiple pictures label (unused) */
      gtk_widget_hide (priv->mpictures_label);
    }

  /* Update UI */
  _update_ui (self);
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

          frogr_picture_set_public (picture, is_public);
          frogr_picture_set_friend (picture, is_friend);
          frogr_picture_set_family (picture, is_family);

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

void
_on_public_private_rbutton_toggled (GtkToggleButton *tbutton,
                                    gpointer data)
{
  FrogrDetailsDialog *self = FROGR_DETAILS_DIALOG (data);
  FrogrDetailsDialogPrivate *priv = FROGR_DETAILS_DIALOG_GET_PRIVATE (data);

  /* Reset consistence and update UI */
  gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (priv->public_rb),
                                      FALSE);
  gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (priv->private_rb),
                                      FALSE);
  _update_ui (self);
}

void
_on_family_friend_cbutton_toggled (GtkToggleButton *tbutton,
                                   gpointer data)
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

  GtkBuilder *builder;
  GtkWidget *main_vbox;

  /* Create widgets */
  builder = gtk_builder_new ();
  gtk_builder_add_from_file (builder, GTKBUILDER_FILE, NULL);

  main_vbox = GTK_WIDGET (gtk_builder_get_object (builder, "main-vbox"));
#if GTK_CHECK_VERSION (2,14,0)
  gtk_widget_reparent (main_vbox,
                       GTK_WIDGET (gtk_dialog_get_content_area (GTK_DIALOG (self))));
#else
  gtk_widget_reparent (main_vbox,
                       GTK_WIDGET (GTK_DIALOG (self)->vbox));
#endif

  gtk_dialog_add_buttons (GTK_DIALOG (self),
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
  gtk_builder_connect_signals (builder, self);

  /* Show the UI */
  gtk_widget_show_all (GTK_WIDGET (self));
  gtk_dialog_set_default_response (GTK_DIALOG (self),
                                   GTK_RESPONSE_OK);
  /* Free */
  g_object_unref (G_OBJECT (builder));
}


/* Public API */

void
frogr_details_dialog_show (GtkWindow *parent, GSList *fpictures)
{
  GtkWidget *dialog = NULL;

  dialog = GTK_WIDGET (g_object_new (FROGR_TYPE_DETAILS_DIALOG,
                                     "modal", TRUE,
                                     "pictures", fpictures,
                                     "transient-for", parent,
                                     "width-request", DIALOG_MIN_WIDTH,
                                     "height-request", DIALOG_MIN_HEIGHT,
                                     "resizable", TRUE,
                                     "title", _("Edit picture details"),
                                     NULL));

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (_dialog_response_cb), NULL);

  /* Fill dialog's widgets with data */
  _fill_dialog_with_data (FROGR_DETAILS_DIALOG (dialog));

  gtk_widget_show_all (dialog);
}
