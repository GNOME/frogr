/*
 * frogr-util.c -- Misc tools.
 *
 * Copyright (C) 2010-2012 Mario Sanchez Prada
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

#include "frogr-util.h"
#include "frogr-global-defs.h"

#include <config.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libexif/exif-byte-order.h>
#include <libexif/exif-data.h>
#include <libexif/exif-entry.h>
#include <libexif/exif-format.h>
#include <libexif/exif-loader.h>
#include <libexif/exif-tag.h>

static gboolean
_spawn_command (const gchar* cmd)
{
  GError *error = NULL;

  if (!g_spawn_command_line_async (cmd, &error))
    {
      if (error)
        {
          DEBUG ("Error spawning command '%s': %s", cmd, error->message);
          g_error_free (error);
        }

      return FALSE;
    }
  return TRUE;
}

const gchar *
_get_data_dir (void)
{
#ifdef MAC_INTEGRATION
  /* For MacOSX, we return the value of the environment value set by
     the wrapper script running the application */
  static gchar *xdg_data_dir = NULL;
  if (!xdg_data_dir)
    xdg_data_dir = g_strdup (g_getenv("XDG_DATA_DIRS"));
  return (const gchar *) xdg_data_dir;
#else
  /* For GNOME, we just return DATA_DIR */
  return DATA_DIR;
#endif
}

const gchar *
frogr_util_get_app_data_dir (void)
{
  static gchar *app_data_dir = NULL;
  if (!app_data_dir)
    app_data_dir = g_strdup_printf ("%s/frogr", _get_data_dir ());

  return (const gchar *) app_data_dir;
}

const gchar *
frogr_util_get_icons_dir (void)
{
  static gchar *icons_dir = NULL;
  if (!icons_dir)
    icons_dir = g_strdup_printf ("%s/icons", _get_data_dir ());

  return (const gchar *) icons_dir;
}

const gchar *
frogr_util_get_locale_dir (void)
{
  static const gchar *locale_dir = NULL;
  if (!locale_dir)
    {
#ifndef MAC_INTEGRATION
      /* If not in MacOSX, we trust the defined variable better */
      locale_dir = g_strdup (FROGR_LOCALE_DIR);
#endif

      /* Fallback for MacOSX and cases where FROGR_LOCALE_DIR was not
	 defined yet because of any reason */
      if (!locale_dir)
	locale_dir = g_strdup_printf ("%s/locale", _get_data_dir ());
    }

  return (const gchar *) locale_dir;
}

gchar *
_get_uris_string_from_list (GList *uris_list)
{
  GList *current_uri = NULL;
  gchar **uris_array = NULL;
  gchar *uris_str = NULL;
  gint n_uris = 0;
  gint i = 0;

  n_uris = g_list_length (uris_list);
  if (n_uris == 0)
    return NULL;

  uris_array = g_new0 (gchar*, n_uris + 1);
  for (current_uri = uris_list; current_uri; current_uri = g_list_next (current_uri))
    uris_array[i++] = (gchar *) (current_uri->data);

  uris_str = g_strjoinv (" ", uris_array);
  g_free (uris_array);

  return uris_str;
}

static void
_open_uris_with_app_info (GList *uris_list, GAppInfo *app_info)
{
  GError *error = NULL;

  /* Early return */
  if (!uris_list)
    return;

  if (!app_info || !g_app_info_launch_uris (app_info, uris_list, NULL, &error))
    {
      /* The default app didn't succeed, so try 'gnome-open' / 'open' */
      gchar *command = NULL;
      gchar *uris = NULL;

      uris = _get_uris_string_from_list (uris_list);

#ifdef MAC_INTEGRATION
      /* In MacOSX use 'open' instead of 'gnome-open' */
      command = g_strdup_printf ("open %s", uris);
#else
      command = g_strdup_printf ("gnome-open %s", uris);
#endif
      _spawn_command (command);

      if (error)
        {
          DEBUG ("Error opening URI(s) %s: %s", uris, error->message);
          g_error_free (error);
        }

      g_free (command);
      g_free (uris);
    }

  g_list_foreach (uris_list, (GFunc) g_free, NULL);
  g_list_free (uris_list);
}

void
frogr_util_open_uri (const gchar *uri)
{
  GAppInfo *app_info = NULL;
  GList *uris_list = NULL;

  /* Early return */
  if (!uri)
    return;

#ifndef MAC_INTEGRATION
  /* Supported network URIs */
  if (g_str_has_prefix (uri, "http:") || g_str_has_prefix (uri, "https:"))
    app_info = g_app_info_get_default_for_uri_scheme ("http");

  /* Supported help URIs */
  if (g_str_has_prefix (uri, "ghelp:"))
    app_info = g_app_info_get_default_for_uri_scheme ("ghelp");
#endif

  uris_list = g_list_append (uris_list, g_strdup (uri));
  _open_uris_with_app_info (uris_list, app_info);
}

void
frogr_util_open_images_in_viewer (GList *uris_list)
{
  GAppInfo *app_info = NULL;

  /* Early return */
  if (!uris_list)
    return;

  app_info = g_app_info_get_default_for_type ("image/jpg", TRUE);
  _open_uris_with_app_info (uris_list, app_info);
}

static void
_show_message_dialog (GtkWindow *parent, const gchar *message, GtkMessageType type)
{
  /* Show alert */
  GtkWidget *dialog =
    gtk_message_dialog_new (parent,
                            GTK_DIALOG_MODAL,
                            type,
                            GTK_BUTTONS_CLOSE,
                            "%s", message);
  gtk_window_set_title (GTK_WINDOW (dialog), APP_SHORTNAME);

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (gtk_widget_destroy), dialog);

  gtk_widget_show_all (dialog);
}

void
frogr_util_show_info_dialog (GtkWindow *parent, const gchar *message)
{
  _show_message_dialog (parent, message, GTK_MESSAGE_INFO);
}

void
frogr_util_show_warning_dialog (GtkWindow *parent, const gchar *message)
{
  _show_message_dialog (parent, message, GTK_MESSAGE_WARNING);
}

void
frogr_util_show_error_dialog (GtkWindow *parent, const gchar *message)
{
  _show_message_dialog (parent, message, GTK_MESSAGE_ERROR);
}

GdkPixbuf *
frogr_util_get_corrected_pixbuf (GdkPixbuf *pixbuf, gint max_width, gint max_height)
{
  GdkPixbuf *scaled_pixbuf = NULL;
  GdkPixbuf *rotated_pixbuf;
  const gchar *orientation;
  gint width;
  gint height;

  g_return_val_if_fail (max_width > 0, NULL);
  g_return_val_if_fail (max_height > 0, NULL);

  /* Look for the right side to reduce */
  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  DEBUG ("Original size: %dx%d\n", width, height);

  if (width > max_width)
    {
      height = (float)height * max_width / width;
      width = max_width;
    }

  if (height > max_height)
    {
      width = (float)width * max_height / height;
      height = max_height;
    }

  DEBUG ("Scaled size: %dx%d\n", width, height);

  /* Scale the pixbuf to its best size */
  scaled_pixbuf = gdk_pixbuf_scale_simple (pixbuf, width, height,
                                           GDK_INTERP_BILINEAR);

  /* Correct orientation if needed */
  orientation = gdk_pixbuf_get_option (pixbuf, "orientation");

  /* No orientation defined or 0 degrees rotation: we're done */
  if (!orientation || !g_strcmp0 (orientation, "1"))
    return scaled_pixbuf;

  DEBUG ("File orientation for file: %s", orientation);
  rotated_pixbuf = NULL;

  /* Rotated 90 degrees */
  if (!g_strcmp0 (orientation, "8"))
    rotated_pixbuf = gdk_pixbuf_rotate_simple (scaled_pixbuf,
                                               GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
  /* Rotated 180 degrees */
  if (!g_strcmp0 (orientation, "3"))
    rotated_pixbuf = gdk_pixbuf_rotate_simple (scaled_pixbuf,
                                               GDK_PIXBUF_ROTATE_UPSIDEDOWN);
  /* Rotated 270 degrees */
  if (!g_strcmp0 (orientation, "6"))
    rotated_pixbuf = gdk_pixbuf_rotate_simple (scaled_pixbuf,
                                               GDK_PIXBUF_ROTATE_CLOCKWISE);
  if (rotated_pixbuf)
    {
      g_object_unref (scaled_pixbuf);
      return rotated_pixbuf;
    }

  /* No rotation was applied, return the scaled pixbuf */
  return scaled_pixbuf;
}

gchar *
frogr_util_get_datasize_string (gulong datasize)
{
  gchar *result = NULL;

  if (datasize != G_MAXULONG)
    {
      gfloat datasize_float = G_MAXFLOAT;
      gchar *unit_str = NULL;
      int n_divisions = 0;

      datasize_float = datasize;
      while (datasize_float > 1000.0 && n_divisions < 3)
        {
          datasize_float /= 1024;
          n_divisions++;
        }

      switch (n_divisions)
        {
        case 0:
          unit_str = g_strdup ("KB");
          break;
        case 1:
          unit_str = g_strdup ("MB");
          break;
        case 2:
          unit_str = g_strdup ("GB");
          break;
        default:
          unit_str = NULL;;
        }

      if (unit_str)
        {
          result = g_strdup_printf ("%.1f %s", datasize_float, unit_str);
          g_free (unit_str);
        }
    }

  return result;
}
