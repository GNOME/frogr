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

#define PICTURE_WIDTH 100
#define PICTURE_HEIGHT 100

void
frogr_details_dialog_show (GtkWindow *parent, FrogrPicture *fpicture)
{
  GtkBuilder *builder;
  GtkWidget *dialog;
  GtkWidget *title_entry;
  GtkWidget *desc_tview;
  GtkWidget *tags_entry;
  GtkWidget *picture_img;
  GtkTextBuffer *text_buffer;
  GdkPixbuf *pixbuf, *scaled_pixbuf;
  gchar *filepath = NULL;
  gchar *title = NULL;
  gchar *description = NULL;
  gchar *tags = NULL;
  gint width, new_width;
  gint height, new_height;
  gint response;

  /* Create widgets */
  builder = gtk_builder_new ();
  gtk_builder_add_from_file (builder, GTKBUILDER_FILE, NULL);

  /* Get widgets */
  dialog = GTK_WIDGET (gtk_builder_get_object (builder, "details-dialog"));
  title_entry = GTK_WIDGET (gtk_builder_get_object (builder, "title_entry"));
  desc_tview = GTK_WIDGET (gtk_builder_get_object (builder, "description_tview"));
  tags_entry = GTK_WIDGET (gtk_builder_get_object (builder, "tags_entry"));
  picture_img = GTK_WIDGET (gtk_builder_get_object (builder, "picture_img"));
  text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (desc_tview));

  /* Fill in with data */
  filepath = (gchar *)frogr_picture_get_filepath (fpicture);
  title = (gchar *)frogr_picture_get_title (fpicture);
  description = (gchar *)frogr_picture_get_description (fpicture);
  tags = (gchar *)frogr_picture_get_tags (fpicture);

  if (title != NULL)
    gtk_entry_set_text (GTK_ENTRY (title_entry), title);

  if (description != NULL)
    gtk_text_buffer_set_text (GTK_TEXT_BUFFER (text_buffer), description, -1);

  if (tags != NULL)
    gtk_entry_set_text (GTK_ENTRY (tags_entry), tags);

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

  scaled_pixbuf = gdk_pixbuf_scale_simple (pixbuf,
                                           new_width, new_height,
                                           GDK_INTERP_TILES);
  g_object_unref (pixbuf);

  gtk_image_set_from_pixbuf (GTK_IMAGE (picture_img), scaled_pixbuf);
  g_object_unref (scaled_pixbuf);

  /* Show the dialog */
  gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);
  gtk_widget_show_all (dialog);

  /* Run the dialog */
   /* XXX: Actually implement this as needed, now it's just a placeholder */
  response = gtk_dialog_run (GTK_DIALOG (dialog));
  if (response == GTK_RESPONSE_OK)
    {
      GtkTextIter start;
      GtkTextIter end;

      /* Save data */
      title = (gchar *)gtk_entry_get_text (GTK_ENTRY (title_entry));
      tags = (gchar *)gtk_entry_get_text (GTK_ENTRY (tags_entry));

      gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (text_buffer), &start, &end);
      description = (gchar *)gtk_text_buffer_get_text (GTK_TEXT_BUFFER (text_buffer),
                                                       &start, &end, FALSE);
      if (title != NULL)
          frogr_picture_set_title (fpicture, title);

      if (description != NULL)
        frogr_picture_set_description (fpicture, description);

      if (tags != NULL)
        frogr_picture_set_tags (fpicture, tags);
    }

  /* Destroy the dialog */
  gtk_widget_destroy (GTK_WIDGET (dialog));

  /* Free memory */
  g_object_unref (fpicture);
}
