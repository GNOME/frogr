/*
 * frogr-location.c -- A location in frogr
 *
 * Copyright (C) 2012 Mario Sanchez Prada
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


struct _FrogrLocation
{
  GObject parent;

  gdouble latitude;
  gdouble longitude;
};

G_DEFINE_TYPE (FrogrLocation, frogr_location, G_TYPE_OBJECT)

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
  FrogrLocation *self = FROGR_LOCATION (object);

  switch (prop_id)
    {
    case PROP_LATITUDE:
      g_value_set_double (value, self->latitude);
      break;
    case PROP_LONGITUDE:
      g_value_set_double (value, self->longitude);
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
}

static void
frogr_location_init (FrogrLocation *self)
{
  /* Default values */
  self->latitude = 0.0;
  self->longitude = 0.0;
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
  g_return_val_if_fail(FROGR_IS_LOCATION(self), 0.0);
  return self->latitude;
}

void
frogr_location_set_latitude (FrogrLocation *self,
                             gdouble latitude)
{
  g_return_if_fail(FROGR_IS_LOCATION(self));
  self->latitude = latitude;
}

gdouble
frogr_location_get_longitude (FrogrLocation *self)
{
  g_return_val_if_fail(FROGR_IS_LOCATION(self), 0.0);
  return self->longitude;
}

void
frogr_location_set_longitude (FrogrLocation *self,
                              gdouble longitude)
{
  g_return_if_fail(FROGR_IS_LOCATION(self));
  self->longitude = longitude;
}
