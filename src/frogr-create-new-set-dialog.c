/*
 * frogr-create-new-set-dialog.c -- 'Create new set' dialog
 *
 * Copyright (C) 2010-2015 Mario Sanchez Prada
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

#include "frogr-create-new-set-dialog.h"

#include "frogr-photoset.h"
#include "frogr-controller.h"
#include "frogr-model.h"
#include "frogr-picture.h"
#include "frogr-util.h"

#include <config.h>
#include <glib/gi18n.h>

#define FROGR_CREATE_NEW_SET_DIALOG_GET_PRIVATE(object)                 \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),                               \
                                FROGR_TYPE_CREATE_NEW_SET_DIALOG,       \
                                FrogrCreateNewSetDialogPrivate))

G_DEFINE_TYPE (FrogrCreateNewSetDialog, frogr_create_new_set_dialog, GTK_TYPE_DIALOG)

typedef struct _FrogrCreateNewSetDialogPrivate {
  GtkWidget *title_entry;
  GtkWidget *description_tv;
  GtkWidget *copy_to_pictures_cb;
  GtkTextBuffer *description_buffer;

  GSList *pictures;
  GSList *photosets;
  gboolean copy_to_pictures;
} FrogrCreateNewSetDialogPrivate;

/* Properties */
enum  {
  PROP_0,
  PROP_PICTURES,
  PROP_PHOTOSETS
};


/* Prototypes */

static void _set_pictures (FrogrCreateNewSetDialog *self, const GSList *pictures);

static void _set_photosets (FrogrCreateNewSetDialog *self, const GSList *photosets);

static void _on_button_toggled (GtkToggleButton *button, gpointer data);

static gboolean _validate_dialog_data (FrogrCreateNewSetDialog *self);

static gboolean _save_data (FrogrCreateNewSetDialog *self);

static void _update_model (FrogrCreateNewSetDialog *self,
                           const gchar *title,
                           const gchar *description);

static void _dialog_response_cb (GtkDialog *dialog, gint response, gpointer data);

/* Private API */

static void
_set_pictures (FrogrCreateNewSetDialog *self, const GSList *pictures)
{
  FrogrCreateNewSetDialogPrivate *priv = FROGR_CREATE_NEW_SET_DIALOG_GET_PRIVATE (self);
  priv->pictures = g_slist_copy ((GSList*) pictures);
  g_slist_foreach (priv->pictures, (GFunc)g_object_ref, NULL);
}

static void
_set_photosets (FrogrCreateNewSetDialog *self, const GSList *photosets)
{
  FrogrCreateNewSetDialogPrivate *priv = NULL;

  priv = FROGR_CREATE_NEW_SET_DIALOG_GET_PRIVATE (self);
  priv->photosets = g_slist_copy ((GSList*)photosets);
  g_slist_foreach (priv->photosets, (GFunc)g_object_ref, NULL);
}

static void
_on_button_toggled (GtkToggleButton *button, gpointer data)
{
  FrogrCreateNewSetDialog *self = NULL;
  FrogrCreateNewSetDialogPrivate *priv = NULL;
  gboolean active = FALSE;

  self = FROGR_CREATE_NEW_SET_DIALOG (data);
  priv = FROGR_CREATE_NEW_SET_DIALOG_GET_PRIVATE (self);
  active = gtk_toggle_button_get_active (button);

  if (GTK_WIDGET (button) == priv->copy_to_pictures_cb)
    priv->copy_to_pictures = active;
}

static gboolean
_validate_dialog_data (FrogrCreateNewSetDialog *self)
{
  FrogrCreateNewSetDialogPrivate *priv = NULL;
  gchar *title = NULL;
  gboolean result = TRUE;

  priv = FROGR_CREATE_NEW_SET_DIALOG_GET_PRIVATE (self);

  /* Validate set's title */
  title = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->title_entry)));
  if ((title == NULL) || g_str_equal (g_strstrip (title), ""))
    result = FALSE;
  g_free (title);

  return result;
}

static gboolean
_save_data (FrogrCreateNewSetDialog *self)
{
  FrogrCreateNewSetDialogPrivate *priv = NULL;
  GtkTextIter start;
  GtkTextIter end;
  gchar *title = NULL;
  gchar *description = NULL;
  gboolean result = FALSE;

  priv = FROGR_CREATE_NEW_SET_DIALOG_GET_PRIVATE (self);

  /* Save data */
  title = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->title_entry)));
  title = g_strstrip (title);

  gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (priv->description_buffer),
                              &start, &end);
  description = gtk_text_buffer_get_text (GTK_TEXT_BUFFER (priv->description_buffer),
                                          &start, &end, FALSE);
  description = g_strstrip (description);

  /* validate and update */
  result = _validate_dialog_data (self);
  if (result)
    _update_model (self, title, description);
  else
    frogr_util_show_error_dialog (GTK_WINDOW (self), _("Missing data required"));

  /* free */
  g_free (title);
  g_free (description);

  /* Return result */
  return result;
}

static void
_update_model (FrogrCreateNewSetDialog *self,
               const gchar *title,
               const gchar *description)
{
  FrogrCreateNewSetDialogPrivate *priv = NULL;
  FrogrController *controller = NULL;
  FrogrModel *model = NULL;
  FrogrPhotoSet *new_set = NULL;
  FrogrPicture *picture = NULL;
  GSList *item = NULL;

  priv = FROGR_CREATE_NEW_SET_DIALOG_GET_PRIVATE (self);
  controller = frogr_controller_get_instance ();
  model = frogr_controller_get_model (controller);

  /* Add the set to the model */
  new_set = frogr_photoset_new_local (title, description);
  frogr_model_add_local_photoset (model, new_set);

  /* Add the set to the list of sets for each picture */
  for (item = priv->pictures; item; item = g_slist_next (item))
    {
      picture = FROGR_PICTURE (item->data);
      frogr_picture_add_photoset (picture, new_set);

      /* Copy album's details over pictures if requested */
      if (priv->copy_to_pictures)
        {
          frogr_picture_set_title (picture, title);
          frogr_picture_set_description (picture, description);
        }
    }
  g_object_unref (new_set);

  frogr_model_notify_changes_in_pictures (model);
}

static void
_dialog_response_cb (GtkDialog *dialog, gint response, gpointer data)
{
  FrogrCreateNewSetDialog *self = NULL;

  /* Try to save data if response is OK */
  self = FROGR_CREATE_NEW_SET_DIALOG (dialog);
  if (response == GTK_RESPONSE_ACCEPT && _save_data (self) == FALSE)
    return;

  /* Destroy the dialog */
  gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
_frogr_create_new_set_dialog_set_property (GObject *object,
                                           guint prop_id,
                                           const GValue *value,
                                           GParamSpec *pspec)
{
  switch (prop_id)
    {
    case PROP_PICTURES:
      _set_pictures (FROGR_CREATE_NEW_SET_DIALOG (object), g_value_get_pointer (value));
      break;
    case PROP_PHOTOSETS:
      _set_photosets (FROGR_CREATE_NEW_SET_DIALOG (object), g_value_get_pointer (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
_frogr_create_new_set_dialog_get_property (GObject *object,
                                           guint prop_id,
                                           GValue *value,
                                           GParamSpec *pspec)
{
  FrogrCreateNewSetDialogPrivate *priv = FROGR_CREATE_NEW_SET_DIALOG_GET_PRIVATE (object);

  switch (prop_id)
    {
    case PROP_PICTURES:
      g_value_set_pointer (value, priv->pictures);
      break;
    case PROP_PHOTOSETS:
      g_value_set_pointer (value, priv->photosets);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
_frogr_create_new_set_dialog_dispose (GObject *object)
{
  FrogrCreateNewSetDialogPrivate *priv = FROGR_CREATE_NEW_SET_DIALOG_GET_PRIVATE (object);

  if (priv->pictures)
    {
      g_slist_foreach (priv->pictures, (GFunc)g_object_unref, NULL);
      g_slist_free (priv->pictures);
      priv->pictures = NULL;
    }

  if (priv->photosets)
    {
      g_slist_foreach (priv->photosets, (GFunc)g_object_unref, NULL);
      g_slist_free (priv->photosets);
      priv->photosets = NULL;
    }

  G_OBJECT_CLASS(frogr_create_new_set_dialog_parent_class)->dispose (object);
}

static void
frogr_create_new_set_dialog_class_init (FrogrCreateNewSetDialogClass *klass)
{
  GObjectClass *obj_class = (GObjectClass *)klass;
  GParamSpec *pspec;

  /* GObject signals */
  obj_class->set_property = _frogr_create_new_set_dialog_set_property;
  obj_class->get_property = _frogr_create_new_set_dialog_get_property;
  obj_class->dispose = _frogr_create_new_set_dialog_dispose;

  /* Install properties */
  pspec = g_param_spec_pointer ("pictures",
                                "pictures",
                                "List of pictures for "
                                "the 'add to set' dialog",
                                G_PARAM_READWRITE
                                | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (obj_class, PROP_PICTURES, pspec);

  pspec = g_param_spec_pointer ("photosets",
                                "photosets",
                                "List of sets currently available "
                                "for the 'add to set' dialog",
                                G_PARAM_READWRITE
                                | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (obj_class, PROP_PHOTOSETS, pspec);

  g_type_class_add_private (obj_class, sizeof (FrogrCreateNewSetDialogPrivate));
}

static void
frogr_create_new_set_dialog_init (FrogrCreateNewSetDialog *self)
{
  FrogrCreateNewSetDialogPrivate *priv = NULL;
  GtkWidget *content_area = NULL;
  GtkWidget *vbox = NULL;
  GtkWidget *grid = NULL;
  GtkWidget *scroller = NULL;
  GtkWidget *widget = NULL;

  priv = FROGR_CREATE_NEW_SET_DIALOG_GET_PRIVATE (self);
  priv->pictures = NULL;
  priv->photosets = NULL;

  /* Create widgets */
  gtk_dialog_add_buttons (GTK_DIALOG (self),
                          _("_Cancel"),
                          GTK_RESPONSE_CANCEL,
                          _("_Add"),
                          GTK_RESPONSE_ACCEPT,
                          NULL);
  gtk_container_set_border_width (GTK_CONTAINER (self), 6);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (self));
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_margin_bottom (vbox, 12);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 12);
  gtk_box_pack_start (GTK_BOX (vbox), grid, TRUE, TRUE, 0);

  widget = gtk_label_new (_("Title:"));
  gtk_widget_set_halign (GTK_WIDGET (widget), GTK_ALIGN_END);
  gtk_grid_attach (GTK_GRID (grid), widget, 0, 0, 1, 1);

  widget = gtk_entry_new ();
  gtk_widget_set_hexpand (GTK_WIDGET (widget), TRUE);
  gtk_grid_attach (GTK_GRID (grid), widget, 1, 0, 1, 1);
  priv->title_entry = widget;

  widget = gtk_label_new (_("Description:"));
  gtk_widget_set_halign (GTK_WIDGET (widget), GTK_ALIGN_END);
  gtk_widget_set_valign (GTK_WIDGET (widget), GTK_ALIGN_START);
  gtk_grid_attach (GTK_GRID (grid), widget, 0, 1, 1, 1);

  widget = gtk_text_view_new ();
  scroller = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroller),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroller),
                                       GTK_SHADOW_ETCHED_IN);
  gtk_container_add (GTK_CONTAINER (scroller), widget);
  gtk_text_view_set_accepts_tab (GTK_TEXT_VIEW (widget), FALSE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (widget), GTK_WRAP_WORD);
  gtk_widget_set_hexpand (GTK_WIDGET (scroller), TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (scroller), TRUE);
  gtk_grid_attach (GTK_GRID (grid), scroller, 1, 1, 1, 1);

  priv->description_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
  priv->description_tv = widget;

  widget = gtk_check_button_new_with_mnemonic (_("Fill Pictures Details with Title and Description"));
  gtk_widget_set_hexpand (GTK_WIDGET (widget), TRUE);
  gtk_grid_attach (GTK_GRID (grid), widget, 1, 2, 1, 1);
  priv->copy_to_pictures_cb = widget;

  gtk_container_add (GTK_CONTAINER (content_area), vbox);

  g_signal_connect (G_OBJECT (priv->copy_to_pictures_cb), "toggled",
                    G_CALLBACK (_on_button_toggled),
                    self);

  gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_ACCEPT);

  priv->copy_to_pictures = FALSE;
}

/* Public API */

void
frogr_create_new_set_dialog_show (GtkWindow *parent,
                                  const GSList *pictures,
                                  const GSList *photosets)
{
  GtkWidget *dialog = NULL;
  dialog = GTK_WIDGET (g_object_new (FROGR_TYPE_CREATE_NEW_SET_DIALOG,
                                     "title", _("Create New Set"),
                                     "modal", TRUE,
                                     "pictures", pictures,
                                     "photosets", photosets,
                                     "transient-for", parent,
                                     "width-request", -1,
                                     "height-request", 300,
                                     "resizable", TRUE,
#if GTK_CHECK_VERSION (3, 12, 0)
                                     "use-header-bar", USE_HEADER_BAR,
#endif
                                     NULL));

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (_dialog_response_cb), NULL);

  gtk_widget_show_all (dialog);
}
