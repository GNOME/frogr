/*
 * frogr-details-dialog.c -- Picture details dialog
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

#include "frogr-details-dialog.h"

#include "frogr-config.h"
#include "frogr-controller.h"
#include "frogr-global-defs.h"
#include "frogr-live-entry.h"
#include "frogr-main-view.h"
#include "frogr-model.h"
#include "frogr-picture.h"
#include "frogr-util.h"

#include <config.h>
#include <glib/gi18n.h>
#include <flicksoup/flicksoup.h>

/* Path relative to the application data dir */
#define MPICTURES_IMAGE "/images/mpictures.png"

#define DIALOG_MIN_WIDTH 700
#define DIALOG_MIN_HEIGHT 480

#define PICTURE_WIDTH 180
#define PICTURE_HEIGHT 140


struct _FrogrDetailsDialog {
  GtkDialog parent;

  GtkWidget *title_entry;
  GtkWidget *desc_tview;
  GtkWidget *tags_entry;
  GtkWidget *public_rb;
  GtkWidget *private_rb;
  GtkWidget *friend_cb;
  GtkWidget *family_cb;
  GtkWidget *show_in_search_cb;
  GtkWidget *send_location_cb;
  GtkWidget *replace_date_posted_cb;
  GtkWidget *license_cb;
  GtkWidget *photo_content_rb;
  GtkWidget *sshot_content_rb;
  GtkWidget *other_content_rb;
  GtkWidget *safe_rb;
  GtkWidget *moderate_rb;
  GtkWidget *restricted_rb;
  GtkTextBuffer *text_buffer;

  GtkWidget *picture_img;
  GtkWidget *picture_button;
  GtkWidget *picture_container;
  GtkWidget *mpictures_label;
  GdkPixbuf *mpictures_pixbuf;

  GSList *pictures;
  gulong picture_button_handler_id;
  gchar *reference_description;
};

G_DEFINE_TYPE (FrogrDetailsDialog, frogr_details_dialog, GTK_TYPE_DIALOG)

/* Properties */
enum  {
  PROP_0,
  PROP_PICTURES
};


static const gchar *license_descriptions[] = {
  N_("Default (as specified in Flickr)"),
  N_("All rights reserved"),
  N_("CC Attribution-NonCommercial-ShareAlike"),
  N_("CC Attribution-NonCommercial"),
  N_("CC Attribution-NonCommercial-NoDerivs"),
  N_("CC Attribution"),
  N_("CC Attribution-ShareAlike"),
  N_("CC Attribution-NoDerivs"),
  NULL
};


/* Prototypes */

static void _set_pictures (FrogrDetailsDialog *self, const GSList *pictures);

static void _create_widgets (FrogrDetailsDialog *self);

static void _update_ui (FrogrDetailsDialog *self);

static void _load_picture_from_disk (FrogrDetailsDialog *self);

static void _load_picture_from_disk_cb (GObject *object,
                                        GAsyncResult *res,
                                        gpointer data);

static void _place_picture_in_dialog_and_show (FrogrDetailsDialog *self);

static void _fill_dialog_with_data (FrogrDetailsDialog *self);

static gboolean _validate_dialog_data (FrogrDetailsDialog *self);

static gboolean _save_data (FrogrDetailsDialog *self);

static void _on_radio_button_clicked (GtkButton *tbutton, gpointer data);

static void _on_picture_button_clicked (GtkButton *button, gpointer data);

static void _on_toggle_button_toggled (GtkToggleButton *tbutton, gpointer data);

static void _dialog_response_cb (GtkDialog *dialog, gint response, gpointer data);


/* Private API */

static void
_set_pictures (FrogrDetailsDialog *self, const GSList *pictures)
{
  self->pictures = g_slist_copy ((GSList*) pictures);
  g_slist_foreach (self->pictures, (GFunc)g_object_ref, NULL);
}

static void
_create_widgets (FrogrDetailsDialog *self)
{
  GtkWidget *main_vbox = NULL;
  GtkWidget *vbox = NULL;
  GtkWidget *hbox = NULL;
  GtkWidget *section_vbox = NULL;
  GtkWidget *internal_hbox = NULL;
  GtkWidget *private_hbox = NULL;
  GtkWidget *content_type_hbox = NULL;
  GtkWidget *safety_level_hbox = NULL;
  GtkWidget *picture_frame = NULL;
  GtkWidget *label = NULL;
  GtkWidget *widget = NULL;
  GtkWidget *grid = NULL;
  GtkWidget *scroller = NULL;
  gchar *markup = NULL;
  gint i;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 18);
  gtk_widget_set_margin_bottom (hbox, 6);
  gtk_widget_show (hbox);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_show (vbox);

  /* Left side (image, radio buttons, checkboxes...) */

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 12);
  gtk_widget_show (grid);

  widget = gtk_button_new ();
  gtk_widget_set_tooltip_text (widget, _("Open with image viewer"));
  gtk_button_set_relief (GTK_BUTTON (widget), GTK_RELIEF_NONE);
  gtk_widget_show (widget);
  self->picture_button = widget;

  picture_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (picture_frame), GTK_SHADOW_NONE);
  gtk_widget_set_halign (GTK_WIDGET (picture_frame), GTK_ALIGN_CENTER);
  gtk_widget_show (picture_frame);

  gtk_grid_attach (GTK_GRID (grid), picture_frame, 1, 0, 1, 1);
  self->picture_container = picture_frame;
  self->picture_img = gtk_image_new ();

  widget = gtk_label_new (NULL);
  gtk_widget_set_margin_bottom (widget, 6);
  gtk_grid_attach (GTK_GRID (grid), widget, 1, 1, 1, 1);
  gtk_widget_show (widget);
  self->mpictures_label = widget;
  self->mpictures_pixbuf = NULL;

  label = gtk_label_new_with_mnemonic (_("_Title:"));
  gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_END);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 2, 1, 1);
  gtk_widget_show (label);

  widget = gtk_entry_new ();
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), widget);
  gtk_widget_set_hexpand (GTK_WIDGET (widget), TRUE);
  gtk_grid_attach (GTK_GRID (grid), widget, 1, 2, 1, 1);
  gtk_widget_show (widget);
  self->title_entry = widget;

  label = gtk_label_new_with_mnemonic (_("_Description:"));
  gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_END);
  gtk_widget_set_valign (GTK_WIDGET (label), GTK_ALIGN_START);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 3, 1, 1);
  gtk_widget_show (label);

  widget = gtk_text_view_new ();
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), widget);
  gtk_text_view_set_accepts_tab (GTK_TEXT_VIEW (widget), FALSE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (widget), GTK_WRAP_WORD);
  gtk_widget_show (widget);

  scroller = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroller),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroller),
                                       GTK_SHADOW_ETCHED_IN);
  gtk_container_add (GTK_CONTAINER (scroller), widget);

  gtk_widget_set_hexpand (GTK_WIDGET (scroller), TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (scroller), TRUE);
  gtk_grid_attach (GTK_GRID (grid), scroller, 1, 3, 1, 1);
  gtk_widget_show (scroller);
  self->desc_tview = widget;
  self->text_buffer =
    gtk_text_view_get_buffer (GTK_TEXT_VIEW (self->desc_tview));

  label = gtk_label_new_with_mnemonic (_("Ta_gs:"));
  gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_END);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 4, 1, 1);
  gtk_widget_show (label);

  widget = frogr_live_entry_new ();
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), widget);
  gtk_widget_set_hexpand (GTK_WIDGET (widget), TRUE);
  gtk_grid_attach (GTK_GRID (grid), widget, 1, 4, 1, 1);
  gtk_widget_show (widget);
  self->tags_entry = widget;

  gtk_box_pack_start (GTK_BOX (vbox), grid, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

  /* Right side (text fields) */

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_widget_show (vbox);

  /* Visibility */

  section_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_show (section_vbox);

  markup = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>",
                                    _("Visibility"));
  widget = gtk_label_new (markup);
  gtk_label_set_use_markup (GTK_LABEL (widget), TRUE);
  g_free (markup);

  gtk_widget_set_halign (widget, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (section_vbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  internal_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

  widget = gtk_radio_button_new_with_mnemonic (NULL, _("_Private"));
  gtk_box_pack_start (GTK_BOX (internal_hbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);
  self->private_rb = widget;

  widget = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (self->private_rb), _("P_ublic"));
  gtk_box_pack_start (GTK_BOX (internal_hbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);
  self->public_rb = widget;

  gtk_box_pack_start (GTK_BOX (section_vbox), internal_hbox, FALSE, FALSE, 0);
  gtk_widget_show (internal_hbox);

  internal_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_show (internal_hbox);

  private_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_show (private_hbox);

  widget = gtk_check_button_new_with_mnemonic (_("_Family"));
  gtk_box_pack_start (GTK_BOX (private_hbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);
  self->family_cb = widget;

  widget = gtk_check_button_new_with_mnemonic (_("F_riends"));
  gtk_box_pack_start (GTK_BOX (private_hbox), widget, FALSE, FALSE, 0);
  self->friend_cb = widget;

  gtk_box_pack_start (GTK_BOX (internal_hbox), private_hbox, FALSE, FALSE, 12);
  gtk_box_pack_start (GTK_BOX (section_vbox), internal_hbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), section_vbox, FALSE, FALSE, 0);

  /* Content type */

  section_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_show (section_vbox);

  content_type_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_show (content_type_hbox);

  markup = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>",
                                    _("Content Type"));
  widget = gtk_label_new (markup);
  gtk_label_set_use_markup (GTK_LABEL (widget), TRUE);
  g_free (markup);

  gtk_widget_set_halign (widget, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (section_vbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  widget = gtk_radio_button_new_with_mnemonic (NULL, _("P_hoto"));
  gtk_box_pack_start (GTK_BOX (content_type_hbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);
  self->photo_content_rb = widget;

  widget = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (self->photo_content_rb), _("Scree_nshot"));
  gtk_box_pack_start (GTK_BOX (content_type_hbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);
  self->sshot_content_rb = widget;

  widget = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (self->photo_content_rb), _("Oth_er"));
  gtk_box_pack_start (GTK_BOX (content_type_hbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);
  self->other_content_rb = widget;

  gtk_box_pack_start (GTK_BOX (section_vbox), content_type_hbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), section_vbox, FALSE, FALSE, 0);

  /* Safety level */

  section_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_show (section_vbox);

  safety_level_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_show (safety_level_hbox);

  markup = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>",
                                    _("Safety Level"));
  widget = gtk_label_new (markup);
  gtk_label_set_use_markup (GTK_LABEL (widget), TRUE);
  g_free (markup);

  gtk_widget_set_halign (widget, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (section_vbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  widget = gtk_radio_button_new_with_mnemonic (NULL, _("S_afe"));
  gtk_box_pack_start (GTK_BOX (safety_level_hbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);
  self->safe_rb = widget;

  widget = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (self->safe_rb), _("_Moderate"));
  gtk_box_pack_start (GTK_BOX (safety_level_hbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);
  self->moderate_rb = widget;

  widget = gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON (self->safe_rb), _("Restr_icted"));
  gtk_box_pack_start (GTK_BOX (safety_level_hbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);
  self->restricted_rb = widget;

  gtk_box_pack_start (GTK_BOX (section_vbox), safety_level_hbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), section_vbox, FALSE, FALSE, 0);

  /* License type */

  section_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_show (section_vbox);

  markup = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>",
                                    _("License Type"));
  widget = gtk_label_new (markup);
  gtk_label_set_use_markup (GTK_LABEL (widget), TRUE);
  g_free (markup);

  gtk_widget_set_halign (widget, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (section_vbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  widget = gtk_combo_box_text_new ();
  for (i = 0; license_descriptions[i]; i++)
    gtk_combo_box_text_insert (GTK_COMBO_BOX_TEXT (widget), i, NULL, _(license_descriptions[i]));

  gtk_widget_show (widget);
  self->license_cb = widget;

  gtk_box_pack_start (GTK_BOX (section_vbox), widget, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), section_vbox, FALSE, FALSE, 0);

  /* Other properties */

  section_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_show (section_vbox);

  markup = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>",
                                    _("Other Properties"));
  widget = gtk_label_new (markup);
  gtk_label_set_use_markup (GTK_LABEL (widget), TRUE);
  g_free (markup);

  gtk_widget_set_halign (widget, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (section_vbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  widget = gtk_check_button_new_with_mnemonic (_("_Show Up in Global Search Results"));
  gtk_box_pack_start (GTK_BOX (section_vbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);
  self->show_in_search_cb = widget;

  widget = gtk_check_button_new_with_mnemonic (_("Set Geo_location Information"));
  gtk_box_pack_start (GTK_BOX (section_vbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);
  self->send_location_cb = widget;

  widget = gtk_check_button_new_with_mnemonic (_("Replace 'Date Posted' with 'Date Taken'"));
  gtk_box_pack_start (GTK_BOX (section_vbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);
  self->replace_date_posted_cb = widget;

  gtk_box_pack_start (GTK_BOX (vbox), section_vbox, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);

  main_vbox = gtk_dialog_get_content_area (GTK_DIALOG (self));
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, TRUE, TRUE, 0);

  /* Connect signals */
  g_signal_connect (G_OBJECT (self->public_rb), "clicked",
                    G_CALLBACK (_on_radio_button_clicked), self);

  g_signal_connect (G_OBJECT (self->private_rb), "clicked",
                    G_CALLBACK (_on_radio_button_clicked), self);

  g_signal_connect (G_OBJECT (self->family_cb), "toggled",
                    G_CALLBACK (_on_toggle_button_toggled), self);

  g_signal_connect (G_OBJECT (self->friend_cb), "toggled",
                    G_CALLBACK (_on_toggle_button_toggled), self);

  g_signal_connect (G_OBJECT (self->show_in_search_cb), "toggled",
                    G_CALLBACK (_on_toggle_button_toggled), self);

  g_signal_connect (G_OBJECT (self->send_location_cb), "toggled",
                    G_CALLBACK (_on_toggle_button_toggled), self);

  g_signal_connect (G_OBJECT (self->replace_date_posted_cb), "toggled",
                    G_CALLBACK (_on_toggle_button_toggled), self);

  g_signal_connect (G_OBJECT (self->photo_content_rb), "clicked",
                    G_CALLBACK (_on_radio_button_clicked), self);

  g_signal_connect (G_OBJECT (self->sshot_content_rb), "clicked",
                    G_CALLBACK (_on_radio_button_clicked), self);

  g_signal_connect (G_OBJECT (self->other_content_rb), "clicked",
                    G_CALLBACK (_on_radio_button_clicked), self);

  g_signal_connect (G_OBJECT (self->safe_rb), "clicked",
                    G_CALLBACK (_on_radio_button_clicked), self);

  g_signal_connect (G_OBJECT (self->moderate_rb), "clicked",
                    G_CALLBACK (_on_radio_button_clicked), self);

  g_signal_connect (G_OBJECT (self->restricted_rb), "clicked",
                    G_CALLBACK (_on_radio_button_clicked), self);

  /* Show widgets */
  gtk_widget_show (main_vbox);
}

static void
_update_ui (FrogrDetailsDialog *self)
{
  gboolean active;

  /* Adjust sensitiveness for check buttons */
  active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->public_rb));
  gtk_widget_set_sensitive (self->friend_cb, !active);
  gtk_widget_set_sensitive (self->family_cb, !active);
}

static void
_load_picture_from_disk (FrogrDetailsDialog *self)
{
  guint n_pictures;

  n_pictures = g_slist_length (self->pictures);
  if (n_pictures > 1)
    {
      g_autofree gchar *mpictures_str = NULL;

      /* Get the 'multiple pictures pixbuf' if not got yet (lazy approach) */
      if (!self->mpictures_pixbuf)
        {
          g_autofree gchar *mpictures_full_path = NULL;

          /* Set the image for editing multiple pictures */
          mpictures_full_path = g_strdup_printf ("%s/" MPICTURES_IMAGE,
                                                 frogr_util_get_app_data_dir ());
          self->mpictures_pixbuf = gdk_pixbuf_new_from_file (mpictures_full_path, NULL);
        }

      /* Just set the pixbuf in the image */
      gtk_image_set_from_pixbuf (GTK_IMAGE (self->picture_img), self->mpictures_pixbuf);

      /* Visually indicate how many pictures are being edited */
      mpictures_str = g_strdup_printf (ngettext ("(%d Picture)", "(%d Pictures)", n_pictures), n_pictures);
      gtk_label_set_text (GTK_LABEL (self->mpictures_label), mpictures_str);

      /* No need to spawn any async operation, show the dialog now */
      _place_picture_in_dialog_and_show (self);
    }
  else
    {
      FrogrPicture *picture = NULL;
      const gchar *file_uri = NULL;
      GFile *gfile = NULL;

      picture = FROGR_PICTURE (self->pictures->data);
      file_uri = frogr_picture_get_fileuri (picture);
      gfile = g_file_new_for_uri (file_uri);

      /* Asynchronously load the picture */
      g_file_load_contents_async (gfile, NULL, _load_picture_from_disk_cb, self);
    }
}

static void
_load_picture_from_disk_cb (GObject *object,
                            GAsyncResult *res,
                            gpointer data)
{
  FrogrDetailsDialog *self = FROGR_DETAILS_DIALOG (data);
  GFile *file = NULL;
  g_autoptr(GError) error = NULL;
  g_autofree gchar *contents = NULL;
  gsize length = 0;

  file = G_FILE (object);
  if (g_file_load_contents_finish (file, res, &contents, &length, NULL, &error))
    {
      FrogrPicture *picture = NULL;
      g_autoptr(GdkPixbuf) pixbuf = NULL;

      picture = FROGR_PICTURE (self->pictures->data);
      if (frogr_picture_is_video (picture))
        {
          pixbuf = frogr_util_get_pixbuf_for_video_file (file, PICTURE_WIDTH, PICTURE_HEIGHT, &error);
        }
      else
        {
          g_autofree gchar *path = NULL;

          path = g_file_get_path (file);
          pixbuf = frogr_util_get_pixbuf_from_image_contents ((const guchar *)contents, length,
                                                              PICTURE_WIDTH, PICTURE_HEIGHT, path, &error);
        }

      if (pixbuf)
        gtk_image_set_from_pixbuf (GTK_IMAGE (self->picture_img), pixbuf);

      /* Everything should be fine by now, show it */
      _place_picture_in_dialog_and_show (self);
    }

  /* Show error to the user and finalize dialog if needed */
  if (error)
    {
      GtkWindow *parent_window = NULL;
      g_autofree gchar *error_msg = NULL;

      parent_window = gtk_window_get_transient_for (GTK_WINDOW (self));
      gtk_widget_destroy (GTK_WIDGET (self));

      if (error)
        error_msg = g_strdup (error->message);
      else
        error_msg = g_strdup (_("An error happened trying to load the picture"));

      frogr_util_show_error_dialog (parent_window, error_msg);
    }
}

static void
_place_picture_in_dialog_and_show (FrogrDetailsDialog *self)
{
  gtk_button_set_image (GTK_BUTTON (self->picture_button),
                        self->picture_img);

  gtk_container_add (GTK_CONTAINER (self->picture_container),
                     self->picture_button);

  self->picture_button_handler_id =
    g_signal_connect (G_OBJECT (self->picture_button), "clicked",
                      G_CALLBACK (_on_picture_button_clicked),
                      self);

  gtk_widget_show (GTK_WIDGET (self));
}

static void
_fill_dialog_with_data (FrogrDetailsDialog *self)
{
  FrogrPicture *picture;
  GSList *item;
  GtkWidget *picture_widget;
  gchar *title_val = NULL;
  gchar *desc_val = NULL;
  gchar *tags_val = NULL;
  gboolean is_public_val = FALSE;
  gboolean is_friend_val = FALSE;
  gboolean is_family_val = FALSE;
  gboolean show_in_search_val = FALSE;
  gboolean send_location_val = FALSE;
  gboolean replace_date_posted_val = FALSE;
  gboolean license_inconsistent = FALSE;
  FspLicense license_val = FSP_LICENSE_NONE;
  FspSafetyLevel safety_level_val = FSP_SAFETY_LEVEL_NONE;
  FspContentType content_type_val = FSP_CONTENT_TYPE_NONE;

  /* Take first element values */
  item = self->pictures;
  picture = FROGR_PICTURE (item->data);

  title_val = (gchar *)frogr_picture_get_title (picture);
  desc_val = (gchar *)frogr_picture_get_description (picture);
  tags_val = (gchar *)frogr_picture_get_tags (picture);
  is_public_val = frogr_picture_is_public (picture);
  is_friend_val = frogr_picture_is_friend (picture);
  is_family_val = frogr_picture_is_family (picture);
  show_in_search_val = frogr_picture_show_in_search (picture);
  send_location_val = frogr_picture_send_location (picture);
  replace_date_posted_val = frogr_picture_replace_date_posted (picture);
  license_val = frogr_picture_get_license (picture);
  safety_level_val = frogr_picture_get_safety_level (picture);
  content_type_val = frogr_picture_get_content_type (picture);

  /* Iterate over the rest of elements */
  for (item = g_slist_next (item); item; item = g_slist_next (item))
    {
      gchar *title = NULL;
      gchar *desc = NULL;
      gchar *tags = NULL;
      gboolean is_public = FALSE;
      gboolean is_friend = FALSE;
      gboolean is_family = FALSE;
      gboolean show_in_search = FALSE;
      gboolean send_location = FALSE;
      gboolean replace_date_posted = FALSE;
      FspLicense license = FSP_LICENSE_NONE;
      FspSafetyLevel safety_level = FSP_SAFETY_LEVEL_NONE;
      FspContentType content_type = FSP_CONTENT_TYPE_NONE;

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
      show_in_search = frogr_picture_show_in_search (picture);
      send_location = frogr_picture_send_location (picture);
      replace_date_posted = frogr_picture_replace_date_posted (picture);

      /* License */
      license = frogr_picture_get_license (picture);

      /* Content type and safety level */
      safety_level = frogr_picture_get_safety_level (picture);
      content_type = frogr_picture_get_content_type (picture);

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
      if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->public_rb)))
        {
          gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (self->public_rb),
                                              is_public_val != is_public);
        }
      if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->private_rb)))
        {
          gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (self->private_rb),
                                              is_public_val != is_public);
        }
      if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->family_cb)))
        {
          gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (self->family_cb),
                                              is_family_val != is_family);
        }
      if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->friend_cb)))
        {
          gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (self->friend_cb),
                                              is_friend_val != is_friend);
        }
      if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->show_in_search_cb)))
        {
          gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (self->show_in_search_cb),
                                              show_in_search_val != show_in_search);
        }
      if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->send_location_cb)))
        {
          gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (self->send_location_cb),
                                              send_location_val != send_location);
        }
      if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->replace_date_posted_cb)))
        {
          gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (self->replace_date_posted_cb),
                                              replace_date_posted_val != replace_date_posted);
        }

      if (!license_inconsistent && license_val != license)
        {
          license_inconsistent = TRUE;
        }

      if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->photo_content_rb)))
        {
          gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (self->photo_content_rb),
                                              content_type_val != content_type);
        }
      if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->photo_content_rb)))
        {
          gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (self->photo_content_rb),
                                              content_type_val != content_type);
        }
      if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->sshot_content_rb)))
        {
          gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (self->sshot_content_rb),
                                              content_type_val != content_type);
        }
      if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->other_content_rb)))
        {
          gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (self->other_content_rb),
                                              content_type_val != content_type);
        }
      if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->safe_rb)))
        {
          gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (self->safe_rb),
                                              safety_level_val != safety_level);
        }
      if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->moderate_rb)))
        {
          gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (self->moderate_rb),
                                              safety_level_val != safety_level);
        }
      if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->restricted_rb)))
        {
          gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (self->restricted_rb),
                                              safety_level_val != safety_level);
        }

      /* Update merged value */
      is_public_val = is_public;
      is_friend_val = is_friend;
      is_family_val = is_family;
      show_in_search_val = show_in_search;
      send_location_val = send_location;
      replace_date_posted_val = replace_date_posted;
      license_val = license;
      content_type_val = content_type;
      safety_level_val = safety_level;
    }

  /* Fill in with data */

  if (title_val != NULL)
    gtk_entry_set_text (GTK_ENTRY (self->title_entry), title_val);

  if (desc_val != NULL)
    {
      gtk_text_buffer_set_text (GTK_TEXT_BUFFER (self->text_buffer), desc_val, -1);
      self->reference_description = g_strstrip (g_strdup (desc_val));
    }
  else
    {
      /* We store "" in this case for ease further comparisons. */
      self->reference_description = g_strdup ("");
    }

  if (tags_val != NULL)
    gtk_entry_set_text (GTK_ENTRY (self->tags_entry), tags_val);

  if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->public_rb)))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->public_rb), is_public_val);
  if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->private_rb)))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->private_rb), !is_public_val);

  if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->friend_cb)))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->friend_cb), is_friend_val);
  if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->family_cb)))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->family_cb), is_family_val);

  if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->show_in_search_cb)))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->show_in_search_cb), show_in_search_val);
  if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->send_location_cb)))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->send_location_cb), send_location_val);
  if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->replace_date_posted_cb)))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->replace_date_posted_cb), replace_date_posted_val);

  if (license_inconsistent)
    gtk_combo_box_set_active (GTK_COMBO_BOX (self->license_cb), FSP_LICENSE_LAST + 1);
  else
    gtk_combo_box_set_active (GTK_COMBO_BOX (self->license_cb), license_val + 1);

  if (content_type_val == FSP_CONTENT_TYPE_SCREENSHOT
      && !gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->sshot_content_rb)))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->sshot_content_rb), TRUE);
  else if (content_type_val == FSP_CONTENT_TYPE_OTHER
      && !gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->other_content_rb)))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->other_content_rb), TRUE);
  else if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->photo_content_rb)))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->photo_content_rb), TRUE);

  if (safety_level_val == FSP_SAFETY_LEVEL_MODERATE
      && !gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->moderate_rb)))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->moderate_rb), TRUE);
  else if (safety_level_val == FSP_SAFETY_LEVEL_RESTRICTED
      && !gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->restricted_rb)))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->restricted_rb), TRUE);
  else if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->safe_rb)))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->safe_rb), TRUE);

  picture_widget = gtk_bin_get_child (GTK_BIN (self->picture_container));
  if (picture_widget)
    {
      gtk_container_remove (GTK_CONTAINER (self->picture_container),
                            picture_widget);
    }

  if (self->picture_button_handler_id)
    {
      g_signal_handler_disconnect (self->picture_button,
                                   self->picture_button_handler_id);
    }

  /* Update UI */
  _update_ui (self);

  /* Initial widget to grab focus */
  gtk_widget_grab_focus (self->title_entry);
  gtk_editable_set_position (GTK_EDITABLE (self->title_entry), -1);

  /* Load the picture from disk, asynchronously */
  _load_picture_from_disk (self);
}

static gboolean
_validate_dialog_data (FrogrDetailsDialog *self)
{
  gboolean result = TRUE;

  /* Mandatory fields (only if editing a single picture) */
  if (g_slist_length (self->pictures) <= 1)
    {
      g_autofree gchar *title =
        g_strdup (gtk_entry_get_text (GTK_ENTRY (self->title_entry)));

      if ((title == NULL) || g_str_equal (g_strstrip (title), ""))
        result = FALSE;
    }

  /* Validated if reached */
  return result;
}

static gboolean
_save_data (FrogrDetailsDialog *self)
{
  GtkTextIter start;
  GtkTextIter end;
  g_autofree gchar *title = NULL;
  g_autofree gchar *description = NULL;
  g_autofree gchar *tags = NULL;
  gboolean is_public;
  gboolean is_friend;
  gboolean is_family;
  gboolean show_in_search;
  gboolean send_location;
  gboolean replace_date_posted;
  FspLicense license;
  FspSafetyLevel safety_level;
  FspContentType content_type;
  gboolean result = FALSE;

  /* Save data */
  title = g_strdup (gtk_entry_get_text (GTK_ENTRY (self->title_entry)));
  title = g_strstrip (title);

  gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (self->text_buffer),
                              &start, &end);
  description = gtk_text_buffer_get_text (GTK_TEXT_BUFFER (self->text_buffer),
                                          &start, &end, FALSE);
  description = g_strstrip (description);

  tags = g_strdup (gtk_entry_get_text (GTK_ENTRY (self->tags_entry)));
  tags = g_strstrip (tags);

  is_public =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->public_rb));

  if (!is_public)
    {
      is_friend =
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->friend_cb));
      is_family =
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->family_cb));
    }
  else
    {
      /* This data is not used for public images */
      is_friend = FALSE;
      is_family = FALSE;
    }

  show_in_search =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->show_in_search_cb));
  send_location =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->send_location_cb));
  replace_date_posted =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->replace_date_posted_cb));

  /* License type (values in the combo are shifted +1) */
  license = gtk_combo_box_get_active (GTK_COMBO_BOX (self->license_cb)) - 1;

  /* Content type */
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->photo_content_rb)))
    content_type = FSP_CONTENT_TYPE_PHOTO;
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->sshot_content_rb)))
    content_type = FSP_CONTENT_TYPE_SCREENSHOT;
  else
    content_type = FSP_CONTENT_TYPE_OTHER;

  /* Safety level */
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->safe_rb)))
    safety_level = FSP_SAFETY_LEVEL_SAFE;
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->moderate_rb)))
    safety_level = FSP_SAFETY_LEVEL_MODERATE;
  else
    safety_level = FSP_SAFETY_LEVEL_RESTRICTED;

  /* validate dialog */
  if (_validate_dialog_data (self))
    {
      FrogrModel *model = NULL;
      FrogrPicture *picture;
      GSList *item;
      guint n_pictures;

      model = frogr_controller_get_model (frogr_controller_get_instance ());

      /* Iterate over the rest of elements */
      n_pictures = g_slist_length (self->pictures);
      for (item = self->pictures; item; item = g_slist_next (item))
        {
          picture = FROGR_PICTURE (item->data);

          if (!g_str_equal (title, "") || (n_pictures <= 1))
            frogr_picture_set_title (picture, title);

          if (!g_str_equal (description, self->reference_description))
            frogr_picture_set_description (picture, description);

          if (!g_str_equal (tags, "") || (n_pictures <= 1))
            frogr_picture_set_tags (picture, tags);

          if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->public_rb)))
            frogr_picture_set_public (picture, is_public);
          if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->friend_cb)))
            frogr_picture_set_friend (picture, is_friend);
          if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->family_cb)))
            frogr_picture_set_family (picture, is_family);
          if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->show_in_search_cb)))
            frogr_picture_set_show_in_search (picture, show_in_search);
          if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->send_location_cb)))
            frogr_picture_set_send_location (picture, send_location);
          if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->replace_date_posted_cb)))
            frogr_picture_set_replace_date_posted (picture, replace_date_posted);

          if (license >= FSP_LICENSE_NONE && license < FSP_LICENSE_LAST)
            frogr_picture_set_license (picture, license);

          if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->photo_content_rb))
              && !gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->sshot_content_rb))
              && !gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->other_content_rb)))
            frogr_picture_set_content_type (picture, content_type);

          if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->safe_rb))
              && !gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->moderate_rb))
              && !gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (self->restricted_rb)))
            frogr_picture_set_safety_level (picture, safety_level);

          /* Everything went fine */
          result = TRUE;
        }

      /* Add tags to the model */
      if (!g_str_equal (tags, ""))
        frogr_model_add_local_tags_from_string (model, tags);

      /* Notify the model that pictures details have probably changed */
      frogr_model_notify_changes_in_pictures (model);
    }
  else
    {
      /* This shows a dialog notifying the problem to the user */
      frogr_util_show_error_dialog (GTK_WINDOW (self), _("Missing data required"));
    }

  /* Return result */
  return result;
}

/* Event handlers */

static void
_on_radio_button_clicked (GtkButton *tbutton, gpointer data)
{
  FrogrDetailsDialog *self = NULL;
  GtkWidget *button = NULL;
  GSList *buttons = NULL;
  GSList *item = NULL;

  g_return_if_fail (GTK_IS_RADIO_BUTTON (tbutton));

  buttons = gtk_radio_button_get_group (GTK_RADIO_BUTTON (tbutton));

  for (item = buttons; item; item = g_slist_next (item))
    {
      button = GTK_WIDGET (item->data);
      gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (button), FALSE);
    }

  self = FROGR_DETAILS_DIALOG (data);
  _update_ui (self);
}

static void
_on_toggle_button_toggled (GtkToggleButton *tbutton, gpointer data)
{
  FrogrDetailsDialog *self = FROGR_DETAILS_DIALOG (data);
  /* Reset consistence and update UI */
  gtk_toggle_button_set_inconsistent (tbutton, FALSE);
  _update_ui (self);
}

static void
_on_picture_button_clicked (GtkButton *button, gpointer data)
{
  FrogrDetailsDialog *self = FROGR_DETAILS_DIALOG (data);
  frogr_util_open_pictures_in_viewer (self->pictures);
}

static void
_dialog_response_cb (GtkDialog *dialog, gint response, gpointer data)
{
  FrogrDetailsDialog *self = FROGR_DETAILS_DIALOG (dialog);

  /* Try to save data if response is OK */
  if (response == GTK_RESPONSE_ACCEPT && _save_data (self) == FALSE)
    return;

  gtk_widget_destroy (GTK_WIDGET (self));

  /* Ensure pictures are reordered on OK */
  if (response == GTK_RESPONSE_ACCEPT)
    frogr_controller_reorder_pictures (frogr_controller_get_instance ());
}

static void
_frogr_details_dialog_set_property (GObject *object,
                                    guint prop_id,
                                    const GValue *value,
                                    GParamSpec *pspec)
{
  switch (prop_id)
    {
    case PROP_PICTURES:
      _set_pictures (FROGR_DETAILS_DIALOG (object), g_value_get_pointer (value));
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
  FrogrDetailsDialog *dialog = FROGR_DETAILS_DIALOG (object);

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
_frogr_details_dialog_dispose (GObject *object)
{
  FrogrDetailsDialog *dialog = FROGR_DETAILS_DIALOG (object);

  if (dialog->mpictures_pixbuf)
    {
      g_object_unref (dialog->mpictures_pixbuf);
      dialog->mpictures_pixbuf = NULL;
    }

  if (dialog->pictures)
    {
      g_slist_free_full (dialog->pictures, g_object_unref);
      dialog->pictures = NULL;
    }

  G_OBJECT_CLASS(frogr_details_dialog_parent_class)->dispose (object);
}

static void
_frogr_details_dialog_finalize (GObject *object)
{
  FrogrDetailsDialog *dialog = FROGR_DETAILS_DIALOG (object);

  g_free (dialog->reference_description);

  G_OBJECT_CLASS(frogr_details_dialog_parent_class)->finalize (object);
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
  obj_class->finalize = _frogr_details_dialog_finalize;

  /* Install properties */
  pspec = g_param_spec_pointer ("pictures",
                                "pictures",
                                "List of pictures for "
                                "the details dialog",
                                G_PARAM_READWRITE
                                | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (obj_class, PROP_PICTURES, pspec);
}

static void
frogr_details_dialog_init (FrogrDetailsDialog *self)
{
  self->pictures = NULL;
  self->picture_button_handler_id = 0;
  self->reference_description = NULL;

  _create_widgets (self);

  gtk_dialog_add_buttons (GTK_DIALOG (self),
                          _("_Cancel"), GTK_RESPONSE_CANCEL,
                          _("_Edit"), GTK_RESPONSE_ACCEPT,
                          NULL);
  gtk_container_set_border_width (GTK_CONTAINER (self), 6);

  g_signal_connect (G_OBJECT (self), "response",
                    G_CALLBACK (_dialog_response_cb), NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_ACCEPT);
}


/* Public API */

void
frogr_details_dialog_show (GtkWindow *parent,
                           const GSList *pictures,
                           const GSList *tags)
{
  FrogrConfig *config = NULL;
  GObject *new = NULL;

  new = g_object_new (FROGR_TYPE_DETAILS_DIALOG,
                      "modal", TRUE,
                      "pictures", pictures,
                      "transient-for", parent,
                      "width-request", DIALOG_MIN_WIDTH,
                      "height-request", -1,
                      "resizable", TRUE,
                      "title", _("Edit Picture Details"),
#if GTK_CHECK_VERSION (3, 12, 0)
                      "use-header-bar", USE_HEADER_BAR,
#endif
                      "visible", TRUE,
                      NULL);

  /* Initialize values for widgets based on the data (pictures) passed */
  _fill_dialog_with_data (FROGR_DETAILS_DIALOG (new));

  /* Enable tags autocompletion if needed */
  config = frogr_config_get_instance ();
  if (config && frogr_config_get_tags_autocompletion (config))
    {
      FrogrDetailsDialog *dialog = FROGR_DETAILS_DIALOG (new);
      frogr_live_entry_set_auto_completion (FROGR_LIVE_ENTRY (dialog->tags_entry), tags);
    }
}
