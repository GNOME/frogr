/*
 * frogr-group.h -- A group in frogr (a photoset from flickr)
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

#ifndef _FROGR_GROUP_H
#define _FROGR_GROUP_H

#include <glib.h>
#include <glib-object.h>

#include <flicksoup/flicksoup.h>

G_BEGIN_DECLS

#define FROGR_TYPE_GROUP           (frogr_group_get_type())
#define FROGR_GROUP(obj)           (G_TYPE_CHECK_INSTANCE_CAST(obj, FROGR_TYPE_GROUP, FrogrGroup))
#define FROGR_GROUP_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST(klass, FROGR_TYPE_GROUP, FrogrGroupClass))
#define FROGR_IS_GROUP(obj)           (G_TYPE_CHECK_INSTANCE_TYPE(obj, FROGR_TYPE_GROUP))
#define FROGR_IS_GROUP_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), FROGR_TYPE_GROUP))
#define FROGR_GROUP_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), FROGR_TYPE_GROUP, FrogrGroupClass))

typedef struct _FrogrGroup FrogrGroup;
typedef struct _FrogrGroupClass FrogrGroupClass;

struct _FrogrGroup
{
  GObject parent_instance;
};

struct _FrogrGroupClass
{
  GObjectClass parent_class;
};


GType frogr_group_get_type(void) G_GNUC_CONST;

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

G_END_DECLS

#endif
