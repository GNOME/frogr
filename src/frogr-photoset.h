/*
 * frogr-photoset.h -- An set in frogr (a photoset from flickr)
 *
 * Copyright (C) 2010-2012 Mario Sanchez Prada
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

#ifndef _FROGR_PHOTOSET_H
#define _FROGR_PHOTOSET_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define FROGR_TYPE_PHOTOSET           (frogr_photoset_get_type())
#define FROGR_PHOTOSET(obj)           (G_TYPE_CHECK_INSTANCE_CAST(obj, FROGR_TYPE_PHOTOSET, FrogrPhotoSet))
#define FROGR_PHOTOSET_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST(klass, FROGR_TYPE_PHOTOSET, FrogrPhotoSetClass))
#define FROGR_IS_PHOTOSET(obj)           (G_TYPE_CHECK_INSTANCE_TYPE(obj, FROGR_TYPE_PHOTOSET))
#define FROGR_IS_PHOTOSET_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), FROGR_TYPE_PHOTOSET))
#define FROGR_PHOTOSET_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), FROGR_TYPE_PHOTOSET, FrogrPhotoSetClass))

typedef struct _FrogrPhotoSet FrogrPhotoSet;
typedef struct _FrogrPhotoSetClass FrogrPhotoSetClass;

struct _FrogrPhotoSet
{
  GObject parent_instance;
};

struct _FrogrPhotoSetClass
{
  GObjectClass parent_class;
};


GType frogr_photoset_get_type(void) G_GNUC_CONST;

/* Constructors */

FrogrPhotoSet *frogr_photoset_new (const gchar *id,
                                   const gchar *title,
                                   const gchar *description);

FrogrPhotoSet *frogr_photoset_new_local (const gchar *title,
                                         const gchar *description);
/* Data managing methods */

const gchar *frogr_photoset_get_id (FrogrPhotoSet *self);
void frogr_photoset_set_id (FrogrPhotoSet *self,
                            const gchar *id);

const gchar *frogr_photoset_get_local_id (FrogrPhotoSet *self);
void frogr_photoset_set_local_id (FrogrPhotoSet *self,
                                  const gchar *id);

const gchar *frogr_photoset_get_title (FrogrPhotoSet *self);
void frogr_photoset_set_title (FrogrPhotoSet *self,
                               const gchar *title);

const gchar *frogr_photoset_get_description (FrogrPhotoSet *self);
void frogr_photoset_set_description (FrogrPhotoSet *self,
                                     const gchar *description);

const gchar *frogr_photoset_get_primary_photo_id (FrogrPhotoSet *self);
void frogr_photoset_set_primary_photo_id (FrogrPhotoSet *self,
                                          const gchar *id);

gint frogr_photoset_get_n_photos (FrogrPhotoSet *self);
void frogr_photoset_set_n_photos (FrogrPhotoSet *self,
                                  gint n);

gboolean frogr_photoset_is_local (FrogrPhotoSet *self);

G_END_DECLS

#endif
