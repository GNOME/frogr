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
#ifdef HAVE_GSTREAMER
#include <gst/gst.h>
#endif
#include <libxml/parser.h>

int
main (int argc, char **argv)
{
  FrogrController *controller = NULL;
  int status;
#ifdef HAVE_GSTREAMER
  GError *error = NULL;

  /* Initialize gstreamer before using any other GLib function */
  gst_init_check (&argc, &argv, &error);
  if (error)
    {
      DEBUG ("Gstreamer could not be initialized: %s", error->message);
      g_error_free (error);
    }
#endif

  /* Initialize libxml2 library */
  xmlInitParser ();

  /* Initialize internationalization */
  bindtextdomain (GETTEXT_PACKAGE, frogr_util_get_locale_dir ());
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  controller = frogr_controller_get_instance ();
  status = frogr_controller_run_app (controller, argc, argv);
  g_object_unref (controller);

  /* cleanup libxml2 library */
  xmlCleanupParser();

  return status;
}
