/*
 * frogr-picture-uploader.h -- Asynchronous picture uploader in frogr
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

#ifndef _FROGR_PICTURE_UPLOADER_H
#define _FROGR_PICTURE_UPLOADER_H

#include "frogr-picture.h"

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define FROGR_TYPE_PICTURE_UPLOADER           (frogr_picture_uploader_get_type())
#define FROGR_PICTURE_UPLOADER(obj)           (G_TYPE_CHECK_INSTANCE_CAST(obj, FROGR_TYPE_PICTURE_UPLOADER, FrogrPictureUploader))
#define FROGR_PICTURE_UPLOADER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST(klass, FROGR_TYPE_PICTURE_UPLOADER, FrogrPictureUploaderClass))
#define FROGR_IS_PICTURE_UPLOADER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE(obj, FROGR_TYPE_PICTURE_UPLOADER))
#define FROGR_IS_PICTURE_UPLOADER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), FROGR_TYPE_PICTURE_UPLOADER))
#define FROGR_PICTURE_UPLOADER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), FROGR_TYPE_PICTURE_UPLOADER, FrogrPictureUploaderClass))

typedef struct _FrogrPictureUploader FrogrPictureUploader;
typedef struct _FrogrPictureUploaderClass FrogrPictureUploaderClass;

/* Callback to be executed after every single upload */
typedef void (*FrogrPictureUploadedCallback) (GObject *source,
                                              FrogrPicture *picture,
                                              GError *error);

/* Callback to be executed after all the pictures are uploaded */
typedef void (*FrogrPicturesUploadedCallback) (GObject *source,
                                               GError *error);

/* Callback to actually upload every picture */
typedef void (*FrogrPictureUploadFunc) (GObject *self, FrogrPicture *picture,
                                        FrogrPictureUploadedCallback picture_uploaded_cb,
                                        GObject *object);

struct _FrogrPictureUploader
{
  GObject parent_instance;
};

struct _FrogrPictureUploaderClass
{
  GObjectClass parent_class;
};


GType frogr_picture_uploader_get_type(void) G_GNUC_CONST;

FrogrPictureUploader *frogr_picture_uploader_new (GSList *pictures,
                                                  FrogrPictureUploadFunc picture_upload_func,
                                                  FrogrPictureUploadedCallback picture_uploaded_cb,
                                                  FrogrPicturesUploadedCallback pictures_uploaded_cb,
                                                  gpointer object);

void frogr_picture_uploader_upload (FrogrPictureUploader *self);

G_END_DECLS

#endif
