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
  gchar *title = NULL;
  gchar *description = NULL;
  gchar *tags = NULL;
  gint response;

  /* Create widgets */
  builder = gtk_builder_new ();
  gtk_builder_add_from_file (builder, GTKBUILDER_FILE, NULL);

  /* Get widgets */
  dialog = GTK_WIDGET (gtk_builder_get_object (builder, "details-dialog"));
  title_entry = GTK_WIDGET (gtk_builder_get_object (builder, "title_entry"));
  desc_tview = GTK_WIDGET (gtk_builder_get_object (builder, "description_tview"));
  tags_entry = GTK_WIDGET (gtk_builder_get_object (builder, "tags_entry"));
  text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (desc_tview));

  /* Fill in with data */
  title = (gchar *)frogr_picture_get_title (fpicture);
  description = (gchar *)frogr_picture_get_description (fpicture);
  tags = (gchar *)frogr_picture_get_tags_string (fpicture);

  if (title != NULL)
    gtk_entry_set_text (GTK_ENTRY (title_entry), title);

  if (description != NULL)
    gtk_text_buffer_set_text (GTK_TEXT_BUFFER (text_buffer), description, -1);

  if (tags != NULL)
    gtk_entry_set_text (GTK_ENTRY (tags_entry), tags);

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

  /* Free */
  g_object_unref (fpicture);
}
