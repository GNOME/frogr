/*
 * frogr-location.c -- A location in frogr
 *
 * Copyright (C) 2011 Mario Sanchez Prada
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

#include "frogr-location.h"

#define FROGR_LOCATION_GET_PRIVATE(object)              \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),               \
                                FROGR_TYPE_LOCATION,    \
                                FrogrLocationPrivate))

G_DEFINE_TYPE (FrogrLocation, frogr_location, G_TYPE_OBJECT)

/* Private struct */
typedef struct _FrogrLocationPrivate FrogrLocationPrivate;
struct _FrogrLocationPrivate
{
  gdouble latitude;
  gdouble longitude;
};

/* Properties */
enum  {
  PROP_0,
  PROP_LATITUDE,
  PROP_LONGITUDE,
};

/* Private API */

static void
_frogr_location_set_property (GObject *object,
                             guint prop_id,
                             const GValue *value,
                             GParamSpec *pspec)
{
  FrogrLocation *self = FROGR_LOCATION (object);

  switch (prop_id)
    {
    case PROP_LATITUDE:
      frogr_location_set_latitude (self, g_value_get_double (value));
      break;
    case PROP_LONGITUDE:
      frogr_location_set_longitude (self, g_value_get_double (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
_frogr_location_get_property (GObject *object,
                             guint prop_id,
                             GValue *value,
                             GParamSpec *pspec)
{
  FrogrLocationPrivate *priv = FROGR_LOCATION_GET_PRIVATE (object);

  switch (prop_id)
    {
    case PROP_LATITUDE:
      g_value_set_double (value, priv->latitude);
      break;
    case PROP_LONGITUDE:
      g_value_set_double (value, priv->longitude);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
frogr_location_class_init(FrogrLocationClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS(klass);

  /* GtkObject signals */
  obj_class->set_property = _frogr_location_set_property;
  obj_class->get_property = _frogr_location_get_property;

  /* Install properties */
  g_object_class_install_property (obj_class,
                                   PROP_LATITUDE,
                                   g_param_spec_double ("latitude",
                                                        "latitude",
                                                        "Latitude for the location",
                                                        -90.0,
                                                        90.0,
                                                        0.0,
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (obj_class,
                                   PROP_LONGITUDE,
                                   g_param_spec_double ("longitude",
                                                        "longitude",
                                                        "Longitude for the location",
                                                        -180.0,
                                                        180.0,
                                                        0.0,
                                                        G_PARAM_READWRITE));

  g_type_class_add_private (obj_class, sizeof (FrogrLocationPrivate));
}

static void
frogr_location_init (FrogrLocation *self)
{
  FrogrLocationPrivate *priv = FROGR_LOCATION_GET_PRIVATE (self);

  /* Default values */
  priv->latitude = 0.0;
  priv->longitude = 0.0;
}


/* Public API */

FrogrLocation *
frogr_location_new (gdouble latitude, gdouble longitude)
{
  return FROGR_LOCATION (g_object_new(FROGR_TYPE_LOCATION,
                                     "latitude", latitude,
                                     "longitude", longitude,
                                     NULL));
}


/* Data Managing functions */

gdouble
frogr_location_get_latitude (FrogrLocation *self)
{
  FrogrLocationPrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_LOCATION(self), 0.0);

  priv = FROGR_LOCATION_GET_PRIVATE (self);
  return priv->latitude;
}

void
frogr_location_set_latitude (FrogrLocation *self,
                             gdouble latitude)
{
  FrogrLocationPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_LOCATION(self));

  priv = FROGR_LOCATION_GET_PRIVATE (self);
  priv->latitude = latitude;
}

gdouble
frogr_location_get_longitude (FrogrLocation *self)
{
  FrogrLocationPrivate *priv = NULL;

  g_return_val_if_fail(FROGR_IS_LOCATION(self), 0.0);

  priv = FROGR_LOCATION_GET_PRIVATE (self);
  return priv->longitude;
}

void
frogr_location_set_longitude (FrogrLocation *self,
                              gdouble longitude)
{
  FrogrLocationPrivate *priv = NULL;

  g_return_if_fail(FROGR_IS_LOCATION(self));

  priv = FROGR_LOCATION_GET_PRIVATE (self);
  priv->longitude = longitude;
}
