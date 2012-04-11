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
#include <gdk/gdkkeysyms.h>

/* Compatibility with old GDK symbols for GTK+ 2 */
#ifndef GDK_KEY_Menu
#        define GDK_KEY_Menu GDK_Menu
#        define GDK_KEY_Delete GDK_Delete
#        define GDK_KEY_F1 GDK_F1
#        define GDK_KEY_a GDK_a
#        define GDK_KEY_d GDK_d
#        define GDK_KEY_g GDK_g
#        define GDK_KEY_l GDK_l
#        define GDK_KEY_p GDK_p
#        define GDK_KEY_q GDK_q
#        define GDK_KEY_r GDK_r
#        define GDK_KEY_s GDK_s
#        define GDK_KEY_t GDK_t
#        define GDK_KEY_u GDK_u
#        define GDK_KEY_v GDK_v
#endif

G_BEGIN_DECLS

GtkWidget *frogr_gtk_compat_box_new (GtkOrientation orientation, gint spacing);

GtkWidget *frogr_gtk_compat_separator_new (GtkOrientation orientation);

GtkWidget *frogr_gtk_compat_combo_box_text_new (void);

void frogr_gtk_compat_combo_box_text_insert (GtkComboBox *combo_box,
                                             gint position,
                                             const gchar *text);
G_END_DECLS

#endif /* FROGR_GTK_COMPAT_H */
