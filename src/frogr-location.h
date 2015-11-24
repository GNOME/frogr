/*
 * frogr-location.h -- A location in frogr
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

#ifndef _FROGR_LOCATION_H
#define _FROGR_LOCATION_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define FROGR_TYPE_LOCATION (frogr_location_get_type())

G_DECLARE_FINAL_TYPE (FrogrLocation, frogr_location, FROGR, LOCATION, GObject)

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
