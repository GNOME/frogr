/*
 * frogr-gtk-compat.c -- Helper functions for GTK compatibility.
 *
 * Copyright (C) 2012 Mario Sanchez Prada
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

#include "frogr-gtk-compat.h"
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

GtkWidget *
frogr_gtk_compat_box_new (GtkOrientation orientation, gint spacing)
{
#ifdef GTK_API_VERSION_3
  GtkWidget *box = gtk_box_new (orientation, spacing);
  gtk_box_set_homogeneous (GTK_BOX (box), FALSE);
  return box;
#else
  if (orientation == GTK_ORIENTATION_VERTICAL)
    return gtk_vbox_new (FALSE, spacing);
  else
    return gtk_hbox_new (FALSE, spacing);
#endif
}

GtkWidget *
frogr_gtk_compat_separator_new (GtkOrientation orientation)
{
#ifdef GTK_API_VERSION_3
  return gtk_separator_new (orientation);
#else
  if (orientation == GTK_ORIENTATION_VERTICAL)
    return gtk_vseparator_new ();
  else
    return gtk_hseparator_new ();
#endif
}

GtkWidget *
frogr_gtk_compat_combo_box_text_new (void)
{
#if GTK_CHECK_VERSION (2,24,0)
  return gtk_combo_box_text_new ();
#else
  return gtk_combo_box_new_text ();
#endif
}

void
frogr_gtk_compat_combo_box_text_insert (GtkComboBox *combo_box,
                                        gint position,
                                        const gchar *text)
{
#ifdef GTK_API_VERSION_3
  gtk_combo_box_text_insert (GTK_COMBO_BOX_TEXT (combo_box), position, NULL, text);
#elif GTK_CHECK_VERSION (2,24,0)
  gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (combo_box), position, text);
#else
  gtk_combo_box_insert_text (combo_box, position, text);
#endif
}
