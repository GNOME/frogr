/*
 * frogr-picture.h -- A picture in frogr
 *
 * Copyright (C) 2009, 2010, 2011 Mario Sanchez Prada
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

#ifndef _FROGR_PICTURE_H
#define _FROGR_PICTURE_H

#include "frogr-album.h"
#include "frogr-group.h"

#include <glib.h>
#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <flicksoup/flicksoup.h>

G_BEGIN_DECLS

#define FROGR_TYPE_PICTURE           (frogr_picture_get_type())
#define FROGR_PICTURE(obj)           (G_TYPE_CHECK_INSTANCE_CAST(obj, FROGR_TYPE_PICTURE, FrogrPicture))
#define FROGR_PICTURE_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST(klass, FROGR_TYPE_PICTURE, FrogrPictureClass))
#define FROGR_IS_PICTURE(obj)           (G_TYPE_CHECK_INSTANCE_TYPE(obj, FROGR_TYPE_PICTURE))
#define FROGR_IS_PICTURE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), FROGR_TYPE_PICTURE))
#define FROGR_PICTURE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), FROGR_TYPE_PICTURE, FrogrPictureClass))

typedef struct _FrogrPicture FrogrPicture;
typedef struct _FrogrPictureClass FrogrPictureClass;

struct _FrogrPicture
{
  GObject parent_instance;
};

struct _FrogrPictureClass
{
  GObjectClass parent_class;
};


GType frogr_picture_get_type(void) G_GNUC_CONST;

/* Constructor */
FrogrPicture *frogr_picture_new (const gchar *filepath,
                                 const gchar *title,
                                 gboolean public,
                                 gboolean family,
                                 gboolean friend);

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

const gchar *frogr_picture_get_filepath (FrogrPicture *self);
void frogr_picture_set_filepath (FrogrPicture *self,
                                 const gchar *filepath);

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
gboolean frogr_picture_show_in_search (FrogrPicture *self);
void frogr_picture_set_show_in_search (FrogrPicture *self,
                                       gboolean show_in_search);

GdkPixbuf *frogr_picture_get_pixbuf (FrogrPicture *self);
void frogr_picture_set_pixbuf (FrogrPicture *self,
                               GdkPixbuf *pixbuf);

GSList *frogr_picture_get_albums (FrogrPicture *self);
void frogr_picture_set_albums (FrogrPicture *self, GSList *albums);
void frogr_picture_add_album (FrogrPicture *self, FrogrAlbum *album);
void frogr_picture_remove_albums (FrogrPicture *self);
gboolean frogr_picture_in_album (FrogrPicture *self, FrogrAlbum *album);

GSList *frogr_picture_get_groups (FrogrPicture *self);
void frogr_picture_set_groups (FrogrPicture *self, GSList *groups);
void frogr_picture_remove_groups (FrogrPicture *self);
gboolean frogr_picture_in_group (FrogrPicture *self, FrogrGroup *group);

G_END_DECLS

#endif
