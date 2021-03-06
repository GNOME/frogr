/*
 * frogr-photoset.c -- An set in frogr (a photoset from flickr)
 *
 * Copyright (C) 2010-2020 Mario Sanchez Prada
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
 * Parts of this file based on code from GConf, licensed as GPL
 * version 2 or later (Copyright (C) 1999, 2000 Red Hat Inc)
 */

#include "frogr-photoset.h"

#include <sys/types.h>
#include <unistd.h>

struct _FrogrPhotoSet
{
  GObject parent;

  gchar *title;
  gchar *description;
  gchar *id;
  gchar *local_id; /* For locally created sets only */
  gchar *primary_photo_id;
  gint n_photos;
};

G_DEFINE_TYPE (FrogrPhotoSet, frogr_photoset, G_TYPE_OBJECT)


/* Properties */
enum  {
  PROP_0,
  PROP_TITLE,
  PROP_DESCRIPTION,
  PROP_ID,
  PROP_LOCAL_ID,
  PROP_PRIMARY_PHOTO_ID,
  PROP_N_PHOTOS
};

/* Prototypes */

static gchar *_create_temporary_id_for_photoset (void);


/* Private API */

/* The following function is based in gconf_unique_key(), licensed as
   GPL version 2 or later (Copyright (C) 1999, 2000 Red Hat Inc) */
static gchar *
_create_temporary_id_for_photoset (void)
{
  /* This function is hardly cryptographically random but should be
     "good enough" */
  static guint serial = 0;
  gchar* key;
  gint64 tv, t, ut;
  pid_t p;
  uid_t u;
  guint32 r;

  tv = g_get_real_time ();

  t = tv / G_USEC_PER_SEC;
  ut = tv;

  p = getpid();
  u = getuid();

  /* don't bother to seed; if it's based on the time or any other
     changing info we can get, we may as well just use that changing
     info. since we don't seed we'll at least get a different number
     on every call to this function in the same executable. */
  r = g_random_int();

  /* The letters may increase uniqueness by preventing "melds"
     i.e. 01t01k01 and 0101t0k1 are not the same */
  key = g_strdup_printf("%" G_GUINT32_FORMAT "t"
                        "%" G_GINT64_FORMAT "ut"
                        "%" G_GINT64_FORMAT "u"
                        "%" G_GUINT32_FORMAT "p"
                        "%" G_GINT32_FORMAT "r"
                        "%" G_GUINT32_FORMAT "k"
                        "%" G_GUINT32_FORMAT,
                        /* Duplicate keys must be generated
                           by two different program instances */
                        serial,
                        /* Duplicate keys must be generated
                           in the same microsecond */
                        t,
                        ut,
                        /* Duplicate keys must be generated by
                           the same user */
                        u,
                        /* Duplicate keys must be generated by
                           two programs that got the same PID */
                        p,
                        /* Duplicate keys must be generated with the
                           same random seed and the same index into
                           the series of pseudorandom values */
                        r,
                        /* Duplicate keys must result from running
                           this function at the same stack location */
                        GPOINTER_TO_UINT(&key));
  ++serial;

  return key;
}

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
    case PROP_LOCAL_ID:
      frogr_photoset_set_local_id (self, g_value_get_string (value));
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
  FrogrPhotoSet *self = FROGR_PHOTOSET (object);

  switch (prop_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, frogr_photoset_get_title (self));
      break;
    case PROP_DESCRIPTION:
      g_value_set_string (value, frogr_photoset_get_description (self));
      break;
    case PROP_ID:
      g_value_set_string (value, frogr_photoset_get_id (self));
      break;
    case PROP_LOCAL_ID:
      g_value_set_string (value, frogr_photoset_get_local_id (self));
      break;
    case PROP_PRIMARY_PHOTO_ID:
      g_value_set_string (value, frogr_photoset_get_primary_photo_id (self));
      break;
    case PROP_N_PHOTOS:
      g_value_set_int (value, frogr_photoset_get_n_photos (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
_frogr_photoset_finalize (GObject *object)
{
  FrogrPhotoSet *self = FROGR_PHOTOSET (object);

  /* free strings */
  g_free (self->title);
  g_free (self->description);
  g_free (self->id);
  g_free (self->local_id);
  g_free (self->primary_photo_id);

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
                                   PROP_LOCAL_ID,
                                   g_param_spec_string ("local-id",
                                                        "local-id",
                                                        "Photoset ID locally generated"
                                                        "(for new sets only)",
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
}

static void
frogr_photoset_init (FrogrPhotoSet *self)
{
  /* Default values */
  self->title = NULL;
  self->description = NULL;
  self->id = NULL;
  self->local_id = NULL;
  self->primary_photo_id = NULL;
  self->n_photos = 0;
}


/* Public API */

FrogrPhotoSet *
frogr_photoset_new (const gchar *id,
                    const gchar *title,
                    const gchar *description)
{
  g_return_val_if_fail (title, NULL);
  g_return_val_if_fail (description, NULL);

  return FROGR_PHOTOSET (g_object_new(FROGR_TYPE_PHOTOSET,
                                      "id", id,
                                      "title", title,
                                      "description", description,
                                      NULL));
}

FrogrPhotoSet *
frogr_photoset_new_local (const gchar *title,
                          const gchar *description)
{
  FrogrPhotoSet *new_set;
  g_autofree gchar *id = NULL;

  g_return_val_if_fail (title, NULL);
  g_return_val_if_fail (description, NULL);

  new_set = frogr_photoset_new (NULL, title, description);

  /* We always need to have an id in locally created photosets */
  id = _create_temporary_id_for_photoset ();
  frogr_photoset_set_local_id (new_set, id);

  return new_set;
}

/* Data Managing functions */

const gchar *
frogr_photoset_get_title (FrogrPhotoSet *self)
{
  g_return_val_if_fail(FROGR_IS_PHOTOSET(self), NULL);
  return (const gchar *)self->title;
}

void
frogr_photoset_set_title (FrogrPhotoSet *self,
                          const gchar *title)
{
  g_return_if_fail(FROGR_IS_PHOTOSET(self));
  g_return_if_fail(title != NULL);

  g_free (self->title);
  self->title = g_strdup (title);
}

const gchar *
frogr_photoset_get_description (FrogrPhotoSet *self)
{
  g_return_val_if_fail(FROGR_IS_PHOTOSET(self), NULL);
  return (const gchar *)self->description;
}

void
frogr_photoset_set_description (FrogrPhotoSet *self,
                                const gchar *description)
{
  g_return_if_fail(FROGR_IS_PHOTOSET(self));
  g_free (self->description);
  self->description = g_strdup (description);
}

const gchar *
frogr_photoset_get_id (FrogrPhotoSet *self)
{
  g_return_val_if_fail(FROGR_IS_PHOTOSET(self), NULL);
  return (const gchar *)self->id;
}

void
frogr_photoset_set_id (FrogrPhotoSet *self,
                       const gchar *id)
{
  g_return_if_fail(FROGR_IS_PHOTOSET(self));
  g_free (self->id);
  self->id = g_strdup (id);
}

const gchar *
frogr_photoset_get_local_id (FrogrPhotoSet *self)
{
  g_return_val_if_fail(FROGR_IS_PHOTOSET(self), NULL);
  return (const gchar *)self->local_id;
}

void
frogr_photoset_set_local_id (FrogrPhotoSet *self,
                             const gchar *id)
{
  g_return_if_fail(FROGR_IS_PHOTOSET(self));
  g_free (self->local_id);
  self->local_id = g_strdup (id);
}

const gchar *
frogr_photoset_get_primary_photo_id (FrogrPhotoSet *self)
{
  g_return_val_if_fail(FROGR_IS_PHOTOSET(self), NULL);
  return (const gchar *)self->primary_photo_id;
}

void
frogr_photoset_set_primary_photo_id (FrogrPhotoSet *self,
                                     const gchar *primary_photo_id)
{
  g_return_if_fail(FROGR_IS_PHOTOSET(self));
  g_free (self->primary_photo_id);
  self->primary_photo_id = g_strdup (primary_photo_id);
}

gint
frogr_photoset_get_n_photos (FrogrPhotoSet *self)
{
  g_return_val_if_fail(FROGR_IS_PHOTOSET(self), FALSE);
  return self->n_photos;
}

void
frogr_photoset_set_n_photos (FrogrPhotoSet *self,
                             gint n)
{
  g_return_if_fail(FROGR_IS_PHOTOSET(self));
  self->n_photos = n;
}

gboolean
frogr_photoset_is_local (FrogrPhotoSet *self)
{
  g_return_val_if_fail(FROGR_IS_PHOTOSET(self), FALSE);
  return self->id == NULL && self->local_id != NULL;
}

gint
frogr_photoset_compare (FrogrPhotoSet *self, FrogrPhotoSet *other)
{
  g_return_val_if_fail (FROGR_IS_PHOTOSET (self), 1);
  g_return_val_if_fail (FROGR_IS_PHOTOSET (other), -1);

  if (self == other)
    return 0;

  if (self->id != NULL && other->id != NULL)
    return g_strcmp0 (self->id, other->id);

  if (self->local_id != NULL && other->local_id != NULL)
    return g_strcmp0 (self->local_id, other->local_id);

  if (self->id != NULL)
    return 1;
  else
    return -1;
}
