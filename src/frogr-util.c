/*
 * frogr-util.c -- Misc tools.
 *
 * Copyright (C) 2010-2011 Mario Sanchez Prada
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>
 *
 */

#include "frogr-global-defs.h"

#include <config.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>

static gboolean
_spawn_command (const gchar* cmd)
{
  GError *error = NULL;

  if (!g_spawn_command_line_async (cmd, &error))
    {
      if (error != NULL)
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

void
frogr_util_open_uri (const gchar *url)
{
  gchar *command = NULL;
  GError *error = NULL;

  /* Early return */
  if (url == NULL)
    return;

#ifdef MAC_INTEGRATION
  /* In MacOSX neither gnome-open nor gtk_show_uri() will work */
  command = g_strdup_printf ("open %s", url);
  _spawn_command (command);
#else
#ifdef GTK_API_VERSION_3
  /* For GTK3 we dare to do it The Right Way (tm). */
  gtk_show_uri (NULL, url, GDK_CURRENT_TIME, &error);
#else
  /* I found some weird behaviours using gtk_show_uri() in GTK2, so
     that's why we just use the gnome-open command instead.  If
     gnome-open fails, then we fallback to gtk_show_uri(). */
  command = g_strdup_printf ("gnome-open %s", url);
  if (!_spawn_command (command))
    gtk_show_uri (NULL, url, GDK_CURRENT_TIME, &error);
#endif /* ifdef GTK_API_VERSION_3 */
#endif /* ifdef MAC_INTEGRATION */

  if (command)
    g_free (command);

  if (error != NULL)
    {
      DEBUG ("Error opening URI %s: %s", url, error->message);
      g_error_free (error);
    }
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
  g_strfreev (uris_array);

  return uris_str;
}

void
frogr_util_open_images_in_viewer (GList *uris_list)
{
  static GAppInfo *app_info = NULL;

  /* Early return */
  if (uris_list == NULL)
    return;

  if (!app_info)
    app_info = g_app_info_get_default_for_type ("image/jpg", TRUE);

  if (!g_app_info_launch_uris (app_info, uris_list, NULL, NULL))
    {
      gchar *command = NULL;
      gchar *uris = NULL;

      /* The default app didn't succeed, so try 'gnome-open' */
      uris = _get_uris_string_from_list (uris_list);

#ifdef MAC_INTEGRATION
      /* In MacOSX neither gnome-open nor gtk_show_uri() will work */
      command = g_strdup_printf ("open %s", uris);
#else
      command = g_strdup_printf ("gnome-open %s", uris);
#endif
      _spawn_command (command);

      g_free (command);
      g_free (uris);
    }
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
