/*
 * frogr-main-window-model.h -- Main window model in frogr
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

#ifndef _FROGR_MAIN_WINDOW_MODEL_H
#define _FROGR_MAIN_WINDOW_MODEL_H

#include <glib.h>
#include <glib-object.h>
#include "frogr-picture.h"

G_BEGIN_DECLS

#define FROGR_TYPE_MAIN_WINDOW_MODEL           (frogr_main_window_model_get_type())
#define FROGR_MAIN_WINDOW_MODEL(obj)           (G_TYPE_CHECK_INSTANCE_CAST(obj, FROGR_TYPE_MAIN_WINDOW_MODEL, FrogrMainWindowModel))
#define FROGR_MAIN_WINDOW_MODEL_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST(klass, FROGR_TYPE_MAIN_WINDOW_MODEL, FrogrMainWindowModelClass))
#define FROGR_IS_MAIN_WINDOW_MODEL(obj)           (G_TYPE_CHECK_INSTANCE_TYPE(obj, FROGR_TYPE_MAIN_WINDOW_MODEL))
#define FROGR_IS_MAIN_WINDOW_MODEL_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), FROGR_TYPE_MAIN_WINDOW_MODEL))
#define FROGR_MAIN_WINDOW_MODEL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), FROGR_TYPE_MAIN_WINDOW_MODEL, FrogrMainWindowModelClass))

typedef struct _FrogrMainWindowModel FrogrMainWindowModel;
typedef struct _FrogrMainWindowModelClass FrogrMainWindowModelClass;

struct _FrogrMainWindowModel
{
  GObject parent_instance;
};

struct _FrogrMainWindowModelClass
{
  GObjectClass parent_class;
};


GType frogr_main_window_model_get_type(void) G_GNUC_CONST;

FrogrMainWindowModel *frogr_main_window_model_new (void);

void frogr_main_window_model_add_picture (FrogrMainWindowModel *fmainwin_model,
                                          FrogrPicture *fpicture);
void frogr_main_window_model_remove_picture (FrogrMainWindowModel *fmainwin_model,
                                             FrogrPicture *fpicture);
void frogr_main_window_model_remove_all (FrogrMainWindowModel *fmainwin_model);

guint frogr_main_window_model_n_pictures (FrogrMainWindowModel *fmainwin_model);
GSList *frogr_main_window_model_get_pictures (FrogrMainWindowModel *fmainwin_model);

G_END_DECLS

#endif
