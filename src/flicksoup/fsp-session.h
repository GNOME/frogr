/*
 * fsp-session.h
 *
 * Copyright (C) 2010-2011 Mario Sanchez Prada
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
 * You should have received a copy of the GNU General Public Lesser License
 * along with this program; if not, see <http://www.gnu.org/licenses/>
 *
 */

#ifndef _FSP_SESSION_H
#define _FSP_SESSION_H

#include "fsp-data.h"

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define FSP_TYPE_SESSION                  \
  (fsp_session_get_type())
#define FSP_SESSION(obj)                  \
  (G_TYPE_CHECK_INSTANCE_CAST (obj, FSP_TYPE_SESSION, FspSession))
#define FSP_SESSION_CLASS(klass)          \
  (G_TYPE_CHECK_CLASS_CAST(klass, FSP_TYPE_SESSION, FspSessionClass))
#define FSP_IS_SESSION(obj)               \
  (G_TYPE_CHECK_INSTANCE_TYPE(obj, FSP_TYPE_SESSION))
#define FSP_IS_SESSION_CLASS(klass)       \
  (G_TYPE_CHECK_CLASS_TYPE((klass), FSP_TYPE_SESSION))
#define FSP_SESSION_GET_CLASS(obj)        \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), FSP_TYPE_SESSION, FspSessionClass))

typedef struct _FspSession FspSession;
typedef struct _FspSessionClass FspSessionClass;
typedef struct _FspSessionPrivate FspSessionPrivate;

struct _FspSession
{
  GObject parent_instance;
  FspSessionPrivate *priv;
};

struct _FspSessionClass
{
  GObjectClass parent_class;
};


GType
fsp_session_get_type                    (void) G_GNUC_CONST;

FspSession *
fsp_session_new                         (const gchar *api_key,
                                         const gchar *secret,
                                         const gchar *token);
gboolean
fsp_session_set_http_proxy              (FspSession *self,
                                         gboolean use_gnome_proxy,
                                         const char *host, const char *port,
                                         const char *username, const char *password);

const gchar *
fsp_session_get_api_key                 (FspSession *self);

const gchar *
fsp_session_get_secret                  (FspSession *self);

const gchar *
fsp_session_get_token                   (FspSession *self);

void
fsp_session_set_token                   (FspSession  *self,
                                         const gchar *token);

void
fsp_session_get_auth_url_async          (FspSession          *self,
                                         GCancellable        *c,
                                         GAsyncReadyCallback cb,
                                         gpointer             data);
gchar *
fsp_session_get_auth_url_finish         (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error);

void
fsp_session_complete_auth_async         (FspSession          *self,
                                         GCancellable        *c,
                                         GAsyncReadyCallback cb,
                                         gpointer             data);
FspDataAuthToken *
fsp_session_complete_auth_finish        (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error);

void
fsp_session_check_auth_info_async       (FspSession          *self,
                                         GCancellable        *c,
                                         GAsyncReadyCallback cb,
                                         gpointer             data);
FspDataAuthToken *
fsp_session_check_auth_info_finish      (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error);

void
fsp_session_get_upload_status_async     (FspSession          *self,
                                         GCancellable        *c,
                                         GAsyncReadyCallback cb,
                                         gpointer             data);
FspDataUploadStatus *
fsp_session_get_upload_status_finish    (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error);

void
fsp_session_upload_async                (FspSession          *self,
                                         const gchar         *fileuri,
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
fsp_session_upload_finish               (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error);

void
fsp_session_get_info_async              (FspSession          *self,
                                         const gchar         *photo_id,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             data);

FspDataPhotoInfo *
fsp_session_get_info_finish             (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error);
void
fsp_session_get_photosets_async         (FspSession          *self,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             data);

GSList *
fsp_session_get_photosets_finish        (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error);

void
fsp_session_add_to_photoset_async       (FspSession          *self,
                                         const gchar         *photo_id,
                                         const gchar         *photoset_id,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             data);

gboolean
fsp_session_add_to_photoset_finish      (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error);
void
fsp_session_create_photoset_async       (FspSession          *self,
                                         const gchar         *title,
                                         const gchar         *description,
                                         const gchar         *primary_photo_id,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             data);

gchar *
fsp_session_create_photoset_finish      (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error);

void
fsp_session_get_groups_async            (FspSession          *self,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             data);

GSList *
fsp_session_get_groups_finish           (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error);

void
fsp_session_add_to_group_async          (FspSession          *self,
                                         const gchar         *photo_id,
                                         const gchar         *group_id,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             data);

gboolean
fsp_session_add_to_group_finish         (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error);

void
fsp_session_get_tags_list_async         (FspSession          *self,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             data);

GSList *
fsp_session_get_tags_list_finish        (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error);

G_END_DECLS

#endif
