/*
 * fsp-photos-mgr.c
 *
 * Copyright (C) 2010 Mario Sanchez Prada
 * Photosors: Mario Sanchez Prada <msanchez@igalia.com>
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

#include "fsp-photos-mgr.h"

#include "fsp-error.h"
#include "fsp-session-priv.h"
#include "fsp-session.h"
#include "fsp-util.h"

#define FSP_PHOTOS_MGR_GET_PRIVATE(object)            \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),               \
                                FSP_TYPE_PHOTOS_MGR,  \
                                FspPhotosMgrPrivate))

G_DEFINE_TYPE (FspPhotosMgr, fsp_photos_mgr, G_TYPE_OBJECT);


/* Private struct */
struct _FspPhotosMgrPrivate
{
  FspSession *session;
  FspFlickrProxy *flickr_proxy; /* Lazy initialization */
};

/* Properties */

enum  {
  PROP_0,
  PROP_SESSION
};


/* Prototypes */

static FspFlickrProxy *
_get_flickr_proxy                       (FspPhotosMgr *self);

static GHashTable *
_get_upload_extra_params                (const gchar    *title,
                                         const gchar    *description,
                                         const gchar    *tags,
                                         FspVisibility   is_public,
                                         FspVisibility   is_family,
                                         FspVisibility   is_friend,
                                         FspSafetyLevel  safety_level,
                                         FspContentType  content_type,
                                         FspSearchScope  hidden);

static void
_upload_cb                              (GObject      *object,
                                         GAsyncResult *result,
                                         gpointer      data);

static void
_get_info_cb                            (GObject      *object,
                                         GAsyncResult *result,
                                         gpointer      data);

/* Private API */

static void
fsp_photos_mgr_set_property             (GObject      *object,
                                         guint         prop_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
  FspPhotosMgr *self = FSP_PHOTOS_MGR (object);
  FspPhotosMgrPrivate *priv = self->priv;

  switch (prop_id)
    {
    case PROP_SESSION:
      priv->session = FSP_SESSION (g_value_dup_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
fsp_photos_mgr_get_property             (GObject    *object,
                                         guint       prop_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
  FspPhotosMgr *self = FSP_PHOTOS_MGR (object);

  switch (prop_id)
    {
    case PROP_SESSION:
      g_value_set_object (value, self->priv->session);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
fsp_photos_mgr_dispose                  (GObject* object)
{
  FspPhotosMgr *self = FSP_PHOTOS_MGR (object);

  /* Unref objects */
  if (self->priv->session)
    {
      g_object_unref (self->priv->session);
      self->priv->session = NULL;
    }

  if (self->priv->flickr_proxy)
    {
      g_object_unref (self->priv->flickr_proxy);
      self->priv->flickr_proxy = NULL;
    }

  /* Call superclass */
  G_OBJECT_CLASS (fsp_photos_mgr_parent_class)->dispose(object);
}

static void
fsp_photos_mgr_class_init               (FspPhotosMgrClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS(klass);
  GParamSpec *pspec = NULL;

  /* Install GOBject methods */
  obj_class->set_property = fsp_photos_mgr_set_property;
  obj_class->get_property = fsp_photos_mgr_get_property;
  obj_class->dispose = fsp_photos_mgr_dispose;

  /* Install properties */
  pspec = g_param_spec_object ("session", "session", "Session",
                               FSP_TYPE_SESSION,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (obj_class, PROP_SESSION, pspec);

  /* Add private */
  g_type_class_add_private (obj_class, sizeof (FspPhotosMgrPrivate));
}

static void
fsp_photos_mgr_init                     (FspPhotosMgr *self)
{
  self->priv = FSP_PHOTOS_MGR_GET_PRIVATE (self);

  self->priv->session = NULL;
  self->priv->flickr_proxy = NULL;
}

static FspFlickrProxy *
_get_flickr_proxy                       (FspPhotosMgr *self)
{
  g_return_val_if_fail (FSP_IS_PHOTOS_MGR (self), NULL);

  FspPhotosMgrPrivate *priv = self->priv;

  if (priv->flickr_proxy == NULL)
    priv->flickr_proxy = g_object_ref (fsp_session_get_flickr_proxy (priv->session));

  return priv->flickr_proxy;
}

static GHashTable *
_get_upload_extra_params                (const gchar    *title,
                                         const gchar    *description,
                                         const gchar    *tags,
                                         FspVisibility   is_public,
                                         FspVisibility   is_family,
                                         FspVisibility   is_friend,
                                         FspSafetyLevel  safety_level,
                                         FspContentType  content_type,
                                         FspSearchScope  hidden)
{
  GHashTable *table = g_hash_table_new_full (g_str_hash, g_str_equal,
                                             (GDestroyNotify) g_free,
                                             (GDestroyNotify) g_free);
  /* Title */
  if (title != NULL)
    g_hash_table_insert (table, g_strdup ("title"), g_strdup (title));

  /* Description */
  if (description != NULL)
    g_hash_table_insert (table, g_strdup ("description"), g_strdup (description));

  /* Tags */
  if (tags != NULL)
    g_hash_table_insert (table, g_strdup ("tags"), g_strdup (tags));

  /* Is public */
  if (is_public != FSP_VISIBILITY_NONE)
    g_hash_table_insert (table, g_strdup ("is_public"),
                         g_strdup_printf ("%d", is_public));

  /* Is family */
  if (is_family != FSP_VISIBILITY_NONE)
    g_hash_table_insert (table, g_strdup ("is_family"),
                         g_strdup_printf ("%d", is_family));

  /* Is friend */
  if (is_friend != FSP_VISIBILITY_NONE)
    g_hash_table_insert (table, g_strdup ("is_friend"),
                         g_strdup_printf ("%d", is_friend));

  /* Safety level */
  if (safety_level != FSP_SAFETY_LEVEL_NONE)
    g_hash_table_insert (table, g_strdup ("safety_level"),
                         g_strdup_printf ("%d", safety_level));

  /* Content type */
  if (content_type != FSP_CONTENT_TYPE_NONE)
    g_hash_table_insert (table, g_strdup ("content_type"),
                         g_strdup_printf ("%d", content_type));

  /* Hidden */
  if (hidden != FSP_SEARCH_SCOPE_NONE)
    g_hash_table_insert (table, g_strdup ("hidden"),
                         g_strdup_printf ("%d", hidden));

  return table;
}

static void
_upload_cb                              (GObject      *object,
                                         GAsyncResult *result,
                                         gpointer      data)
{
  g_return_if_fail (FSP_IS_FLICKR_PROXY (object));
  g_return_if_fail (G_IS_ASYNC_RESULT (result));
  g_return_if_fail (data != NULL);

  FspFlickrProxy *proxy = FSP_FLICKR_PROXY(object);
  gchar *photo_id = NULL;
  GError *error = NULL;

  photo_id = fsp_flickr_proxy_photo_upload_finish (proxy, result, &error);
  build_async_result_and_complete ((GAsyncData *) data, (gpointer) photo_id, error);
}

static void
_get_info_cb                            (GObject      *object,
                                         GAsyncResult *result,
                                         gpointer      data)
{
  g_return_if_fail (FSP_IS_FLICKR_PROXY (object));
  g_return_if_fail (G_IS_ASYNC_RESULT (result));
  g_return_if_fail (data != NULL);

  FspFlickrProxy *proxy = FSP_FLICKR_PROXY(object);
  FspDataPhotoInfo *photo_info = NULL;
  GError *error = NULL;

  photo_info = fsp_flickr_proxy_photo_get_info_finish (proxy, result, &error);
  build_async_result_and_complete ((GAsyncData *) data, (gpointer) photo_info, error);
}

/* Public API */

FspPhotosMgr *
fsp_photos_mgr_new                      (FspSession *session)
{
  g_return_val_if_fail (session != NULL, NULL);

  GObject *object = g_object_new (FSP_TYPE_PHOTOS_MGR,
                                  "session", session,
                                  NULL);
  return FSP_PHOTOS_MGR (object);
}

FspSession *
fsp_photos_mgr_get_session              (FspPhotosMgr *self)
{
  g_return_val_if_fail (FSP_IS_PHOTOS_MGR (self), NULL);

  return self->priv->session;
}

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
                                         gpointer             data)
{
  g_return_if_fail (FSP_IS_PHOTOS_MGR (self));
  g_return_if_fail (filepath != NULL);

  FspFlickrProxy *proxy = NULL;
  GHashTable *extra_params = NULL;
  GAsyncData *clos = g_slice_new0 (GAsyncData);
  clos->object = G_OBJECT (self);
  clos->cancellable = cancellable;
  clos->callback = callback;
  clos->source_tag = fsp_photos_mgr_upload_async;
  clos->data = data;

  /* Get flickr proxy and extra params (those actually used) */
  proxy = _get_flickr_proxy (self);
  extra_params = _get_upload_extra_params (title, description, tags,
                                           is_public, is_family, is_friend,
                                           safety_level, content_type, hidden);

  fsp_flickr_proxy_photo_upload_async (proxy, filepath, extra_params,
                                       cancellable, _upload_cb, clos);
  /* Free hash table */
  g_hash_table_unref (extra_params);
}

gchar *
fsp_photos_mgr_upload_finish            (FspPhotosMgr  *self,
                                         GAsyncResult  *res,
                                         GError       **error)
{
  g_return_val_if_fail (FSP_IS_PHOTOS_MGR (self), NULL);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), NULL);

  gchar *photo_id = NULL;

  /* Check for errors */
  if (!check_async_errors_on_finish (G_OBJECT (self), res,
                                     fsp_photos_mgr_upload_async, error))
    {
      GSimpleAsyncResult *simple = NULL;
      gpointer result = NULL;

      /* Get result */
      simple = G_SIMPLE_ASYNC_RESULT (res);
      result = g_simple_async_result_get_op_res_gpointer (simple);
      if (result != NULL)
        photo_id = (gchar *) result;
      else
        g_set_error_literal (error, FSP_ERROR, FSP_ERROR_OTHER,
                             "Internal error");
    }

  return photo_id;
}

void
fsp_photos_mgr_get_info_async           (FspPhotosMgr        *self,
                                         const gchar         *photo_id,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             data)
{
  g_return_if_fail (FSP_IS_PHOTOS_MGR (self));
  g_return_if_fail (photo_id != NULL);

  FspFlickrProxy *proxy = NULL;
  GAsyncData *clos = g_slice_new0 (GAsyncData);
  clos->object = G_OBJECT (self);
  clos->cancellable = cancellable;
  clos->callback = callback;
  clos->source_tag = fsp_photos_mgr_get_info_async;
  clos->data = data;

  /* Get flickr proxy and call it */
  proxy = _get_flickr_proxy (self);
  fsp_flickr_proxy_photo_get_info_async (proxy, photo_id,
                                         cancellable, _get_info_cb, clos);
}

FspDataPhotoInfo *
fsp_photos_mgr_get_info_finish          (FspPhotosMgr  *self,
                                         GAsyncResult  *res,
                                         GError       **error)
{
  g_return_val_if_fail (FSP_IS_PHOTOS_MGR (self), NULL);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), NULL);

  FspDataPhotoInfo *photo_info = NULL;

  /* Check for errors */
  if (!check_async_errors_on_finish (G_OBJECT (self), res,
                                     fsp_photos_mgr_get_info_async, error))
    {
      GSimpleAsyncResult *simple = NULL;
      gpointer result = NULL;

      /* Get result */
      simple = G_SIMPLE_ASYNC_RESULT (res);
      result = g_simple_async_result_get_op_res_gpointer (simple);
      if (result != NULL)
        photo_info = (FspDataPhotoInfo *) result;
      else
        g_set_error_literal (error, FSP_ERROR, FSP_ERROR_OTHER,
                             "Internal error");
    }

  return photo_info;
}
