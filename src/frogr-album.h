/*
 * frogr-album.h -- An album in frogr (a photoset from flickr)
 *
 * Copyright (C) 2010 Mario Sanchez Prada
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

#ifndef _FROGR_ALBUM_H
#define _FROGR_ALBUM_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define FROGR_TYPE_ALBUM           (frogr_album_get_type())
#define FROGR_ALBUM(obj)           (G_TYPE_CHECK_INSTANCE_CAST(obj, FROGR_TYPE_ALBUM, FrogrAlbum))
#define FROGR_ALBUM_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST(klass, FROGR_TYPE_ALBUM, FrogrAlbumClass))
#define FROGR_IS_ALBUM(obj)           (G_TYPE_CHECK_INSTANCE_TYPE(obj, FROGR_TYPE_ALBUM))
#define FROGR_IS_ALBUM_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), FROGR_TYPE_ALBUM))
#define FROGR_ALBUM_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), FROGR_TYPE_ALBUM, FrogrAlbumClass))

typedef struct _FrogrAlbum FrogrAlbum;
typedef struct _FrogrAlbumClass FrogrAlbumClass;

struct _FrogrAlbum
{
  GObject parent_instance;
};

struct _FrogrAlbumClass
{
  GObjectClass parent_class;
};


GType frogr_album_get_type(void) G_GNUC_CONST;

/* Constructors */
FrogrAlbum *frogr_album_new (const gchar *title,
                             const gchar *description);

/* Data managing methods */

const gchar *frogr_album_get_id (FrogrAlbum *self);
void frogr_album_set_id (FrogrAlbum *self,
                         const gchar *id);

const gchar *frogr_album_get_title (FrogrAlbum *self);
void frogr_album_set_title (FrogrAlbum *self,
                            const gchar *title);

const gchar *frogr_album_get_description (FrogrAlbum *self);
void frogr_album_set_description (FrogrAlbum *self,
                                  const gchar *description);

const gchar *frogr_album_get_primary_photo_id (FrogrAlbum *self);
void frogr_album_set_primary_photo_id (FrogrAlbum *self,
                                       const gchar *id);

gint frogr_album_get_n_photos (FrogrAlbum *self);
void frogr_album_set_n_photos (FrogrAlbum *self,
                               gint n);
G_END_DECLS

#endif
