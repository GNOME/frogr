/*
 * frogr-model.h -- Model in frogr
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

#ifndef _FROGR_MODEL_H
#define _FROGR_MODEL_H

#include "frogr-account.h"
#include "frogr-photoset.h"
#include "frogr-picture.h"

#include <glib.h>
#include <glib-object.h>
#include <json-glib/json-glib.h>

G_BEGIN_DECLS

#define FROGR_TYPE_MODEL (frogr_model_get_type())

G_DECLARE_FINAL_TYPE (FrogrModel, frogr_model, FROGR, MODEL, GObject)

FrogrModel *frogr_model_new (void);

/* Pictures */

void frogr_model_add_picture (FrogrModel *self,
                              FrogrPicture *fset);
void frogr_model_remove_picture (FrogrModel *self,
                                 FrogrPicture *fset);

guint frogr_model_n_pictures (FrogrModel *self);

GSList *frogr_model_get_pictures (FrogrModel *self);

void frogr_model_notify_changes_in_pictures (FrogrModel *self);

/* Photosets */

void frogr_model_set_remote_photosets (FrogrModel *self,
                                       GSList *photosets_list);

void frogr_model_add_local_photoset (FrogrModel *self,
                                     FrogrPhotoSet *fset);

GSList *frogr_model_get_photosets (FrogrModel *self);

void frogr_model_set_photosets (FrogrModel *self,
                                GSList *photosets);

guint frogr_model_n_photosets (FrogrModel *self);

FrogrPhotoSet *frogr_model_get_photoset_by_id (FrogrModel *self,
                                               const gchar *id);
/* Groups */

guint frogr_model_n_groups (FrogrModel *self);

GSList *frogr_model_get_groups (FrogrModel *self);

void frogr_model_set_groups (FrogrModel *self,
                             GSList *groups_list);

FrogrGroup *frogr_model_get_group_by_id (FrogrModel *self,
                                         const gchar *id);
/* Tags */

void frogr_model_set_remote_tags (FrogrModel *self,
                                  GSList *tags_list);

void frogr_model_add_local_tags_from_string (FrogrModel *self,
                                             const gchar *tags_string);

GSList *frogr_model_get_tags (FrogrModel *self);

guint frogr_model_n_tags (FrogrModel *self);

guint frogr_model_n_local_tags (FrogrModel *self);

/* Serialization */

JsonObject *frogr_model_serialize (FrogrModel *self);

void frogr_model_deserialize (FrogrModel *self, JsonObject *root_object);

G_END_DECLS

#endif
