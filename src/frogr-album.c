/*
 * frogr-album.c -- An album in frogr (a photoset from flickr)
 *
 * Copyright (C) 2010, 2011 Mario Sanchez Prada
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

#include "frogr-album.h"

#define FROGR_ALBUM_GET_PRIVATE(object)                 \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),               \
                                FROGR_TYPE_ALBUM,       \
                                FrogrAlbumPrivate))

G_DEFINE_TYPE (FrogrAlbum, frogr_album, G_TYPE_OBJECT);

/* Private struct */
typedef struct _FrogrAlbumPrivate FrogrAlbumPrivate;
struct _FrogrAlbumPrivate
{
  gchar *title;
  gchar *description;
  gchar *id;
  gchar *primary_photo_id;
  gint n_photos;
};

/* Properties */
enum  {
  PROP_0,
  PROP_TITLE,
  PROP_DESCRIPTION,
  PROP_ID,
  PROP_PRIMARY_PHOTO_ID,
  PROP_N_PHOTOS,
};

/* Prototypes */


/* Private API */

static void
_frogr_album_set_property (GObject *object,
                           guint prop_id,
                           const GValue *value,
                           GParamSpec *pspec)
{
  FrogrAlbum *self = FROGR_ALBUM (object);

  switch (prop_id)
    {
    case PROP_TITLE:
      frogr_album_set_title (self, g_value_get_string (value));
      break;
    case PROP_DESCRIPTION:
      frogr_album_set_description (self, g_value_get_string (value));
      break;
    case PROP_ID:
      frogr_album_set_id (self, g_value_get_string (value));
      break;
    case PROP_PRIMARY_PHOTO_ID:
      frogr_album_set_primary_photo_id (self, g_value_get_string (value));
      break;
    case PROP_N_PHOTOS:
      frogr_album_set_n_photos (self, g_value_get_int (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
_frogr_album_get_property (GObject *object,
                           guint prop_id,
                           GValue *value,
                           GParamSpec *pspec)
{
  FrogrAlbumPrivate *priv = FROGR_ALBUM_GET_PRIVATE (object);

  switch (prop_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, priv->title);
      break;
    case PROP_DESCRIPTION:
      g_value_set_string (value, priv->description);
      break;
    case PROP_ID:
      g_value_set_string (value, priv->id);
      break;
    case PROP_PRIMARY_PHOTO_ID:
      g_value_set_string (value, priv->primary_photo_id);
      break;
    case PROP_N_PHOTOS:
      g_value_set_int (value, priv->n_photos);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
_frogr_album_finalize (GObject* object)
{
  FrogrAlbumPrivate *priv = FROGR_ALBUM_GET_PRIVATE (object);

  /* free strings */
  g_free (priv->title);
  g_free (priv->description);
  g_free (priv->id);
  g_free (priv->primary_photo_id);

  /* call super class */
  G_OBJECT_CLASS (frogr_album_parent_class)->finalize(object);
}

static void
frogr_album_class_init(FrogrAlbumClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS(klass);

  /* GtkObject signals */
  obj_class->set_property = _frogr_album_set_property;
  obj_class->get_property = _frogr_album_get_property;
  obj_class->finalize = _frogr_album_finalize;

  /* Install properties */
  g_object_class_install_property (obj_class,
                                   PROP_TITLE,
                                   g_param_spec_string ("title",
                                                        "title",
                                                        "Album's title",
                                                        NULL,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (obj_class,
                                   PROP_DESCRIPTION,
                                   g_param_spec_string ("description",
                                                        "description",
                                                        "Album's description",
                                                        NULL,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (obj_class,
                                   PROP_ID,
                                   g_param_spec_string ("id",
                                                        "id",
                                                        "Photoset ID from flickr",
                                                        NULL,
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (obj_class,
                                   PROP_PRIMARY_PHOTO_ID,
                                   g_param_spec_string ("primary-photo-id",
                                                        "primary-photo-id",
                                                        "ID of the primary photo "
                                                        "for the album",
                                                        NULL,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (obj_class,
                                   PROP_N_PHOTOS,
                                   g_param_spec_int ("n-photos",
                                                     "n-photos",
                                                     "Number of photos "
                                                     "inside the album",
                                                     0,
                                                     G_MAXINT,
                                                     0,
                                                     G_PARAM_READWRITE));

  g_type_class_add_private (obj_class, sizeof (FrogrAlbumPrivate));
}

static void
frogr_album_init (FrogrAlbum *self)
{
  FrogrAlbumPrivate *priv = FROGR_ALBUM_GET_PRIVATE (self);

  /* Default values */
  priv->title = NULL;
  priv->description = NULL;
  priv->id = NULL;
  priv->primary_photo_id = NULL;
  priv->n_photos = 0;
}


/* Public API */

FrogrAlbum *
frogr_album_new (const gchar *title,
                 const gchar *description)
{
  g_return_val_if_fail (title, NULL);
  g_return_val_if_fail (description, NULL);

  GObject *new = g_object_new(FROGR_TYPE_ALBUM,
                              "title", title,
                              "description", description,
                              NULL);
  return FROGR_ALBUM (new);
}


/* Data Managing functions */

const gchar *
frogr_album_get_title (FrogrAlbum *self)
{
  g_return_val_if_fail(FROGR_IS_ALBUM(self), NULL);

  FrogrAlbumPrivate *priv =
    FROGR_ALBUM_GET_PRIVATE (self);

  return (const gchar *)priv->title;
}

void
frogr_album_set_title (FrogrAlbum *self,
                       const gchar *title)
{
  g_return_if_fail(FROGR_IS_ALBUM(self));
  g_return_if_fail(title != NULL);

  FrogrAlbumPrivate *priv =
    FROGR_ALBUM_GET_PRIVATE (self);

  g_free (priv->title);
  priv->title = g_strdup (title);
}

const gchar *
frogr_album_get_description (FrogrAlbum *self)
{
  g_return_val_if_fail(FROGR_IS_ALBUM(self), NULL);

  FrogrAlbumPrivate *priv =
    FROGR_ALBUM_GET_PRIVATE (self);

  return (const gchar *)priv->description;
}

void
frogr_album_set_description (FrogrAlbum *self,
                             const gchar *description)
{
  g_return_if_fail(FROGR_IS_ALBUM(self));

  FrogrAlbumPrivate *priv =
    FROGR_ALBUM_GET_PRIVATE (self);

  g_free (priv->description);
  priv->description = g_strdup (description);
}

const gchar *
frogr_album_get_id (FrogrAlbum *self)
{
  g_return_val_if_fail(FROGR_IS_ALBUM(self), NULL);

  FrogrAlbumPrivate *priv =
    FROGR_ALBUM_GET_PRIVATE (self);

  return (const gchar *)priv->id;
}

void
frogr_album_set_id (FrogrAlbum *self,
                    const gchar *id)
{
  g_return_if_fail(FROGR_IS_ALBUM(self));

  FrogrAlbumPrivate *priv =
    FROGR_ALBUM_GET_PRIVATE (self);

  g_free (priv->id);
  priv->id = g_strdup (id);
}

const gchar *
frogr_album_get_primary_photo_id (FrogrAlbum *self)
{
  g_return_val_if_fail(FROGR_IS_ALBUM(self), NULL);

  FrogrAlbumPrivate *priv =
    FROGR_ALBUM_GET_PRIVATE (self);

  return (const gchar *)priv->primary_photo_id;
}

void
frogr_album_set_primary_photo_id (FrogrAlbum *self,
                                  const gchar *primary_photo_id)
{
  g_return_if_fail(FROGR_IS_ALBUM(self));

  FrogrAlbumPrivate *priv =
    FROGR_ALBUM_GET_PRIVATE (self);

  g_free (priv->primary_photo_id);
  priv->primary_photo_id = g_strdup (primary_photo_id);
}

gint
frogr_album_get_n_photos (FrogrAlbum *self)
{
  g_return_val_if_fail(FROGR_IS_ALBUM(self), FALSE);

  FrogrAlbumPrivate *priv =
    FROGR_ALBUM_GET_PRIVATE (self);

  return priv->n_photos;
}

void
frogr_album_set_n_photos (FrogrAlbum *self,
                          gint n)
{
  g_return_if_fail(FROGR_IS_ALBUM(self));

  FrogrAlbumPrivate *priv =
    FROGR_ALBUM_GET_PRIVATE (self);

  priv->n_photos = n;
}

