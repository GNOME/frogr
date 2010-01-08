/*
 * frogr-main-window-private.h -- Private data for the main window
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

#ifndef FROGR_MAIN_WINDOW_PRIVATE_H
#define FROGR_MAIN_WINDOW_PRIVATE_H

#include <gtk/gtk.h>
#include "frogr-controller.h"
#include "frogr-main-window.h"
#include "frogr-main-window-model.h"

G_BEGIN_DECLS

#define FROGR_MAIN_WINDOW_GET_PRIVATE(object)                   \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),                       \
                                FROGR_TYPE_MAIN_WINDOW,         \
                                FrogrMainWindowPrivate))

typedef struct _FrogrMainWindowPrivate {
  FrogrMainWindowModel *model;
  FrogrMainWindowState state;
  FrogrController *controller;
  GtkWindow *window;
} FrogrMainWindowPrivate;

void _frogr_main_window_add_tags_to_pictures (FrogrMainWindow *self);
void _frogr_main_window_edit_selected_pictures (FrogrMainWindow *self);
void _frogr_main_window_remove_selected_pictures (FrogrMainWindow *self);
void _frogr_main_window_load_pictures (FrogrMainWindow *self, GSList *filepaths);
void _frogr_main_window_upload_pictures (FrogrMainWindow *self);

G_END_DECLS

#endif /* FROGR_MAIN_WINDOW_PRIVATE_H */
