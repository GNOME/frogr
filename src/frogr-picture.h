/*
 * frogr-picture.h -- A picture in frogr
 *
 * Copyright (C) 2009 Mario Sanchez Prada
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

#include <glib.h>
#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

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
                                 gboolean public);

/* Data managing methods */

const gchar *frogr_picture_get_id (FrogrPicture *fpicture);
void frogr_picture_set_id (FrogrPicture *fpicture,
                           const gchar *id);

const gchar *frogr_picture_get_title (FrogrPicture *fpicture);
void frogr_picture_set_title (FrogrPicture *fpicture,
                              const gchar *title);

const gchar *frogr_picture_get_description (FrogrPicture *fpicture);
void frogr_picture_set_description (FrogrPicture *fpicture,
                                    const gchar *description);

const gchar *frogr_picture_get_filepath (FrogrPicture *fpicture);
void frogr_picture_set_filepath (FrogrPicture *fpicture,
                                 const gchar *filepath);

const GSList *frogr_picture_get_tags_list (FrogrPicture *fpicture);
const gchar *frogr_picture_get_tags (FrogrPicture *fpicture);
void frogr_picture_set_tags (FrogrPicture *fpicture,
                             const gchar *tags_string);

gboolean frogr_picture_is_public (FrogrPicture *fpicture);
void frogr_picture_set_public (FrogrPicture *fpicture,
                               gboolean public);

gboolean frogr_picture_is_friend (FrogrPicture *fpicture);
void frogr_picture_set_friend (FrogrPicture *fpicture,
                               gboolean friend);

gboolean frogr_picture_is_family (FrogrPicture *fpicture);
void frogr_picture_set_family (FrogrPicture *fpicture,
                               gboolean family);

GdkPixbuf *frogr_picture_get_pixbuf (FrogrPicture *fpicture);
void frogr_picture_set_pixbuf (FrogrPicture *fpicture,
                               GdkPixbuf *pixbuf);

G_END_DECLS

#endif
