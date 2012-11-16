/*
 * main.c -- Main file and initialization
 *
 * Copyright (C) 2009-2012 Mario Sanchez Prada
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

#include "frogr-controller.h"

#include "frogr-global-defs.h"
#include "frogr-util.h"

#include <config.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gst/gst.h>
#include <libxml/parser.h>

static GSList *
_get_files_list_from_array (GFile **files, int n_files)
{
  GSList *fileuris = NULL;
  int i = 0;

  for (i = 0; i < n_files; i++)
    {
      gchar *fileuri = NULL;

      fileuri = g_strdup (g_file_get_uri (files[i]));
      if (fileuri)
        fileuris = g_slist_append (fileuris, fileuri);
    }

  return fileuris;
}

static gboolean
_load_pictures_on_idle (gpointer data)
{
  FrogrController *fcontroller = NULL;
  GSList *fileuris = NULL;

  g_return_val_if_fail (data, FALSE);

  fcontroller = frogr_controller_get_instance ();
  fileuris = (GSList *)data;

  frogr_controller_load_pictures (fcontroller, fileuris);
  return FALSE;
}

static void
_app_startup_cb (GApplication *app, gpointer data)
{
  FrogrController *fcontroller = NULL;

  DEBUG ("Started!\n");

  /* Initialize libxml2 library */
  xmlInitParser ();

  /* Initialize internationalization */
  bindtextdomain (GETTEXT_PACKAGE, frogr_util_get_locale_dir ());
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  /* Actually start the application */
  fcontroller = frogr_controller_get_instance ();
  frogr_controller_run_app (fcontroller, GTK_APPLICATION (app));
}

static void
_app_activate_cb (GApplication *app, gpointer data)
{
  DEBUG ("Activated!\n");
}

static void
_app_shutdown_cb (GApplication *app, gpointer data)
{
  DEBUG ("%s", "Shutting down application...");

  /* cleanup libxml2 library */
  xmlCleanupParser();
}

static void
_app_open_files_cb (GApplication *app, GFile **files, gint n_files, gchar *hint, gpointer data)
{
  DEBUG ("Trying to open %d files\n", n_files);

  GSList *fileuris = _get_files_list_from_array (files, n_files);
  gdk_threads_add_idle (_load_pictures_on_idle, fileuris);
}

int
main (int argc, char **argv)
{
  GtkApplication *app = NULL;
  GError *error = NULL;
  int status;

  /* Initialize gstreamer before using any other GLib function */
  gst_init_check (&argc, &argv, &error);
  if (error)
    {
      /* TODO: Disable video support when this happens */
      DEBUG ("Gstreamer could not be initialized: %s", error->message);
      g_error_free (error);
    }

  /* Initialize and run the Gtk application */
  g_set_application_name(APP_SHORTNAME);
  app = gtk_application_new (APP_ID,
                             G_APPLICATION_NON_UNIQUE
                             | G_APPLICATION_HANDLES_OPEN);

  g_signal_connect (app, "activate", G_CALLBACK (_app_activate_cb), NULL);
  g_signal_connect (app, "startup", G_CALLBACK (_app_startup_cb), NULL);
  g_signal_connect (app, "shutdown", G_CALLBACK (_app_shutdown_cb), NULL);
  g_signal_connect (app, "open", G_CALLBACK (_app_open_files_cb), NULL);

  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}
