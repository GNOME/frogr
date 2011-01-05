/*
 * frogr-util.c -- Misc tools.
 *
 * Copyright (C) 2010 Mario Sanchez Prada
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

#include <glib/gi18n.h>
#include <gtk/gtk.h>

void
frogr_util_open_url_in_browser (const gchar *url)
{
  if (url == NULL)
    return;

#if GTK_CHECK_VERSION (2,14,0)
  gchar *command = NULL;
  GError *error = NULL;

  /* FIXME: Replace this lines with gtk_show_uri() when we found the
     reason behind frogr hanging after calling twice that function */
  command = g_strdup_printf ("gnome-open %s", url);
  g_spawn_command_line_async (command, &error);
  g_free (command);

  if (error != NULL)
    {
      g_debug ("Error opening URL %s: %s", url, error->message);
      g_error_free (error);
    }
#else
  gnome_url_show (url);
#endif
}

static void
frogr_util_show_message_dialog (GtkWindow *parent, const gchar *message, GtkMessageType type)
{
  /* Show alert */
  GtkWidget *dialog =
    gtk_message_dialog_new (parent,
                            GTK_DIALOG_MODAL,
                            type,
                            GTK_BUTTONS_CLOSE,
                            "%s", message);
  gtk_window_set_title (GTK_WINDOW (dialog), PACKAGE);

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (gtk_widget_destroy), dialog);

  gtk_widget_show_all (dialog);
}

void
frogr_util_show_info_dialog (GtkWindow *parent, const gchar *message)
{
  frogr_util_show_message_dialog (parent, message, GTK_MESSAGE_INFO);
}

void
frogr_util_show_warning_dialog (GtkWindow *parent, const gchar *message)
{
  frogr_util_show_message_dialog (parent, message, GTK_MESSAGE_WARNING);
}

void
frogr_util_show_error_dialog (GtkWindow *parent, const gchar *message)
{
  frogr_util_show_message_dialog (parent, message, GTK_MESSAGE_ERROR);
}
