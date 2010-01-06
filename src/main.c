/*
 * main.c -- Main file and initialization
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
#include <gtk/gtk.h>

#include "frogr-controller.h"

int
main (int argc, char **argv)
{
  FrogrController *fcontroller = NULL;

  /* Init threads system */
  g_thread_init (NULL);
  gdk_threads_init ();

  gdk_threads_enter ();
  gtk_init (&argc, &argv);
  g_set_application_name(PACKAGE);

  /* Translation domain */
  bindtextdomain (GETTEXT_PACKAGE, FROGR_LOCALE_DIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  /* Run app */
  fcontroller = frogr_controller_get_instance ();
  frogr_controller_run_app (fcontroller);

  gdk_threads_leave ();

  return 0;
}
