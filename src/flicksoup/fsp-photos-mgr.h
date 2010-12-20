/*
 * fsp-photos-mgr.h
 *
 * Copyright (C) 2010 Mario Sanchez Prada
 * Authors: Mario Sanchez Prada <msanchez@igalia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 3 of the GNU Lesser General
 * Public License as published by the Free Software Foundation.
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

#ifndef _FSP_PHOTOS_MGR_H
#define _FSP_PHOTOS_MGR_H

#include <glib.h>
#include <glib-object.h>

#include <flicksoup/fsp-data.h>
#include <flicksoup/fsp-session.h>

G_BEGIN_DECLS

#define FSP_TYPE_PHOTOS_MGR                   \
  (fsp_photos_mgr_get_type())
#define FSP_PHOTOS_MGR(obj)                                           \
  (G_TYPE_CHECK_INSTANCE_CAST (obj, FSP_TYPE_PHOTOS_MGR, FspPhotosMgr))
#define FSP_PHOTOS_MGR_CLASS(klass)                                   \
  (G_TYPE_CHECK_CLASS_CAST(klass, FSP_TYPE_PHOTOS_MGR, FspPhotosMgrClass))
#define FSP_IS_PHOTOS_MGR(obj)                                \
  (G_TYPE_CHECK_INSTANCE_TYPE(obj, FSP_TYPE_PHOTOS_MGR))
#define FSP_IS_PHOTOS_MGR_CLASS(klass)                        \
  (G_TYPE_CHECK_CLASS_TYPE((klass), FSP_TYPE_PHOTOS_MGR))
#define FSP_PHOTOS_MGR_GET_CLASS(obj)                                 \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), FSP_TYPE_PHOTOS_MGR, FspPhotosMgrClass))

typedef struct _FspPhotosMgr FspPhotosMgr;
typedef struct _FspPhotosMgrClass FspPhotosMgrClass;
typedef struct _FspPhotosMgrPrivate FspPhotosMgrPrivate;

struct _FspPhotosMgr
{
  GObject parent_instance;
  FspPhotosMgrPrivate *priv;
};

struct _FspPhotosMgrClass
{
  GObjectClass parent_class;
};


GType
fsp_photos_mgr_get_type                 (void) G_GNUC_CONST;

FspPhotosMgr *
fsp_photos_mgr_new                      (FspSession *session);

FspSession *
fsp_photos_mgr_get_session              (FspPhotosMgr *self);

void
fsp_photos_mgr_upload_async             (FspPhotosMgr        *self,
                                         const gchar         *filepath,
                                         const gchar         *title,
                                         const gchar         *description,
                                         const gchar         *tags,
                                         FspVisibility        is_public,
                                         FspVisibility        is_family,
                                         FspVisibility        is_friend,
                                         FspSafetyLevel       safety_level,
                                         FspContentType       content_type,
                                         FspSearchScope       hidden,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             data);

gchar *
fsp_photos_mgr_upload_finish            (FspPhotosMgr  *self,
                                         GAsyncResult  *res,
                                         GError       **error);

void
fsp_photos_mgr_get_info_async           (FspPhotosMgr        *self,
                                         const gchar         *photo_id,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             data);

FspDataPhotoInfo *
fsp_photos_mgr_get_info_finish          (FspPhotosMgr  *self,
                                         GAsyncResult  *res,
                                         GError       **error);
void
fsp_photos_mgr_get_photosets_async      (FspPhotosMgr        *self,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             data);

GSList *
fsp_photos_mgr_get_photosets_finish     (FspPhotosMgr  *self,
                                         GAsyncResult  *res,
                                         GError       **error);

void
fsp_photos_mgr_add_to_photoset_async    (FspPhotosMgr        *self,
                                         const gchar         *photo_id,
                                         const gchar         *photoset_id,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             data);

gboolean
fsp_photos_mgr_add_to_photoset_finish   (FspPhotosMgr  *self,
                                         GAsyncResult  *res,
                                         GError       **error);
void
fsp_photos_mgr_create_photoset_async    (FspPhotosMgr        *self,
                                         const gchar         *title,
                                         const gchar         *description,
                                         const gchar         *primary_photo_id,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             data);

gchar *
fsp_photos_mgr_create_photoset_finish   (FspPhotosMgr  *self,
                                         GAsyncResult  *res,
                                         GError       **error);


G_END_DECLS

#endif
