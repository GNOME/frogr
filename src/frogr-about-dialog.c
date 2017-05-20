/*
 * frogr-about-dialog.c -- About dialog
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

#include "frogr-about-dialog.h"

#include "frogr-global-defs.h"
#include "frogr-util.h"

#include <config.h>
#include <glib/gi18n.h>

/* Path relative to the icons dir */
#define ABOUT_DIALOG_ICON "/hicolor/48x48/apps/" APP_ID ".png"

static const gchar *authors[] = {
  "Mario Sanchez Prada <msanchez@gnome.org>",
  "",
  NULL
};

static const gchar *artists[] = {
  "Adrian Perez de Castro <aperez@igalia.com>",
  NULL
};

static const gchar *appdescr = N_("A Flickr Remote Organizer for GNOME\n");
static const gchar *copyright = "(c) 2009-2015 Mario Sanchez Prada";
static const gchar *website = "http://wiki.gnome.org/Apps/Frogr";

void
frogr_about_dialog_show (GtkWindow *parent)
{
  GdkPixbuf *logo = NULL;
  gchar *version = NULL;
  gchar *icon_full_path = NULL;

  icon_full_path = g_strdup_printf ("%s/" ABOUT_DIALOG_ICON,
				    frogr_util_get_icons_dir ());
  logo = gdk_pixbuf_new_from_file (icon_full_path, NULL);
  g_free (icon_full_path);

  version = g_strdup_printf ("%s~unreleased", VERSION);

  /* Show about dialog */
  gtk_show_about_dialog (GTK_WINDOW (parent),
                         "name", PACKAGE,
                         "authors", authors,
                         "artists", artists,
                         "comments", _(appdescr),
                         "copyright", copyright,
                         "license-type", GTK_LICENSE_GPL_3_0,
                         "version", version,
                         "website", website,
                         "logo", logo,
                         "translator-credits", _("translator-credits"),
                         "modal", TRUE,
                         NULL);

  g_object_unref (logo);
  g_free (version);
}
