/*
 * fsp-flickr-proxy.h
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

#ifndef _FSP_FLICKR_PROXY_H
#define _FSP_FLICKR_PROXY_H

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

#include <flicksoup/fsp-data.h>
#include <flicksoup/fsp-photos-mgr.h>

G_BEGIN_DECLS

#define FSP_TYPE_FLICKR_PROXY                   \
  (fsp_flickr_proxy_get_type())
#define FSP_FLICKR_PROXY(obj)                                           \
  (G_TYPE_CHECK_INSTANCE_CAST (obj, FSP_TYPE_FLICKR_PROXY, FspFlickrProxy))
#define FSP_FLICKR_PROXY_CLASS(klass)                                   \
  (G_TYPE_CHECK_CLASS_CAST(klass, FSP_TYPE_FLICKR_PROXY, FspFlickrProxyClass))
#define FSP_IS_FLICKR_PROXY(obj)                                \
  (G_TYPE_CHECK_INSTANCE_TYPE(obj, FSP_TYPE_FLICKR_PROXY))
#define FSP_IS_FLICKR_PROXY_CLASS(klass)                        \
  (G_TYPE_CHECK_CLASS_TYPE((klass), FSP_TYPE_FLICKR_PROXY))
#define FSP_FLICKR_PROXY_GET_CLASS(obj)                                 \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), FSP_TYPE_FLICKR_PROXY, FspFlickrProxyClass))

typedef struct _FspFlickrProxy FspFlickrProxy;
typedef struct _FspFlickrProxyClass FspFlickrProxyClass;
typedef struct _FspFlickrProxyPrivate FspFlickrProxyPrivate;

struct _FspFlickrProxy
{
  GObject parent_instance;
  FspFlickrProxyPrivate *priv;
};

struct _FspFlickrProxyClass
{
  GObjectClass parent_class;
};

GType
fsp_flickr_proxy_get_type               (void) G_GNUC_CONST;

FspFlickrProxy *
fsp_flickr_proxy_new                    (const gchar *api_key,
                                         const gchar *secret,
                                         const gchar *token);

/* Authentication */

gchar *
fsp_flickr_proxy_get_api_key            (FspFlickrProxy *self);

gchar *
fsp_flickr_proxy_get_secret             (FspFlickrProxy *self);

gchar *
fsp_flickr_proxy_get_token              (FspFlickrProxy *self);

void
fsp_flickr_proxy_set_token              (FspFlickrProxy *self,
                                         const gchar    *token);

void
fsp_flickr_proxy_get_frob_async         (FspFlickrProxy      *self,
                                         GCancellable        *c,
                                         GAsyncReadyCallback  cb,
                                         gpointer             data);
gchar *
fsp_flickr_proxy_get_frob_finish        (FspFlickrProxy  *self,
                                         GAsyncResult    *res,
                                         GError         **error);

void
fsp_flickr_proxy_get_auth_token_async   (FspFlickrProxy      *self,
                                         const gchar         *frob,
                                         GCancellable        *c,
                                         GAsyncReadyCallback  cb,
                                         gpointer             data);
FspDataAuthToken *
fsp_flickr_proxy_get_auth_token_finish  (FspFlickrProxy  *self,
                                         GAsyncResult    *res,
                                         GError         **error);

/* Photos */

void
fsp_flickr_proxy_photo_upload_async     (FspFlickrProxy      *self,
                                         const gchar         *filepath,
                                         GHashTable          *extra_params,
                                         GCancellable        *c,
                                         GAsyncReadyCallback  cb,
                                         gpointer             data);

gchar *
fsp_flickr_proxy_photo_upload_finish    (FspFlickrProxy *self,
                                         GAsyncResult   *res,
                                         GError        **error);

void
fsp_flickr_proxy_photo_get_info_async   (FspFlickrProxy      *self,
                                         const gchar         *photo_id,
                                         GCancellable        *c,
                                         GAsyncReadyCallback  cb,
                                         gpointer             data);

FspDataPhotoInfo *
fsp_flickr_proxy_photo_get_info_finish  (FspFlickrProxy *self,
                                         GAsyncResult   *res,
                                         GError        **error);

G_END_DECLS

#endif
