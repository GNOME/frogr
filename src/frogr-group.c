/*
 * frogr-group.c -- A group in frogr (a photoset from flickr)
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

#include "frogr-group.h"

#define FROGR_GROUP_GET_PRIVATE(object)                 \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),               \
                                FROGR_TYPE_GROUP,       \
                                FrogrGroupPrivate))

G_DEFINE_TYPE (FrogrGroup, frogr_group, G_TYPE_OBJECT);

/* Private struct */
typedef struct _FrogrGroupPrivate FrogrGroupPrivate;
struct _FrogrGroupPrivate
{
  gchar *id;
  gchar *name;
  FspGroupPrivacy privacy;
  gint n_photos;
};

/* Properties */
enum  {
  PROP_0,
  PROP_ID,
  PROP_NAME,
  PROP_PRIVACY,
  PROP_N_PHOTOS,
};

/* Prototypes */


/* Private API */

static void
_frogr_group_set_property (GObject *object,
                           guint prop_id,
                           const GValue *value,
                           GParamSpec *pspec)
{
  FrogrGroup *self = FROGR_GROUP (object);

  switch (prop_id)
    {
    case PROP_ID:
      frogr_group_set_id (self, g_value_get_string (value));
      break;
    case PROP_NAME:
      frogr_group_set_name (self, g_value_get_string (value));
      break;
    case PROP_PRIVACY:
      frogr_group_set_privacy (self, g_value_get_int (value));
      break;
    case PROP_N_PHOTOS:
      frogr_group_set_n_photos (self, g_value_get_int (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
_frogr_group_get_property (GObject *object,
                           guint prop_id,
                           GValue *value,
                           GParamSpec *pspec)
{
  FrogrGroupPrivate *priv = FROGR_GROUP_GET_PRIVATE (object);

  switch (prop_id)
    {
    case PROP_ID:
      g_value_set_string (value, priv->id);
      break;
    case PROP_NAME:
      g_value_set_string (value, priv->name);
      break;
    case PROP_PRIVACY:
      g_value_set_int (value, priv->privacy);
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
_frogr_group_finalize (GObject* object)
{
  FrogrGroupPrivate *priv = FROGR_GROUP_GET_PRIVATE (object);

  /* free strings */
  g_free (priv->id);
  g_free (priv->name);

  /* call super class */
  G_OBJECT_CLASS (frogr_group_parent_class)->finalize(object);
}

static void
frogr_group_class_init(FrogrGroupClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS(klass);

  /* GtkObject signals */
  obj_class->set_property = _frogr_group_set_property;
  obj_class->get_property = _frogr_group_get_property;
  obj_class->finalize = _frogr_group_finalize;

  /* Install properties */
  g_object_class_install_property (obj_class,
                                   PROP_ID,
                                   g_param_spec_string ("id",
                                                        "id",
                                                        "Group ID from flickr",
                                                        NULL,
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (obj_class,
                                   PROP_NAME,
                                   g_param_spec_string ("name",
                                                        "name",
                                                        "Group's name",
                                                        NULL,
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (obj_class,
                                   PROP_PRIVACY,
                                   g_param_spec_int ("privacy",
                                                     "privacy",
                                                     "Privacy level of the group",
                                                     FSP_GROUP_PRIVACY_NONE,
                                                     FSP_GROUP_PRIVACY_PUBLIC,
                                                     FSP_GROUP_PRIVACY_NONE,
                                                     G_PARAM_READWRITE));

  g_object_class_install_property (obj_class,
                                   PROP_N_PHOTOS,
                                   g_param_spec_int ("n-photos",
                                                     "n-photos",
                                                     "Number of photos "
                                                     "inside the group",
                                                     0,
                                                     G_MAXINT,
                                                     0,
                                                     G_PARAM_READWRITE));

  g_type_class_add_private (obj_class, sizeof (FrogrGroupPrivate));
}

static void
frogr_group_init (FrogrGroup *self)
{
  FrogrGroupPrivate *priv = FROGR_GROUP_GET_PRIVATE (self);

  /* Default values */
  priv->id = NULL;
  priv->name = NULL;
  priv->privacy = FSP_GROUP_PRIVACY_NONE;
  priv->n_photos = 0;
}


/* Public API */

FrogrGroup *
frogr_group_new (const gchar *id,
                 const gchar *name,
                 FspGroupPrivacy privacy,
                 gint n_photos)
{
  g_return_val_if_fail (id, NULL);
  g_return_val_if_fail (name, NULL);

  GObject *new = g_object_new(FROGR_TYPE_GROUP,
                              "id", id,
                              "name", name,
                              "privacy", privacy,
                              "n_photos", n_photos,
                              NULL);
  return FROGR_GROUP (new);
}


/* Data Managing functions */

const gchar *
frogr_group_get_id (FrogrGroup *self)
{
  g_return_val_if_fail(FROGR_IS_GROUP(self), NULL);

  FrogrGroupPrivate *priv =
    FROGR_GROUP_GET_PRIVATE (self);

  return (const gchar *)priv->id;
}

void
frogr_group_set_id (FrogrGroup *self,
                    const gchar *id)
{
  g_return_if_fail(FROGR_IS_GROUP(self));

  FrogrGroupPrivate *priv =
    FROGR_GROUP_GET_PRIVATE (self);

  g_free (priv->id);
  priv->id = g_strdup (id);
}

const gchar *
frogr_group_get_name (FrogrGroup *self)
{
  g_return_val_if_fail(FROGR_IS_GROUP(self), NULL);

  FrogrGroupPrivate *priv =
    FROGR_GROUP_GET_PRIVATE (self);

  return (const gchar *)priv->name;
}

void
frogr_group_set_name (FrogrGroup *self,
                       const gchar *name)
{
  g_return_if_fail(FROGR_IS_GROUP(self));
  g_return_if_fail(name != NULL);

  FrogrGroupPrivate *priv =
    FROGR_GROUP_GET_PRIVATE (self);

  g_free (priv->name);
  priv->name = g_strdup (name);
}


FspGroupPrivacy
frogr_group_get_privacy (FrogrGroup *self)
{
  g_return_val_if_fail(FROGR_IS_GROUP(self), FALSE);

  FrogrGroupPrivate *priv =
    FROGR_GROUP_GET_PRIVATE (self);

  return priv->privacy;
}

void
frogr_group_set_privacy (FrogrGroup *self, FspGroupPrivacy privacy)
{
  g_return_if_fail(FROGR_IS_GROUP(self));

  FrogrGroupPrivate *priv =
    FROGR_GROUP_GET_PRIVATE (self);

  priv->privacy = privacy;
}

gint
frogr_group_get_n_photos (FrogrGroup *self)
{
  g_return_val_if_fail(FROGR_IS_GROUP(self), FALSE);

  FrogrGroupPrivate *priv =
    FROGR_GROUP_GET_PRIVATE (self);

  return priv->n_photos;
}

void
frogr_group_set_n_photos (FrogrGroup *self,
                          gint n)
{
  g_return_if_fail(FROGR_IS_GROUP(self));

  FrogrGroupPrivate *priv =
    FROGR_GROUP_GET_PRIVATE (self);

  priv->n_photos = n;
}

