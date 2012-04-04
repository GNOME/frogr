/*
 * frogr-gtk-compat.h -- Helper functions for GTK compatibility.
 *
 * Copyright (C) 2012 Mario Sanchez Prada
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

#ifndef FROGR_COMPAT_GTK_H
#define FROGR_COMPAT_GTK_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

GtkWidget *frogr_gtk_compat_box_new (GtkOrientation orientation, gint spacing);

GtkWidget *frogr_gtk_compat_separator_new (GtkOrientation orientation);

GtkWidget *frogr_gtk_compat_combo_box_text_new (void);

void frogr_gtk_compat_combo_box_text_insert (GtkComboBoxText *combo_box,
                                             gint position,
                                             const gchar *text);
G_END_DECLS

#endif /* FROGR_GTK_COMPAT_H */
