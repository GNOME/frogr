/*
 * frogr-photoset.c -- An set in frogr (a photoset from flickr)
 *
 * Copyright (C) 2010-2011 Mario Sanchez Prada
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

#include "frogr-photoset.h"

#define FROGR_PHOTOSET_GET_PRIVATE(object)              \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),               \
                                FROGR_TYPE_SET,         \
                                FrogrPhotoSetPrivate))

G_DEFINE_TYPE (FrogrPhotoSet, frogr_photoset, G_TYPE_OBJECT)

/* Private struct */
typedef struct _FrogrPhotoSetPrivate FrogrPhotoSetPrivate;
struct _FrogrPhotoSetPrivate
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
  PROP_N_PHOTOS
};

/* Prototypes */


/* Private API */

static void
_frogr_photoset_set_property (GObject *object,
                              guint prop_id,
                              const GValue *value,
                              GParamSpec *pspec)
{
  FrogrPhotoSet *self = FROGR_PHOTOSET (object);

  switch (prop_id)
    {
    case PROP_TITLE:
      frogr_photoset_set_title (self, g_value_get_string (value));
      break;
    case PROP_DESCRIPTION:
      frogr_photoset_set_description (self, g_value_get_string (value));
      break;
    case PROP_ID:
      frogr_photoset_set_id (self, g_value_get_string (value));
      break;
    case PROP_PRIMARY_PHOTO_ID:
      frogr_photoset_set_primary_photo_id (self, g_value_get_string (value));
      break;
    case PROP_N_PHOTOS:
      frogr_photoset_set_n_photos (self, g_value_get_int (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
_frogr_photoset_get_property (GObject *object,
                              guint prop_id,
                              GValue *value,
                              GParamSpec *pspec)
{
  FrogrPhotoSetPrivate *priv = FROGR_PHOTOSET_GET_PRIVATE (object);

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
_frogr_photoset_finalize (GObject* object)
{
  FrogrPhotoSetPrivate *priv = FROGR_PHOTOSET_GET_PRIVATE (object);

  /* free strings */
  g_free (priv->title);
  g_free (priv->description);
  g_free (priv->id);
  g_free (priv->primary_photo_id);

  /* call super class */
  G_OBJECT_CLASS (frogr_photoset_parent_class)->finalize(object);
}

static void
frogr_photoset_class_init(FrogrPhotoSetClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS(klass);

  /* GtkObject signals */
  obj_class->set_property = _frogr_photoset_set_property;
  obj_class->get_property = _frogr_photoset_get_property;
  obj_class->finalize = _frogr_photoset_finalize;

  /* Install properties */
  g_object_class_install_property (obj_class,
                                   PROP_TITLE,
                                   g_param_spec_string ("title",
                                                        "title",
                                                        "Set's title",
                                                        NULL,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (obj_class,
                                   PROP_DESCRIPTION,
                                   g_param_spec_string ("description",
                                                        "description",
                                                        "Set's description",
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
                                                        "for the set",
                                                        NULL,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (obj_class,
                                   PROP_N_PHOTOS,
                                   g_param_spec_int ("n-photos",
                                                     "n-photos",
                                                     "Number of photos "
                                                     "inside the set",
                                                     0,
                                                     G_MAXINT,
                                                     0,
                                                     G_PARAM_READWRITE));

  g_type_class_add_private (obj_class, sizeof (FrogrPhotoSetPrivate));
}

static void
frogr_photoset_init (FrogrPhotoSet *self)
{
  FrogrPhotoSetPrivate *priv = FROGR_PHOTOSET_GET_PRIVATE (self);

  /* Default values */
  priv->title = NULL;
  priv->description = NULL;
  priv->id = NULL;
  priv->primary_photo_id = NULL;
  priv->n_photos = 0;
}


/* Public API */

FrogrPhotoSet *
frogr_photoset_new (const gchar *title,
                    const gchar *description)
{
  g_return_val_if_fail (title, NULL);
  g_return_val_if_fail (description, NULL);

  return FROGR_PHOTOSET (g_object_new(FROGR_TYPE_SET,
                                      "title", title,
                                      "description", description,
                                      NULL));
}


/* Data Managing functions */

const gchar *
frogr_photoset_get_title (FrogrPhotoSet *self)
{
  FrogrPhotoSetPrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_SET(self), NULL);

  priv = FROGR_PHOTOSET_GET_PRIVATE (self);
  return (const gchar *)priv->title;
}

void
frogr_photoset_set_title (FrogrPhotoSet *self,
                          const gchar *title)
{
  FrogrPhotoSetPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_SET(self));
  g_return_if_fail(title != NULL);

  priv = FROGR_PHOTOSET_GET_PRIVATE (self);
  g_free (priv->title);
  priv->title = g_strdup (title);
}

const gchar *
frogr_photoset_get_description (FrogrPhotoSet *self)
{
  FrogrPhotoSetPrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_SET(self), NULL);

  priv = FROGR_PHOTOSET_GET_PRIVATE (self);
  return (const gchar *)priv->description;
}

void
frogr_photoset_set_description (FrogrPhotoSet *self,
                                const gchar *description)
{
  FrogrPhotoSetPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_SET(self));

  priv = FROGR_PHOTOSET_GET_PRIVATE (self);
  g_free (priv->description);
  priv->description = g_strdup (description);
}

const gchar *
frogr_photoset_get_id (FrogrPhotoSet *self)
{
  FrogrPhotoSetPrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_SET(self), NULL);

  priv = FROGR_PHOTOSET_GET_PRIVATE (self);
  return (const gchar *)priv->id;
}

void
frogr_photoset_set_id (FrogrPhotoSet *self,
                       const gchar *id)
{
  FrogrPhotoSetPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_SET(self));

  priv = FROGR_PHOTOSET_GET_PRIVATE (self);
  g_free (priv->id);
  priv->id = g_strdup (id);
}

const gchar *
frogr_photoset_get_primary_photo_id (FrogrPhotoSet *self)
{
  FrogrPhotoSetPrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_SET(self), NULL);

  priv = FROGR_PHOTOSET_GET_PRIVATE (self);
  return (const gchar *)priv->primary_photo_id;
}

void
frogr_photoset_set_primary_photo_id (FrogrPhotoSet *self,
                                     const gchar *primary_photo_id)
{
  FrogrPhotoSetPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_SET(self));

  priv = FROGR_PHOTOSET_GET_PRIVATE (self);
  g_free (priv->primary_photo_id);
  priv->primary_photo_id = g_strdup (primary_photo_id);
}

gint
frogr_photoset_get_n_photos (FrogrPhotoSet *self)
{
  FrogrPhotoSetPrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_SET(self), FALSE);

  priv = FROGR_PHOTOSET_GET_PRIVATE (self);
  return priv->n_photos;
}

void
frogr_photoset_set_n_photos (FrogrPhotoSet *self,
                             gint n)
{
  FrogrPhotoSetPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_SET(self));

  priv = FROGR_PHOTOSET_GET_PRIVATE (self);
  priv->n_photos = n;
}

