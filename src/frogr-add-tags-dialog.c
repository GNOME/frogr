/*
 * frogr-add-tags-dialog.c -- frogr 'add tags' dialog
 *
 * Copyright (C) 2009-2018 Mario Sanchez Prada
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

#include "frogr-add-tags-dialog.h"

#include "frogr-config.h"
#include "frogr-controller.h"
#include "frogr-global-defs.h"
#include "frogr-live-entry.h"
#include "frogr-main-view.h"
#include "frogr-model.h"
#include "frogr-picture.h"

#include <config.h>
#include <glib/gi18n.h>


struct _FrogrAddTagsDialog {
  GtkDialog parent;

  GtkWidget *entry;
  GSList *pictures;
};

G_DEFINE_TYPE (FrogrAddTagsDialog, frogr_add_tags_dialog, GTK_TYPE_DIALOG)


/* Properties */
enum  {
  PROP_0,
  PROP_PICTURES
};


/* Prototypes */

static void _set_pictures (FrogrAddTagsDialog *self, const GSList *pictures);

static void _dialog_response_cb (GtkDialog *dialog, gint response, gpointer data);

/* Private API */

static void
_set_pictures (FrogrAddTagsDialog *self, const GSList *pictures)
{
  self->pictures = g_slist_copy ((GSList*) pictures);
  g_slist_foreach (self->pictures, (GFunc)g_object_ref, NULL);
}

static void
_dialog_response_cb (GtkDialog *dialog, gint response, gpointer data)
{
  if (response == GTK_RESPONSE_ACCEPT)
    {
      FrogrAddTagsDialog *self = NULL;
      g_autofree gchar *tags = NULL;

      self = FROGR_ADD_TAGS_DIALOG (dialog);

      /* Update pictures data */
      tags = g_strdup (gtk_entry_get_text (GTK_ENTRY (self->entry)));
      tags = g_strstrip (tags);

      /* Check if there's something to add */
      if (tags && !g_str_equal (tags, ""))
        {
          FrogrModel *model = NULL;
          FrogrPicture *picture = NULL;
          GSList *item = NULL;

          DEBUG ("Adding tags to picture(s): %s", tags);

          /* Iterate over the rest of elements */
          for (item = self->pictures; item; item = g_slist_next (item))
            {
              picture = FROGR_PICTURE (item->data);
              frogr_picture_add_tags (picture, tags);
            }

          /* Add tags to the model */
          model = frogr_controller_get_model (frogr_controller_get_instance ());
          frogr_model_add_local_tags_from_string (model, tags);
        }
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
  switch (prop_id)
    {
    case PROP_PICTURES:
      _set_pictures (FROGR_ADD_TAGS_DIALOG (object), g_value_get_pointer (value));
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
  FrogrAddTagsDialog *dialog = FROGR_ADD_TAGS_DIALOG (object);

  switch (prop_id)
    {
    case PROP_PICTURES:
      g_value_set_pointer (value, dialog->pictures);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
_frogr_add_tags_dialog_dispose (GObject *object)
{
  FrogrAddTagsDialog *dialog = FROGR_ADD_TAGS_DIALOG (object);

  if (dialog->pictures)
    {
      g_slist_free_full (dialog->pictures, g_object_unref);
      dialog->pictures = NULL;
    }

  G_OBJECT_CLASS(frogr_add_tags_dialog_parent_class)->dispose (object);
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

  /* Install properties */
  pspec = g_param_spec_pointer ("pictures",
                                "pictures",
                                "List of pictures for "
                                "the 'add tags' dialog",
                                G_PARAM_READWRITE
                                | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (obj_class, PROP_PICTURES, pspec);
}

static void
frogr_add_tags_dialog_init (FrogrAddTagsDialog *self)
{
  GtkWidget *content_area = NULL;
  GtkWidget *vbox = NULL;
  GtkWidget *label = NULL;

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
  gtk_widget_set_margin_bottom (vbox, 6);
  gtk_widget_show (vbox);

  label = gtk_label_new (_("Enter a spaces separated list of tags:"));
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  self->entry = frogr_live_entry_new ();
  gtk_box_pack_start (GTK_BOX (vbox), self->entry, FALSE, FALSE, 0);
  gtk_widget_show (self->entry);

  gtk_widget_set_size_request (GTK_WIDGET (self), 300, -1);

  gtk_container_add (GTK_CONTAINER (content_area), vbox);

  g_signal_connect (G_OBJECT (self), "response",
                    G_CALLBACK (_dialog_response_cb), NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_ACCEPT);
}

/* Public API */

void
frogr_add_tags_dialog_show (GtkWindow *parent, const GSList *pictures, const GSList *tags)
{
  FrogrConfig *config = NULL;
  GObject *new = NULL;

  new = g_object_new (FROGR_TYPE_ADD_TAGS_DIALOG,
                      "title", _("Add Tags"),
                      "modal", TRUE,
                      "pictures", pictures,
                      "transient-for", parent,
                      "resizable", TRUE,
#if GTK_CHECK_VERSION (3, 12, 0)
                      "use-header-bar", USE_HEADER_BAR,
#endif
                      NULL);

  /* Enable autocompletion if needed */
  config = frogr_config_get_instance ();
  if (config && frogr_config_get_tags_autocompletion (config))
    {
      FrogrAddTagsDialog *dialog = FROGR_ADD_TAGS_DIALOG (new);
      frogr_live_entry_set_auto_completion (FROGR_LIVE_ENTRY (dialog->entry), tags);
    }

  gtk_widget_show (GTK_WIDGET (new));
}
