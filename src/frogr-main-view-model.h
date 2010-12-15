/*
 * frogr-main-view-model.h -- Main view model in frogr
 *
 * Copyright (C) 2009, 2010 Mario Sanchez Prada
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

#ifndef _FROGR_MAIN_VIEW_MODEL_H
#define _FROGR_MAIN_VIEW_MODEL_H

#include <glib.h>
#include <glib-object.h>
#include "frogr-picture.h"

G_BEGIN_DECLS

#define FROGR_TYPE_MAIN_VIEW_MODEL           (frogr_main_view_model_get_type())
#define FROGR_MAIN_VIEW_MODEL(obj)           (G_TYPE_CHECK_INSTANCE_CAST(obj, FROGR_TYPE_MAIN_VIEW_MODEL, FrogrMainViewModel))
#define FROGR_MAIN_VIEW_MODEL_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST(klass, FROGR_TYPE_MAIN_VIEW_MODEL, FrogrMainViewModelClass))
#define FROGR_IS_MAIN_VIEW_MODEL(obj)           (G_TYPE_CHECK_INSTANCE_TYPE(obj, FROGR_TYPE_MAIN_VIEW_MODEL))
#define FROGR_IS_MAIN_VIEW_MODEL_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), FROGR_TYPE_MAIN_VIEW_MODEL))
#define FROGR_MAIN_VIEW_MODEL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), FROGR_TYPE_MAIN_VIEW_MODEL, FrogrMainViewModelClass))

typedef struct _FrogrMainViewModel FrogrMainViewModel;
typedef struct _FrogrMainViewModelClass FrogrMainViewModelClass;

struct _FrogrMainViewModel
{
  GObject parent_instance;
};

struct _FrogrMainViewModelClass
{
  GObjectClass parent_class;
};


GType frogr_main_view_model_get_type(void) G_GNUC_CONST;

FrogrMainViewModel *frogr_main_view_model_new (void);

void frogr_main_view_model_add_picture (FrogrMainViewModel *self,
                                        FrogrPicture *fpicture);
void frogr_main_view_model_remove_picture (FrogrMainViewModel *self,
                                           FrogrPicture *fpicture);
void frogr_main_view_model_remove_all (FrogrMainViewModel *self);

guint frogr_main_view_model_n_pictures (FrogrMainViewModel *self);
GSList *frogr_main_view_model_get_pictures (FrogrMainViewModel *self);

G_END_DECLS

#endif
