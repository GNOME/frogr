/*
 * main.c -- Main file and initialization
 *
 * Copyright (C) 2009-2011 Mario Sanchez Prada
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

#include "frogr-global-defs.h"

#include <config.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libxml/parser.h>

#ifdef MAC_INTEGRATION
#include <gtkosxapplication.h>
#endif

static GSList *
_get_uris_list_from_array (char **uris_str, int n_uris)
{
  GSList *fileuris = NULL;
  int i = 0;

  for (i = 0; i < n_uris; i++)
    {
      gchar *uri = NULL;
      gchar *fileuri = NULL;

      /* Add the 'file://' schema if not present */
      if (g_str_has_prefix (uris_str[i], "/"))
        uri = g_strdup_printf ("file://%s", uris_str[i]);
      else
        uri = g_strdup (uris_str[i]);

      fileuri = g_strdup (uri);
      if (fileuri)
        fileuris = g_slist_append (fileuris, fileuri);

      g_free (uri);
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

int
main (int argc, char **argv)
{
  FrogrController *fcontroller = NULL;
  GSList *fileuris = NULL;

  /* Check optional command line parameters */
  if (argc > 1)
    fileuris = _get_uris_list_from_array (&argv[1], argc - 1);

  gtk_init (&argc, &argv);

  g_set_application_name(APP_SHORTNAME);

#ifdef MAC_INTEGRATION
  /* Initialize the GtkOSXApplication singleton */
  g_object_new (GTK_TYPE_OSX_APPLICATION, NULL);
#endif

  /* Translation domain */
  bindtextdomain (GETTEXT_PACKAGE, FROGR_LOCALE_DIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  /* Init libxml2 library */
  xmlInitParser ();

  /* Run app (and load pictures if present) */
  fcontroller = frogr_controller_get_instance ();
  if (fileuris)
    gdk_threads_add_idle (_load_pictures_on_idle, fileuris);

  frogr_controller_run_app (fcontroller);

  if (fileuris)
    {
      g_slist_foreach (fileuris, (GFunc)g_free, NULL);
      g_slist_free (fileuris);
    }

  /* cleanup libxml2 library */
  xmlCleanupParser();

  return 0;
}
