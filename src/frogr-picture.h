/*
 * frogr-picture.h -- A picture in frogr
 *
 * Copyright (C) 2009-2012 Mario Sanchez Prada
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

#ifndef _FROGR_PICTURE_H
#define _FROGR_PICTURE_H

#include "frogr-group.h"
#include "frogr-location.h"
#include "frogr-photoset.h"

#include <glib.h>
#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <flicksoup/flicksoup.h>

G_BEGIN_DECLS

#define FROGR_TYPE_PICTURE (frogr_picture_get_type())

G_DECLARE_FINAL_TYPE (FrogrPicture, frogr_picture, FROGR, PICTURE, GObject)

/* Constructor */
FrogrPicture *frogr_picture_new (const gchar *fileuri,
                                 const gchar *title,
                                 gboolean public,
                                 gboolean family,
                                 gboolean friend,
                                 gboolean is_video);

/* Data managing methods */

const gchar *frogr_picture_get_id (FrogrPicture *self);
void frogr_picture_set_id (FrogrPicture *self,
                           const gchar *id);

const gchar *frogr_picture_get_title (FrogrPicture *self);
void frogr_picture_set_title (FrogrPicture *self,
                              const gchar *title);

const gchar *frogr_picture_get_description (FrogrPicture *self);
void frogr_picture_set_description (FrogrPicture *self,
                                    const gchar *description);

const gchar *frogr_picture_get_fileuri (FrogrPicture *self);

const GSList *frogr_picture_get_tags_list (FrogrPicture *self);
const gchar *frogr_picture_get_tags (FrogrPicture *self);
void frogr_picture_set_tags (FrogrPicture *self, const gchar *tags_string);
void frogr_picture_add_tags (FrogrPicture *self, const gchar *tags_string);
void frogr_picture_remove_tags (FrogrPicture *self);

gboolean frogr_picture_is_public (FrogrPicture *self);
void frogr_picture_set_public (FrogrPicture *self,
                               gboolean public);

gboolean frogr_picture_is_friend (FrogrPicture *self);
void frogr_picture_set_friend (FrogrPicture *self,
                               gboolean friend);

gboolean frogr_picture_is_family (FrogrPicture *self);
void frogr_picture_set_family (FrogrPicture *self,
                               gboolean family);

FspSafetyLevel frogr_picture_get_safety_level (FrogrPicture *self);
void frogr_picture_set_safety_level (FrogrPicture *self,
                                     FspSafetyLevel safety_level);
FspContentType frogr_picture_get_content_type (FrogrPicture *self);
void frogr_picture_set_content_type (FrogrPicture *self,
                                     FspContentType content_type);
FspLicense frogr_picture_get_license (FrogrPicture *self);
void frogr_picture_set_license (FrogrPicture *self, FspLicense license);

FrogrLocation *frogr_picture_get_location (FrogrPicture *self);
void frogr_picture_set_location (FrogrPicture *self, FrogrLocation *location);

gboolean frogr_picture_show_in_search (FrogrPicture *self);
void frogr_picture_set_show_in_search (FrogrPicture *self,
                                       gboolean show_in_search);

gboolean frogr_picture_send_location (FrogrPicture *self);
void frogr_picture_set_send_location (FrogrPicture *self, gboolean send_location);

gboolean frogr_picture_replace_date_posted (FrogrPicture *self);
void frogr_picture_set_replace_date_posted (FrogrPicture *self, gboolean replace_date_posted);

GdkPixbuf *frogr_picture_get_pixbuf (FrogrPicture *self);
void frogr_picture_set_pixbuf (FrogrPicture *self,
                               GdkPixbuf *pixbuf);

gboolean frogr_picture_is_video (FrogrPicture *self);

guint frogr_picture_get_filesize (FrogrPicture *self);
void frogr_picture_set_filesize (FrogrPicture *self, guint filesize);

void frogr_picture_set_datetime (FrogrPicture *self, const gchar *datetime);
const gchar *frogr_picture_get_datetime (FrogrPicture *self);

GSList *frogr_picture_get_photosets (FrogrPicture *self);
void frogr_picture_set_photosets (FrogrPicture *self, GSList *photosets);
void frogr_picture_add_photoset (FrogrPicture *self, FrogrPhotoSet *set);
void frogr_picture_remove_photosets (FrogrPicture *self);
gboolean frogr_picture_in_photoset (FrogrPicture *self, FrogrPhotoSet *set);

GSList *frogr_picture_get_groups (FrogrPicture *self);
void frogr_picture_set_groups (FrogrPicture *self, GSList *groups);
void frogr_picture_add_group (FrogrPicture *self, FrogrGroup *group);
void frogr_picture_remove_groups (FrogrPicture *self);
gboolean frogr_picture_in_group (FrogrPicture *self, FrogrGroup *group);

gint frogr_picture_compare_by_property (FrogrPicture *self, FrogrPicture *other,
                                        const gchar *property_name);

G_END_DECLS

#endif
