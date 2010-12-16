/*
 * frogr-auth-dialog.c -- Authorization dialog
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

#include "frogr-auth-dialog.h"

#include "frogr-controller.h"

#include <config.h>
#include <glib/gi18n.h>

static gchar *unauth_txt =
  N_("You have not properly authorized %s in flickr.\n"
     "Please press the button to authorize %s "
     "and then come back to this screen to complete the process.");

static gchar *auth_txt =
  N_("Press the button to start using %s once you've "
     "authorized it in your flickr account.");

/* Prototypes */

static void _ask_for_authorization (GtkWindow *parent);

static void _ask_for_authorization_response_cb (GtkDialog *dialog, gint response, gpointer data);

static void _ask_for_auth_confirmation (GtkWindow *parent);

static void _ask_for_auth_confirmation_response_cb (GtkDialog *dialog, gint response, gpointer data);

/* Private API */

static void
_ask_for_authorization (GtkWindow *parent)
{
  GtkWidget *dialog = NULL;
  dialog = gtk_message_dialog_new (parent,
                                   GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_INFO,
                                   GTK_BUTTONS_OK,
                                   _(unauth_txt),
                                   PACKAGE_NAME,
                                   PACKAGE_NAME);

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (_ask_for_authorization_response_cb), NULL);

  gtk_widget_show_all (dialog);
}

static void
_ask_for_authorization_response_cb (GtkDialog *dialog, gint response, gpointer data)
{
  if (response == GTK_RESPONSE_OK)
    {
      FrogrController *controller = frogr_controller_get_instance();
      frogr_controller_open_auth_url (controller);
      g_object_unref (controller);
    }

  gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
_ask_for_auth_confirmation (GtkWindow *parent)
{
  GtkWidget *dialog = NULL;
  dialog = gtk_message_dialog_new (parent,
                                   GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_INFO,
                                   GTK_BUTTONS_OK,
                                   _(auth_txt),
                                   PACKAGE_NAME);

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (_ask_for_auth_confirmation_response_cb), NULL);

  gtk_widget_show_all (dialog);
}

static void
_ask_for_auth_confirmation_response_cb (GtkDialog *dialog, gint response, gpointer data)
{
  if (response == GTK_RESPONSE_OK)
    {
      FrogrController *controller = frogr_controller_get_instance();
      frogr_controller_complete_auth (controller);
      g_object_unref (controller);
    }

  gtk_widget_destroy (GTK_WIDGET (dialog));
}

/* Public API */

void
frogr_auth_dialog_show (GtkWindow *parent, FrogrAuthDialogStep step)
{
  switch (step) {
  case REQUEST_AUTHORIZATION:
    _ask_for_authorization (parent);
    break;
  case CONFIRM_AUTHORIZATION:
    _ask_for_auth_confirmation (parent);
    break;
  default:
    g_assert_not_reached();
  }
}
