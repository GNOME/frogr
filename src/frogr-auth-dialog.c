/*
 * frogr-auth-dialog.c -- Authorization dialog
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
#include "frogr-auth-dialog.h"
#include "frogr-controller.h"

static gchar *unauth_text =
  "Please press in the 'Continue' button to authorize " PACKAGE_NAME " "
  "in your flickr account and then come back to this screen to "
  "complete the process.";

static gchar *auth_text =
  "Press on 'Complete' to start using " PACKAGE_NAME " once you've "
  "authorized it in your flickr account.";

void
frogr_auth_dialog_show (GtkWindow *parent)
{
  FrogrController *controller = frogr_controller_get_instance ();
  if (!frogr_controller_is_authorized (controller))
    {
      GtkWidget *dialog;
      GtkWidget *vbox;
      GtkWidget *info_label;
      GtkWidget *button;
      gboolean authorizing = FALSE;
      gboolean authorized = FALSE;
      gint response = 0;

      dialog = gtk_dialog_new ();
      gtk_window_set_title (GTK_WINDOW (dialog), "Authorize frogr");
      gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
      gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
      gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (parent));
      gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

      /* Add buttons */
      button = gtk_dialog_add_button (GTK_DIALOG (dialog),
                                      "Continue",
                                      GTK_RESPONSE_ACCEPT);
      /* Add labels */
      info_label = gtk_label_new (unauth_text);
      gtk_label_set_line_wrap (GTK_LABEL (info_label), TRUE);

      vbox = GTK_DIALOG (dialog) -> vbox;
      gtk_box_pack_start (GTK_BOX (vbox), info_label, FALSE, FALSE, 6);

      /* Show the dialog */
      gtk_widget_show_all (dialog);
      do
        {
          response = gtk_dialog_run (GTK_DIALOG (dialog));
          if (response == GTK_RESPONSE_ACCEPT && !authorizing)
            {
              frogr_controller_open_authorization_url (controller);
              authorizing = TRUE;

              gtk_button_set_label (GTK_BUTTON (button), "Complete");
              gtk_label_set_text (GTK_LABEL (info_label), auth_text);
            }
          else if (response == GTK_RESPONSE_ACCEPT)
            {
              if (frogr_controller_complete_authorization (controller))
                {
                  /* Everything went fine */
                  authorized = TRUE;
                  g_debug ("Authorization successfully completed!");
                }
              else
                {
                  /* An error happened */
                  GtkWidget *msg_dialog;
                  msg_dialog = gtk_message_dialog_new (GTK_WINDOW (dialog),
                                                       GTK_DIALOG_MODAL,
                                                       GTK_MESSAGE_ERROR,
                                                       GTK_BUTTONS_OK,
                                                       "Authorization failed.\n"
                                                       "Please try again",
                                                       NULL);
                  /* Show and destroy a an error message */
                  gtk_dialog_run (GTK_DIALOG (msg_dialog));
                  gtk_widget_destroy (msg_dialog);

                  /* Restore values if reached */
                  authorizing = FALSE;
                  authorized = FALSE;
                  gtk_button_set_label (GTK_BUTTON (button), "Continue");
                  gtk_label_set_text (GTK_LABEL (info_label), unauth_text);
                  g_debug ("Authorization not completed");
                }
            }

        } while (!authorized && (response == GTK_RESPONSE_ACCEPT));

      /* Destroy the dialog */
      gtk_widget_destroy (GTK_WIDGET (dialog));
    }

  /* Release controller */
  g_object_unref (controller);
}
