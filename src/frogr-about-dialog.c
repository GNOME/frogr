/*
 * frogr-about-dialog.c -- About dialog
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

#include "frogr-about-dialog.h"

#include "frogr-util.h"

#include <config.h>
#include <glib/gi18n.h>

#define ABOUT_DIALOG_ICON ICONS_DIR "/hicolor/48x48/apps/frogr.png"

static const gchar *authors[] = {
  "Mario Sanchez Prada <msanchez@igalia.com>",
  "Adrian Perez de Castro <aperez@igalia.com>",
  "",
  NULL
};

static const gchar *artists[] = {
  "Adrian Perez de Castro <aperez@igalia.com>",
  NULL
};

static const gchar *appdescr = N_("A Flickr remote organizer for GNOME\n");
static const gchar *copyright = "(c) 2009, 2010, 2011 Mario Sanchez Prada";
static const gchar *website = "http://live.gnome.org/Frogr";
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
  "This program uses flickr API through the flicksoup\n"
  "library (which comes bundled-in with frogr), but\n"
  "it's neither approved nor certified by flickr.";
static const char translators_tpl[] =
"%s (es)\n* Alejandro Piñeiro Iglesias <apinheiro@igalia.com>\n"
         "* Mario Sanchez Prada <msanchez@igalia.com>\n\n"
"%s (en_GB)\n* Philip Withnall <philip@tecnocode.co.uk>\n\n"
"%s (fr)\n* Alexandre Franke <alexandre.franke@gmail.com>\n";

#if !GTK_CHECK_VERSION (2,23,0)
static void
_frogr_about_dialog_uri_hook (GtkAboutDialog *about,
                              const gchar *link,
                              gpointer data)
{
  gchar *uri = g_strconcat (data, link, NULL);
  frogr_util_open_url_in_browser (uri);
  g_free (uri);
}
#endif

void
frogr_about_dialog_show (GtkWindow *parent)
{
  GdkPixbuf *logo = NULL;
  char *translators = NULL;
  char *version = NULL;

  logo = gdk_pixbuf_new_from_file (ABOUT_DIALOG_ICON, NULL);

#if !GTK_CHECK_VERSION (2,23,0)
  /* Install about dialog hooks */
  gtk_about_dialog_set_url_hook (_frogr_about_dialog_uri_hook, "", NULL);
  gtk_about_dialog_set_email_hook (_frogr_about_dialog_uri_hook, "mailto:",
                                   NULL);
#endif

  /* Translated list of translators */
  translators = g_strdup_printf (translators_tpl,
                                 _("Spanish"),
                                 _("British English"),
                                 _("French"));

  /* XXX: Show clearly that this is an unreleased version */
  version = g_strdup_printf ("%s~unreleased", VERSION);

  /* Show about dialog */
  gtk_show_about_dialog (GTK_WINDOW (parent),
                         "name", PACKAGE,
                         "authors", authors,
                         "artists", artists,
                         "comments", _(appdescr),
                         "copyright", copyright,
                         "license", license,
                         "version", version,
                         "website", website,
                         "logo", logo,
                         "translator-credits", translators,
                         "modal", TRUE,
                         NULL);

  g_object_unref (logo);
  g_free (translators);
  g_free (version);
}
