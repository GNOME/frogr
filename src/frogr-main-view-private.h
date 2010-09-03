/*
 * frogr-main-view-private.h -- Private data for the main view
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

#ifndef FROGR_MAIN_VIEW_PRIVATE_H
#define FROGR_MAIN_VIEW_PRIVATE_H

#include <gtk/gtk.h>
#include "frogr-controller.h"
#include "frogr-main-view.h"
#include "frogr-main-view-model.h"

G_BEGIN_DECLS

#define FROGR_MAIN_VIEW_GET_PRIVATE(object)             \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),               \
                                FROGR_TYPE_MAIN_VIEW,   \
                                FrogrMainViewPrivate))

typedef struct _FrogrMainViewPrivate {
  FrogrMainViewModel *model;
  FrogrMainViewState state;
  FrogrController *controller;
  GtkWindow *window;
} FrogrMainViewPrivate;

void _frogr_main_view_add_tags_to_pictures (FrogrMainView *self);
void _frogr_main_view_edit_selected_pictures (FrogrMainView *self);
void _frogr_main_view_remove_selected_pictures (FrogrMainView *self);
void _frogr_main_view_load_pictures (FrogrMainView *self, GSList *filepaths);
void _frogr_main_view_upload_pictures (FrogrMainView *self);

G_END_DECLS

#endif /* FROGR_MAIN_VIEW_PRIVATE_H */
