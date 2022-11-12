/*
 * main.c -- Main file and initialization
 *
 * Copyright (C) 2009-2017 Mario Sanchez Prada
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
#if HAVE_GSTREAMER
#include <gst/gst.h>
#endif
#include <libxml/parser.h>
#include <errno.h>
#include <gcrypt.h>
#include <pthread.h>

#if GCRYPT_VERSION_NUMBER < 0x010600
GCRY_THREAD_OPTION_PTHREAD_IMPL;
#endif

int
main (int argc, char **argv)
{
  g_autoptr(FrogrController) controller = NULL;
  int status;
#if HAVE_GSTREAMER
  g_autoptr(GError) error = NULL;

  /* Initialize gstreamer before using any other GLib function */
  gst_init_check (&argc, &argv, &error);
  if (error)
    {
      DEBUG ("Gstreamer could not be initialized: %s", error->message);
    }
#endif

#if GCRYPT_VERSION_NUMBER < 0x010600
  /* Initialize gcrypt at the very beginning */
  gcry_control (GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread);
#endif

  /* Version check should be almost the very first call because it
     makes sure that important subsystems are initialized. */
  g_assert (gcry_check_version (LIBGCRYPT_MIN_VERSION));
  /* Allocate a pool of 16k secure memory.  This make the secure
     memory available and also drops privileges where needed. */
  gcry_control (GCRYCTL_INIT_SECMEM, 16384, 0);
  /* Tell Libgcrypt that initialization has completed. */
  gcry_control (GCRYCTL_INITIALIZATION_FINISHED, 0);

  /* Initialize libxml2 library */
  xmlInitParser ();

  /* Initialize internationalization */
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  controller = frogr_controller_get_instance ();
  status = frogr_controller_run_app (controller, argc, argv);

  /* cleanup libxml2 library */
  xmlCleanupParser();

  return status;
}
