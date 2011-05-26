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
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "frogr-global-defs.h"

#include <config.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#ifndef GTK_API_VERSION_3
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
#endif

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
frogr_util_open_url_in_browser (const gchar *url)
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
      DEBUG ("Error opening URL %s: %s", url, error->message);
      g_error_free (error);
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
