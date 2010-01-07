/*
 * frogr-main-window-gtk.h -- Gtk main window for the application
 *
 * Copyright (C) 2009 Mario Sanchez Prada
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

#ifndef FROGR_MAIN_WINDOW_GTK_H
#define FROGR_MAIN_WINDOW_GTK_H

#include <gtk/gtk.h>
#include "frogr-main-window.h"

G_BEGIN_DECLS

#define FROGR_TYPE_MAIN_WINDOW_GTK           (frogr_main_window_gtk_get_type())
#define FROGR_MAIN_WINDOW_GTK(obj)           (G_TYPE_CHECK_INSTANCE_CAST(obj, FROGR_TYPE_MAIN_WINDOW_GTK, FrogrMainWindowGtk))
#define FROGR_MAIN_WINDOW_GTK_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST(klass, FROGR_TYPE_MAIN_WINDOW_GTK, FrogrMainWindowGtkClass))
#define FROGR_IS_MAIN_WINDOW_GTK(obj)           (G_TYPE_CHECK_INSTANCE_TYPE(obj, FROGR_TYPE_MAIN_WINDOW_GTK))
#define FROGR_IS_MAIN_WINDOW_GTK_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), FROGR_TYPE_MAIN_WINDOW_GTK))
#define FROGR_MAIN_WINDOW_GTK_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), FROGR_TYPE_MAIN_WINDOW_GTK, FrogrMainWindowGtkClass))

typedef struct _FrogrMainWindowGtk        FrogrMainWindowGtk;
typedef struct _FrogrMainWindowGtkClass   FrogrMainWindowGtkClass;

struct _FrogrMainWindowGtkClass
{
  FrogrMainWindowClass parent_class;
};

struct _FrogrMainWindowGtk
{
  FrogrMainWindow parent;
};

GType frogr_main_window_gtk_get_type (void) G_GNUC_CONST;

FrogrMainWindowGtk *frogr_main_window_gtk_new (void);

G_END_DECLS

#endif /* FROGR_MAIN_WINDOW_GTK_H */
