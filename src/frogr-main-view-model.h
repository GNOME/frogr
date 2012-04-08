/*
 * frogr-main-view-model.h -- Main view model in frogr
 *
 * Copyright (C) 2009-2011 Mario Sanchez Prada
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

#ifndef _FROGR_MAIN_VIEW_MODEL_H
#define _FROGR_MAIN_VIEW_MODEL_H

#include "frogr-account.h"
#include "frogr-photoset.h"
#include "frogr-picture.h"

#include <glib.h>
#include <glib-object.h>

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
                                        FrogrPicture *fset);
void frogr_main_view_model_remove_picture (FrogrMainViewModel *self,
                                           FrogrPicture *fset);

guint frogr_main_view_model_n_pictures (FrogrMainViewModel *self);

GSList *frogr_main_view_model_get_pictures (FrogrMainViewModel *self);

void frogr_main_view_model_reorder_pictures (FrogrMainViewModel *self,
                                             const gchar *property_name,
                                             gboolean reversed);

void frogr_main_view_model_add_photoset (FrogrMainViewModel *self,
                                         FrogrPhotoSet *fset);
void frogr_main_view_model_remove_photoset (FrogrMainViewModel *self,
                                            FrogrPhotoSet *fset);
void frogr_main_view_model_remove_all_photosets (FrogrMainViewModel *self);

guint frogr_main_view_model_n_photosets (FrogrMainViewModel *self);

GSList *frogr_main_view_model_get_photosets (FrogrMainViewModel *self);

void frogr_main_view_model_set_photosets (FrogrMainViewModel *self,
                                          GSList *photosets_list);

void frogr_main_view_model_add_group (FrogrMainViewModel *self,
                                      FrogrGroup *fgroup);
void frogr_main_view_model_remove_group (FrogrMainViewModel *self,
                                         FrogrGroup *fgroup);
void frogr_main_view_model_remove_all_groups (FrogrMainViewModel *self);

guint frogr_main_view_model_n_groups (FrogrMainViewModel *self);

GSList *frogr_main_view_model_get_groups (FrogrMainViewModel *self);

void frogr_main_view_model_set_groups (FrogrMainViewModel *self,
                                       GSList *groups_list);

GSList *frogr_main_view_model_get_remote_tags (FrogrMainViewModel *self);

void frogr_main_view_model_set_remote_tags (FrogrMainViewModel *self,
                                            GSList *tags_list);

void frogr_main_view_model_remove_remote_tags (FrogrMainViewModel *self);

guint frogr_main_view_model_n_remote_tags (FrogrMainViewModel *self);

GSList *frogr_main_view_model_get_local_tags_list (FrogrMainViewModel *self);

void frogr_main_view_model_add_local_tags_from_string (FrogrMainViewModel *self,
                                                       const gchar *tags_str);

void frogr_main_view_model_remove_local_tags (FrogrMainViewModel *self);

guint frogr_main_view_model_n_local_tags (FrogrMainViewModel *self);

GSList *frogr_main_view_model_get_all_tags (FrogrMainViewModel *self);

G_END_DECLS

#endif
