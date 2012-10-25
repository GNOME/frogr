/*
 * frogr-location.h -- A location in frogr
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

#ifndef _FROGR_LOCATION_H
#define _FROGR_LOCATION_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define FROGR_TYPE_LOCATION           (frogr_location_get_type())
#define FROGR_LOCATION(obj)           (G_TYPE_CHECK_INSTANCE_CAST(obj, FROGR_TYPE_LOCATION, FrogrLocation))
#define FROGR_LOCATION_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST(klass, FROGR_TYPE_LOCATION, FrogrLocationClass))
#define FROGR_IS_LOCATION(obj)           (G_TYPE_CHECK_INSTANCE_TYPE(obj, FROGR_TYPE_LOCATION))
#define FROGR_IS_LOCATION_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), FROGR_TYPE_LOCATION))
#define FROGR_LOCATION_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), FROGR_TYPE_LOCATION, FrogrLocationClass))

typedef struct _FrogrLocation FrogrLocation;
typedef struct _FrogrLocationClass FrogrLocationClass;

struct _FrogrLocation
{
  GObject parent_instance;
};

struct _FrogrLocationClass
{
  GObjectClass parent_class;
};


GType frogr_location_get_type(void) G_GNUC_CONST;

/* Constructor */
FrogrLocation *frogr_location_new (gdouble latitude, gdouble longitude);

/* Data Managing functions */

gdouble frogr_location_get_latitude (FrogrLocation *self);
void frogr_location_set_latitude (FrogrLocation *self,
                                  gdouble latitude);

gdouble frogr_location_get_longitude (FrogrLocation *self);
void frogr_location_set_longitude (FrogrLocation *self,
                                   gdouble longitude);

G_END_DECLS

#endif
