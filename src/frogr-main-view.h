/*
 * frogr-main-view.h -- Main view for the application
 *
 * Copyright (C) 2009-2011 Mario Sanchez Prada
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

#define FROGR_TYPE_MAIN_VIEW           (frogr_main_view_get_type())
#define FROGR_MAIN_VIEW(obj)           (G_TYPE_CHECK_INSTANCE_CAST(obj, FROGR_TYPE_MAIN_VIEW, FrogrMainView))
#define FROGR_MAIN_VIEW_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST(klass, FROGR_TYPE_MAIN_VIEW, FrogrMainViewClass))
#define FROGR_IS_MAIN_VIEW(obj)           (G_TYPE_CHECK_INSTANCE_TYPE(obj, FROGR_TYPE_MAIN_VIEW))
#define FROGR_IS_MAIN_VIEW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), FROGR_TYPE_MAIN_VIEW))
#define FROGR_MAIN_VIEW_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), FROGR_TYPE_MAIN_VIEW, FrogrMainViewClass))

typedef struct _FrogrMainView        FrogrMainView;
typedef struct _FrogrMainViewClass   FrogrMainViewClass;

struct _FrogrMainViewClass
{
  GtkApplicationWindowClass parent_class;
};

struct _FrogrMainView
{
  GtkApplicationWindow parent;
};

GType frogr_main_view_get_type (void) G_GNUC_CONST;

FrogrMainView *frogr_main_view_new (GtkApplication *app);

void frogr_main_view_set_status_text (FrogrMainView *self, const gchar *text);

void frogr_main_view_show_progress (FrogrMainView *self, const gchar *text);

void frogr_main_view_set_progress_description (FrogrMainView *self, const gchar *text);

void frogr_main_view_set_progress_status_text (FrogrMainView *self, const gchar *text);

void frogr_main_view_set_progress_status_fraction (FrogrMainView *self, double fraction);

void frogr_main_view_pulse_progress (FrogrMainView *self);

void frogr_main_view_hide_progress (FrogrMainView *self);

void frogr_main_view_reorder_pictures (FrogrMainView *self);

FrogrModel *frogr_main_view_get_model (FrogrMainView *self);

G_END_DECLS

#endif /* FROGR_MAIN_VIEW_H */
