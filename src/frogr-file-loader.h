/*
 * frogr-file-loader.h -- Asynchronous file loader in frogr
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

#ifndef _FROGR_FILE_LOADER_H
#define _FROGR_FILE_LOADER_H

#include "frogr-picture.h"

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define FROGR_TYPE_FILE_LOADER            (frogr_file_loader_get_type())
#define FROGR_FILE_LOADER(obj)            (G_TYPE_CHECK_INSTANCE_CAST(obj, FROGR_TYPE_FILE_LOADER, FrogrFileLoader))
#define FROGR_FILE_LOADER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST(klass, FROGR_TYPE_FILE_LOADER, FrogrFileLoaderClass))
#define FROGR_IS_FILE_LOADER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE(obj, FROGR_TYPE_FILE_LOADER))
#define FROGR_IS_FILE_LOADER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), FROGR_TYPE_FILE_LOADER))
#define FROGR_FILE_LOADER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), FROGR_TYPE_FILE_LOADER, FrogrFileLoaderClass))

typedef struct _FrogrFileLoader FrogrFileLoader;
typedef struct _FrogrFileLoaderClass FrogrFileLoaderClass;

struct _FrogrFileLoader
{
  GObject parent_instance;
};

struct _FrogrFileLoaderClass
{
  GObjectClass parent_class;
};


GType frogr_file_loader_get_type(void) G_GNUC_CONST;

FrogrFileLoader *frogr_file_loader_new (GSList *file_uris, gulong max_picture_size, gulong max_video_size);

void frogr_file_loader_load (FrogrFileLoader *self);

G_END_DECLS

#endif
