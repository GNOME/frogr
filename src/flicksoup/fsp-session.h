/*
 * fsp-session.h
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
void
fsp_session_set_http_proxy              (FspSession *self,
                                         const gchar *proxy_address);

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
fsp_session_get_upload_status_async     (FspSession          *self,
                                         GCancellable        *c,
                                         GAsyncReadyCallback cb,
                                         gpointer             data);
FspDataUploadStatus *
fsp_session_get_upload_status_finish    (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error);

G_END_DECLS

#endif
