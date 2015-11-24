/*
 * frogr-main-view.h -- Main view for the application
 *
 * Copyright (C) 2009-2012 Mario Sanchez Prada
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

#ifndef FROGR_MAIN_VIEW_H
#define FROGR_MAIN_VIEW_H

#include "frogr-model.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define FROGR_TYPE_MAIN_VIEW (frogr_main_view_get_type())

G_DECLARE_FINAL_TYPE (FrogrMainView, frogr_main_view, FROGR, MAIN_VIEW, GtkApplicationWindow)

FrogrMainView *frogr_main_view_new (GtkApplication *app);

void frogr_main_view_update_project_path (FrogrMainView *self, const gchar *path);

void frogr_main_view_set_status_text (FrogrMainView *self, const gchar *text);

void frogr_main_view_show_progress (FrogrMainView *self, const gchar *title, const gchar *text);

void frogr_main_view_set_progress_description (FrogrMainView *self, const gchar *text);

void frogr_main_view_set_progress_status_text (FrogrMainView *self, const gchar *text);

void frogr_main_view_set_progress_status_fraction (FrogrMainView *self, double fraction);

void frogr_main_view_pulse_progress (FrogrMainView *self);

void frogr_main_view_hide_progress (FrogrMainView *self);

void frogr_main_view_reorder_pictures (FrogrMainView *self);

FrogrModel *frogr_main_view_get_model (FrogrMainView *self);

G_END_DECLS

#endif /* FROGR_MAIN_VIEW_H */
