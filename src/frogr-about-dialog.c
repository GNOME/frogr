/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
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
#include "frogr-about-dialog.h"

static const gchar *authors[] = {
  "Mario Sanchez Prada\n<msanchez@igalia.com>",
  NULL
};

static const gchar *appdescr = "A Flickr remote organizer for GNOME\n";
static const gchar *copyright = "(c) 2009 Mario Sanchez Prada";
static const gchar *website = "http://frogr.igalia.com/";
static const gchar *license =
  "frogr is free software: you can redistribute\n"
  "it and/or modify it under the terms of the GNU\n"
  "General Public License version 3 as published by\n"
  "the Free Software Foundation.\n"
  "\n"
  "frogr is distributed in the hope that it will\n"
  "be useful, but WITHOUT ANY WARRANTY; without even\n"
  "the implied warranty of MERCHANTABILITY or FITNESS\n"
  "FOR A PARTICULAR PURPOSE. See the GNU General\n"
  "Public License for more details.\n"
  "\n"
  "You should have received a copy of the GNU General\n"
  "Public License along with frogr. If not, see\n"
  "http://www.gnu.org/licenses/\n\n"
  "This program uses flickr API (through libflickcurl),\n"
  "but it's neither approved nor certified by flickr.";;

static void
_frogr_about_dialog_uri_hook (GtkAboutDialog *about,
                              const gchar *link,
                              gpointer data)
{
  gchar *uri = g_strconcat (data, link, NULL);

  /* FIXME: Use gtk_show_uri with gtk 2.14 or newer */
  gnome_url_show (uri);

  g_free (uri);
}

void
frogr_about_dialog_show (GtkWindow *parent)
{
  /* Install about dialog hooks */
  gtk_about_dialog_set_url_hook (_frogr_about_dialog_uri_hook, "", NULL);
  gtk_about_dialog_set_email_hook (_frogr_about_dialog_uri_hook, "mailto:",
                                   NULL);

  /* Show about dialog */
  gtk_show_about_dialog (GTK_WINDOW (parent),
                         "name", PACKAGE, "authors", authors,
                         "comments", appdescr, "copyright", copyright,
                         "license", license, "version", "0.1",
                         /* "website", website, */
                         "modal", TRUE, NULL);
}
