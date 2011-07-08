/*
 * frogr-util.h -- Misc tools.
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

#ifndef FROGR_UTIL_H
#define FROGR_UTIL_H

G_BEGIN_DECLS

const gchar *frogr_util_get_app_data_dir (void);

const gchar *frogr_util_get_icons_dir (void);

const gchar *frogr_util_get_locale_dir (void);

void frogr_util_open_uri (const gchar *url);

void frogr_util_open_images_in_viewer (GList *uris_list);

void frogr_util_show_info_dialog (GtkWindow *parent, const gchar *message);

void frogr_util_show_warning_dialog (GtkWindow *parent, const gchar *message);

void frogr_util_show_error_dialog (GtkWindow *parent, const gchar *message);

G_END_DECLS

#endif /* FROGR_UTIL_H */
