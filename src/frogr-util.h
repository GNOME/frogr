/*
 * frogr-util.h -- Misc tools.
 *
 * Copyright (C) 2010-2017 Mario Sanchez Prada
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

#ifndef FROGR_UTIL_H
#define FROGR_UTIL_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

const gchar *frogr_util_get_app_data_dir (void);

const gchar *frogr_util_get_icons_dir (void);

void frogr_util_open_uri (const gchar *uri);

void frogr_util_open_pictures_in_viewer (GSList *pictures);

void frogr_util_show_info_dialog (GtkWindow *parent, const gchar *message);

void frogr_util_show_warning_dialog (GtkWindow *parent, const gchar *message);

void frogr_util_show_error_dialog (GtkWindow *parent, const gchar *message);

GdkPixbuf *frogr_util_get_pixbuf_for_video_file (GFile *file, gint max_width, gint max_height, GError **error);

GdkPixbuf *frogr_util_get_pixbuf_from_image_contents (const guchar *contents, gsize length, gint max_width, gint max_height, const gchar *path, GError **error);

gchar *frogr_util_get_datasize_string (gulong datasize);

const gchar * const *frogr_util_get_supported_mimetypes (void);

G_END_DECLS

#endif /* FROGR_UTIL_H */
