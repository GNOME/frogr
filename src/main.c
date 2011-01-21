/*
 * main.c -- Main file and initialization
 *
 * Copyright (C) 2009, 2010, 2011 Mario Sanchez Prada
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

#include "frogr-controller.h"

#include <config.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libxml/parser.h>

static GSList *
_get_paths_list_from_array (char **paths_str, int n_paths)
{
  GSList *filepaths = NULL;
  GError *err = NULL;
  int i = 0;

  for (i = 0; i < n_paths; i++)
    {
      gchar *uri = NULL;
      gchar *filepath = NULL;

      /* Add the 'file://' schema if not present */
      if (g_str_has_prefix (paths_str[i], "/"))
        uri = g_strdup_printf ("file://%s", paths_str[i]);
      else
        uri = g_strdup (paths_str[i]);

      filepath = g_filename_from_uri (uri, NULL, &err);
      if (err)
        {
          g_debug ("Error loading picture %s: %s\n", uri, err->message);
          g_error_free (err);
          err = NULL;
        }
      else
        filepaths = g_slist_append (filepaths, filepath);

      g_free (uri);
    }

  return filepaths;
}

static gboolean
_load_pictures_on_idle (gpointer data)
{
  g_return_val_if_fail (data, FALSE);

  FrogrController *fcontroller = NULL;
  GSList *filepaths = NULL;

  fcontroller = frogr_controller_get_instance ();
  filepaths = (GSList *)data;

  frogr_controller_load_pictures (fcontroller, filepaths);

  return FALSE;
}

int
main (int argc, char **argv)
{
  FrogrController *fcontroller = NULL;
  GSList *filepaths = NULL;

  /* Check optional command line parameters */
  if (argc > 1)
    filepaths = _get_paths_list_from_array (&argv[1], argc - 1);

  gtk_init (&argc, &argv);
  g_set_application_name(PACKAGE);

  /* Translation domain */
  bindtextdomain (GETTEXT_PACKAGE, FROGR_LOCALE_DIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  /* Init libxml2 library */
  xmlInitParser ();

  /* Run app (and load pictures if present) */
  fcontroller = frogr_controller_get_instance ();
  if (filepaths)
    gdk_threads_add_idle (_load_pictures_on_idle, filepaths);

  frogr_controller_run_app (fcontroller);

  if (filepaths)
    {
      g_slist_foreach (filepaths, (GFunc)g_free, NULL);
      g_slist_free (filepaths);
    }

  /* cleanup libxml2 library */
  xmlCleanupParser();

  return 0;
}
