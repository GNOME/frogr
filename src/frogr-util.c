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
#ifdef HAVE_GTK_2_14
  GError *error = NULL;

  gtk_show_uri (NULL, url, gtk_get_current_event_time (), &error);
  if (error != NULL)
    {
      g_debug ("Error opening URL %s: %s\n", url, error->message);
      g_error_free (error);
    }
#else
  gnome_url_show (url);
#endif
}
