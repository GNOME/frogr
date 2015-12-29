/*
 * frogr-group.h -- A group in frogr (a photoset from flickr)
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

#ifndef _FROGR_GROUP_H
#define _FROGR_GROUP_H

#include <glib.h>
#include <glib-object.h>

#include <flicksoup/flicksoup.h>

G_BEGIN_DECLS

#define FROGR_TYPE_GROUP (frogr_group_get_type())

G_DECLARE_FINAL_TYPE (FrogrGroup, frogr_group, FROGR, GROUP, GObject)

/* Constructors */
FrogrGroup *frogr_group_new (const gchar *id,
                             const gchar *name,
                             FspGroupPrivacy privacy,
                             gint n_photos);

/* Data managing methods */

const gchar *frogr_group_get_id (FrogrGroup *self);
void frogr_group_set_id (FrogrGroup *self, const gchar *id);

const gchar *frogr_group_get_name (FrogrGroup *self);
void frogr_group_set_name (FrogrGroup *self, const gchar *name);

FspGroupPrivacy frogr_group_get_privacy (FrogrGroup *self);
void frogr_group_set_privacy (FrogrGroup *self, FspGroupPrivacy privacy);

gint frogr_group_get_n_photos (FrogrGroup *self);
void frogr_group_set_n_photos (FrogrGroup *self, gint n);

gint frogr_group_compare (FrogrGroup *self, FrogrGroup *other);

G_END_DECLS

#endif
