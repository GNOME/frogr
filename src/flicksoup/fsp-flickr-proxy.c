/*
 * fsp-flickr-proxy.c
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

#include "fsp-flickr-proxy.h"

#include "fsp-error.h"
#include "fsp-flickr-parser.h"
#include "fsp-util.h"

#include <libsoup/soup.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FLICKR_API_BASE_URL   "http://api.flickr.com/services/rest"
#define FLICKR_API_UPLOAD_URL "http://api.flickr.com/services/upload"

#define FSP_FLICKR_PROXY_GET_PRIVATE(object)            \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),               \
                                FSP_TYPE_FLICKR_PROXY,  \
                                FspFlickrProxyPrivate))

G_DEFINE_TYPE (FspFlickrProxy, fsp_flickr_proxy, G_TYPE_OBJECT);


/* Private struct */
struct _FspFlickrProxyPrivate
{
  gchar *api_key;
  gchar *secret;
  gchar *token;

  FspFlickrParser *parser;
  SoupSession *soup_session;
};

typedef struct
{
  GAsyncData *gasync_data;
  GHashTable *extra_params;
} UploadPhotoData;


/* Properties */

enum  {
  PROP_0,
  PROP_API_KEY,
  PROP_SECRET,
  PROP_TOKEN
};


/* Prototypes */

static gboolean
_check_errors_on_soup_response          (SoupMessage  *msg,
                                         GError      **error);

static SoupMessage *
_get_soup_message_for_upload            (GFile       *file,
                                         const gchar *contents,
                                         gsize        length,
                                         GHashTable  *extra_params);

static void
_load_file_contents_cb                  (GObject      *object,
                                         GAsyncResult *res,
                                         gpointer      data);

static void
_get_frob_soup_session_cb               (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer     data);
static void
_get_auth_token_soup_session_cb         (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer     data);

static void
_photo_upload_soup_session_cb           (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer     data);

static void
_photo_get_info_soup_session_cb         (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer     data);

static void
_perform_async_request                  (FspFlickrProxy      *self,
                                         const gchar         *url,
                                         SoupSessionCallback  request_cb,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             source_tag,
                                         gpointer             data);

static void
_upload_cancelled_cb                    (GCancellable *cancellable,
                                         gpointer      data);

/* Private API */

static void
fsp_flickr_proxy_set_property           (GObject      *object,
                                         guint         prop_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
  FspFlickrProxy *self = FSP_FLICKR_PROXY (object);
  FspFlickrProxyPrivate *priv = self->priv;

  switch (prop_id)
    {
    case PROP_API_KEY:
      g_free (priv->api_key);
      priv->api_key = g_value_dup_string (value);
      break;
    case PROP_SECRET:
      g_free (priv->secret);
      priv->secret = g_value_dup_string (value);
      break;
    case PROP_TOKEN:
      fsp_flickr_proxy_set_token (self, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
fsp_flickr_proxy_get_property           (GObject    *object,
                                         guint       prop_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
  FspFlickrProxy *self = FSP_FLICKR_PROXY (object);

  switch (prop_id)
    {
    case PROP_API_KEY:
      g_value_set_string (value, self->priv->api_key);
      break;
    case PROP_SECRET:
      g_value_set_string (value, self->priv->secret);
      break;
    case PROP_TOKEN:
      g_value_set_string (value, self->priv->token);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
fsp_flickr_proxy_dispose                (GObject* object)
{
  FspFlickrProxy *self = FSP_FLICKR_PROXY (object);

  /* Unref objects */
  if (self->priv->parser)
    {
      g_object_unref (self->priv->parser);
      self->priv->parser = NULL;
    }

  if (self->priv->soup_session)
    {
      g_object_unref (self->priv->soup_session);
      self->priv->soup_session = NULL;
    }

  /* Call superclass */
  G_OBJECT_CLASS (fsp_flickr_proxy_parent_class)->dispose(object);
}

static void
fsp_flickr_proxy_finalize               (GObject* object)
{
  FspFlickrProxy *self = FSP_FLICKR_PROXY (object);

  /* Free memory */
  g_free (self->priv->api_key);
  g_free (self->priv->secret);
  g_free (self->priv->token);

  /* Call superclass */
  G_OBJECT_CLASS (fsp_flickr_proxy_parent_class)->finalize(object);
}

static void
fsp_flickr_proxy_class_init             (FspFlickrProxyClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS(klass);
  GParamSpec *pspec = NULL;

  /* Install GOBject methods */
  obj_class->set_property = fsp_flickr_proxy_set_property;
  obj_class->get_property = fsp_flickr_proxy_get_property;
  obj_class->dispose = fsp_flickr_proxy_dispose;
  obj_class->finalize = fsp_flickr_proxy_finalize;

  /* Install properties */
  pspec = g_param_spec_string ("api-key", "api-key", "API key", NULL,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (obj_class, PROP_API_KEY, pspec);

  pspec = g_param_spec_string ("secret", "secret", "Shared secret", NULL,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  g_object_class_install_property (obj_class, PROP_SECRET, pspec);

  pspec = g_param_spec_string ("token", "token", "Authorized token", NULL,
                               G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_TOKEN, pspec);

  /* Add private */
  g_type_class_add_private (obj_class, sizeof (FspFlickrProxyPrivate));
}

static void
fsp_flickr_proxy_init                   (FspFlickrProxy *self)
{
  self->priv = FSP_FLICKR_PROXY_GET_PRIVATE (self);

  self->priv->parser = fsp_flickr_parser_get_instance ();
  self->priv->soup_session = soup_session_async_new ();
  self->priv->api_key = NULL;
  self->priv->secret = NULL;
  self->priv->token = NULL;
}

static gboolean
_check_errors_on_soup_response           (SoupMessage  *msg,
                                          GError      **error)
{
  g_assert (SOUP_IS_MESSAGE (msg));

  GError *err = NULL;

  /* Check non-succesful SoupMessage's only */
  if (!SOUP_STATUS_IS_SUCCESSFUL (msg->status_code))
    {
      if (msg->status_code == SOUP_STATUS_CANCELLED)
        err = g_error_new (FSP_ERROR, FSP_ERROR_CANCELLED,
                           "Cancelled by user");
      else if (SOUP_STATUS_IS_CLIENT_ERROR (msg->status_code))
        err = g_error_new (FSP_ERROR, FSP_ERROR_CLIENT_ERROR,
                           "Bad request");
      else if (SOUP_STATUS_IS_SERVER_ERROR (msg->status_code))
        err = g_error_new (FSP_ERROR, FSP_ERROR_SERVER_ERROR,
                           "Server error");
      else
        err = g_error_new (FSP_ERROR, FSP_ERROR_NETWORK_ERROR,
                           "Network error");
    }

  /* Propagate error */
  if (err != NULL)
    g_propagate_error (error, err);

  /* Return result */
  return (err != NULL);
}

static SoupMessage *
_get_soup_message_for_upload            (GFile       *file,
                                         const gchar *contents,
                                         gsize        length,
                                         GHashTable  *extra_params)
{
  GFileInfo *file_info = NULL;
  SoupMessage *msg = NULL;
  SoupMultipart *mpart = NULL;
  SoupBuffer *buffer = NULL;
  GHashTableIter iter;
  gpointer key, value;
  gchar *mime_type;
  gchar *filepath;
  gchar *fileuri;

  /* Gather needed information */
  filepath = g_file_get_path (file);
  file_info = g_file_query_info (file,
                                 G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                                 G_FILE_QUERY_INFO_NONE,
                                 NULL,
                                 NULL);
  /* Check mimetype */
  mime_type = g_strdup (g_file_info_get_content_type (file_info));

  /* Init multipart container */
  mpart = soup_multipart_new (SOUP_FORM_MIME_TYPE_MULTIPART);

  /* Traverse extra_params to append them to the message */
  g_hash_table_iter_init (&iter, extra_params);

  while (g_hash_table_iter_next (&iter, &key, &value))
    soup_multipart_append_form_string (mpart, key, value);

  /* Append the content of the file */
  buffer = soup_buffer_new (SOUP_MEMORY_TAKE, contents, length);
  fileuri = g_filename_to_uri (filepath, NULL, NULL);
  soup_multipart_append_form_file (mpart, "photo", fileuri,
                                   mime_type, buffer);
  g_free (fileuri);

  /* Get the associated message */
  msg = soup_form_request_new_from_multipart (FLICKR_API_UPLOAD_URL, mpart);

  /* Free */
  soup_multipart_free (mpart);
  g_free (filepath);
  g_free (mime_type);

  /* Return message */
  return msg;
}

static void
_load_file_contents_cb                  (GObject      *object,
                                         GAsyncResult *res,
                                         gpointer      data)
{
  g_return_if_fail (G_IS_FILE (object));
  g_return_if_fail (G_IS_ASYNC_RESULT (res));
  g_return_if_fail (data != NULL);

  UploadPhotoData *up_clos = NULL;
  GAsyncData *ga_clos = NULL;
  GHashTable *extra_params = NULL;
  GFile *file = NULL;
  GError *error = NULL;
  gchar *contents;
  gsize length;

  /* Get data from UploadPhotoData closure, and free it */
  up_clos = (UploadPhotoData *) data;
  ga_clos = up_clos->gasync_data;
  extra_params = up_clos->extra_params;
  g_slice_free (UploadPhotoData, up_clos);

  file = G_FILE (object);
  if (g_file_load_contents_finish (file, res, &contents, &length, NULL, &error))
    {
      FspFlickrProxy * proxy = NULL;
      SoupMessage *msg = NULL;

      /* Get the proxy and the associated message */
      proxy = FSP_FLICKR_PROXY (ga_clos->object);
      msg = _get_soup_message_for_upload (file, contents, length, extra_params);

      /* Connect to the "cancelled" signal thread safely */
      ga_clos->cancellable_id =
        g_cancellable_connect (ga_clos->cancellable,
                               G_CALLBACK (_upload_cancelled_cb),
                               proxy, NULL);

      /* Perform the async request */
      soup_session_queue_message (proxy->priv->soup_session,
                                  msg, _photo_upload_soup_session_cb, ga_clos);

      /* Free */
      g_hash_table_unref (extra_params);
      g_object_unref (file);
    }
  else
    {
      /* If an error happened here, report through the async callback */
      g_warning ("Unable to get contents for file\n");
      if (error)
        g_error_free (error);
      error = g_error_new (FSP_ERROR, FSP_ERROR_OTHER, "Error reading file for upload");
      build_async_result_and_complete (ga_clos, NULL, error);
    }
}

static void
_get_frob_soup_session_cb               (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer data)
{
  g_assert (SOUP_IS_SESSION (session));
  g_assert (SOUP_IS_MESSAGE (msg));
  g_assert (data != NULL);

  GAsyncData *clos = NULL;
  FspFlickrProxy *proxy = NULL;
  gchar *frob = NULL;
  GError *err = NULL;

  /* Get needed data from closure */
  clos = (GAsyncData *) data;
  proxy = FSP_FLICKR_PROXY (clos->object);

  /* Get value from response */
  if (!_check_errors_on_soup_response (msg, &err))
    {
      frob = fsp_flickr_parser_get_frob (proxy->priv->parser,
                                         msg->response_body->data,
                                         (int) msg->response_body->length,
                                         &err);
    }

  /* Build response and call async callback */
  build_async_result_and_complete (clos, (gpointer) frob, err);
}

static void
_get_auth_token_soup_session_cb         (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer data)
{
  g_assert (SOUP_IS_SESSION (session));
  g_assert (SOUP_IS_MESSAGE (msg));
  g_assert (data != NULL);

  GAsyncData *clos = NULL;
  FspFlickrProxy *proxy = NULL;
  FspDataAuthToken *auth_token = NULL;
  GError *err = NULL;

  /* Get needed data from closure */
  clos = (GAsyncData *) data;
  proxy = FSP_FLICKR_PROXY (clos->object);

  /* Get value from response */
  if (!_check_errors_on_soup_response (msg, &err))
    {
      auth_token =
        fsp_flickr_parser_get_auth_token (proxy->priv->parser,
                                          msg->response_body->data,
                                          (int) msg->response_body->length,
                                          &err);
    }

  /* Build response and call async callback */
  build_async_result_and_complete (clos, (gpointer) auth_token, err);
}

static void
_photo_upload_soup_session_cb           (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer     data)
{
  g_assert (SOUP_IS_SESSION (session));
  g_assert (SOUP_IS_MESSAGE (msg));
  g_assert (data != NULL);

  GAsyncData *clos = NULL;
  FspFlickrProxy *proxy = NULL;
  gchar *photo_id = NULL;
  GError *err = NULL;

  /* Get needed data from closure */
  clos = (GAsyncData *) data;
  proxy = FSP_FLICKR_PROXY (clos->object);

  /* Get value from response */
  if (!_check_errors_on_soup_response (msg, &err))
    {
      photo_id =
        fsp_flickr_parser_get_upload_result (proxy->priv->parser,
                                             msg->response_body->data,
                                             (int) msg->response_body->length,
                                             &err);
    }

  /* Build response and call async callback */
  build_async_result_and_complete (clos, photo_id, err);
}

static void
_photo_get_info_soup_session_cb         (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer     data)
{
  g_assert (SOUP_IS_SESSION (session));
  g_assert (SOUP_IS_MESSAGE (msg));
  g_assert (data != NULL);

  GAsyncData *clos = NULL;
  FspFlickrProxy *proxy = NULL;
  FspDataPhotoInfo *photo_info = NULL;
  GError *err = NULL;

  /* Get needed data from closure */
  clos = (GAsyncData *) data;
  proxy = FSP_FLICKR_PROXY (clos->object);

  /* Get value from response */
  if (!_check_errors_on_soup_response (msg, &err))
    {
      photo_info =
        fsp_flickr_parser_get_photo_info (proxy->priv->parser,
                                          msg->response_body->data,
                                          (int) msg->response_body->length,
                                          &err);
    }

  /* Build response and call async callback */
  build_async_result_and_complete (clos, photo_info, err);
}

static void
_perform_async_request                  (FspFlickrProxy      *self,
                                         const gchar         *url,
                                         SoupSessionCallback  request_cb,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             source_tag,
                                         gpointer             data)
{
  g_return_if_fail (FSP_IS_FLICKR_PROXY (self));
  g_return_if_fail (url != NULL);
  g_return_if_fail (request_cb != NULL);
  g_return_if_fail (callback != NULL);

  GAsyncData *clos = NULL;
  SoupMessage *msg = NULL;

  /* Save important data for the callback */
  clos = g_slice_new0 (GAsyncData);
  clos->object = G_OBJECT (self);
  clos->cancellable = cancellable;
  clos->callback = callback;
  clos->source_tag = source_tag;
  clos->data = data;

  /* Build and queue the message */
  msg = soup_message_new (SOUP_METHOD_GET, url);
  soup_session_queue_message (self->priv->soup_session,
                              msg, request_cb, clos);
}

static void
_upload_cancelled_cb                    (GCancellable *cancellable,
                                         gpointer      data)
{
  FspFlickrProxy *proxy = FSP_FLICKR_PROXY (data);
  soup_session_abort (proxy->priv->soup_session);
}

/* Public API */

FspFlickrProxy *
fsp_flickr_proxy_new                    (const gchar *api_key,
                                         const gchar *secret,
                                         const gchar *token)
{
  g_return_val_if_fail (api_key != NULL, NULL);
  g_return_val_if_fail (secret != NULL, NULL);

  GObject *object = g_object_new(FSP_TYPE_FLICKR_PROXY,
                                 "api-key", api_key,
                                 "secret", secret,
                                 "token", token,
                                 NULL);
  return FSP_FLICKR_PROXY (object);
}

const gchar *
fsp_flickr_proxy_get_api_key            (FspFlickrProxy *self)
{
  g_return_val_if_fail (FSP_IS_FLICKR_PROXY (self), NULL);

  return self->priv->api_key;
}

const gchar *
fsp_flickr_proxy_get_secret             (FspFlickrProxy *self)
{
  g_return_val_if_fail (FSP_IS_FLICKR_PROXY (self), NULL);

  return self->priv->secret;
}

const gchar *
fsp_flickr_proxy_get_token              (FspFlickrProxy *self)
{
  g_return_val_if_fail (FSP_IS_FLICKR_PROXY (self), NULL);

  return self->priv->token;
}

void
fsp_flickr_proxy_set_token              (FspFlickrProxy *self,
                                         const gchar    *token)
{
  g_return_if_fail (FSP_IS_FLICKR_PROXY (self));

  g_free (self->priv->token);
  self->priv->token = g_strdup (token);
}

void
fsp_flickr_proxy_get_frob_async         (FspFlickrProxy      *self,
                                         GCancellable        *c,
                                         GAsyncReadyCallback  cb,
                                         gpointer             data)
{
  g_return_if_fail (FSP_IS_FLICKR_PROXY (self));
  g_return_if_fail (cb != NULL);

  gchar *url = NULL;
  gchar *signed_query = NULL;

  /* Build the signed url */
  signed_query = get_signed_query (self->priv->secret,
                                   "method", "flickr.auth.getFrob",
                                   "api_key", self->priv->api_key,
                                   NULL);
  url = g_strdup_printf ("%s/?%s", FLICKR_API_BASE_URL, signed_query);
  g_free (signed_query);

  /* Perform the async request */
  _perform_async_request (self, url, _get_frob_soup_session_cb, c, cb,
                          fsp_flickr_proxy_get_frob_async, data);

  /* Free */
  g_free (url);
}

gchar *
fsp_flickr_proxy_get_frob_finish        (FspFlickrProxy  *self,
                                         GAsyncResult    *res,
                                         GError         **error)
{
  g_return_val_if_fail (FSP_IS_FLICKR_PROXY (self), NULL);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), NULL);

  gchar *frob = NULL;

  /* Check for errors */
  if (!check_async_errors_on_finish (G_OBJECT (self), res,
                                     fsp_flickr_proxy_get_frob_async, error))
    {
      GSimpleAsyncResult *simple = NULL;
      gpointer result = NULL;

      /* Get result */
      simple = G_SIMPLE_ASYNC_RESULT (res);
      result = g_simple_async_result_get_op_res_gpointer (simple);
      if (result != NULL)
        frob = (gchar *) result;
      else
        g_set_error_literal (error, FSP_ERROR, FSP_ERROR_OTHER,
                             "Internal error");
    }

  return frob;
}

void
fsp_flickr_proxy_get_auth_token_async   (FspFlickrProxy      *self,
                                         const gchar         *frob,
                                         GCancellable        *c,
                                         GAsyncReadyCallback  cb,
                                         gpointer             data)
{
  g_return_if_fail (FSP_IS_FLICKR_PROXY (self));
  g_return_if_fail (frob != NULL);
  g_return_if_fail (cb != NULL);

  gchar *signed_query = NULL;
  gchar *url = NULL;

  /* Build the signed url */
  signed_query = get_signed_query (self->priv->secret,
                                   "method", "flickr.auth.getToken",
                                   "api_key", self->priv->api_key,
                                   "frob", frob,
                                   NULL);
  url = g_strdup_printf ("%s/?%s", FLICKR_API_BASE_URL, signed_query);
  g_free (signed_query);

  /* Perform the async request */
  _perform_async_request (self, url, _get_auth_token_soup_session_cb, c, cb,
                          fsp_flickr_proxy_get_auth_token_async, data);

  g_free (url);
}

FspDataAuthToken *
fsp_flickr_proxy_get_auth_token_finish  (FspFlickrProxy  *self,
                                         GAsyncResult    *res,
                                         GError         **error)
{
  g_return_val_if_fail (FSP_IS_FLICKR_PROXY (self), NULL);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), NULL);

  FspDataAuthToken *auth_token = NULL;

  /* Check for errors */
  if (!check_async_errors_on_finish (G_OBJECT (self), res,
                                     fsp_flickr_proxy_get_auth_token_async, error))
    {
      GSimpleAsyncResult *simple = NULL;
      gpointer result = NULL;

      /* Get result */
      simple = G_SIMPLE_ASYNC_RESULT (res);
      result = g_simple_async_result_get_op_res_gpointer (simple);
      if (result != NULL)
        auth_token = FSP_DATA_AUTH_TOKEN (result);
      else
        g_set_error_literal (error, FSP_ERROR, FSP_ERROR_OTHER,
                             "Internal error");

    }

  return auth_token;
}

void
fsp_flickr_proxy_photo_upload_async     (FspFlickrProxy      *self,
                                         const gchar         *filepath,
                                         GHashTable          *extra_params,
                                         GCancellable        *c,
                                         GAsyncReadyCallback  cb,
                                         gpointer             data)
{
  g_return_if_fail (FSP_IS_FLICKR_PROXY (self));
  g_return_if_fail (filepath != NULL);
  g_return_if_fail (cb != NULL);

  GAsyncData *ga_clos = NULL;
  UploadPhotoData *up_clos = NULL;
  GFile *file = NULL;
  gchar *api_sig = NULL;

  /* Save important data for the callback */
  ga_clos = g_slice_new0 (GAsyncData);
  ga_clos->object = G_OBJECT (self);
  ga_clos->cancellable = c;
  ga_clos->callback = cb;
  ga_clos->source_tag = fsp_flickr_proxy_photo_upload_async;
  ga_clos->data = data;

  /* Get ownership of the table */
  g_hash_table_ref (extra_params);

  /* Add remaining parameters to the hash table */
  g_hash_table_insert (extra_params, g_strdup ("api_key"),
                       g_strdup (self->priv->api_key));
  g_hash_table_insert (extra_params, g_strdup ("auth_token"),
                       g_strdup (self->priv->token));

  /* Build the api signature and add it to the hash table */
  api_sig = get_api_signature_from_hash_table (self->priv->secret, extra_params);
  g_hash_table_insert (extra_params, g_strdup ("api_sig"), api_sig);

  /* Save important data for the upload process itself */
  up_clos = g_slice_new0 (UploadPhotoData);
  up_clos->gasync_data = ga_clos;
  up_clos->extra_params = extra_params;

  /* Asynchronously load the contents of the file */
  file = g_file_new_for_path (filepath);
  g_file_load_contents_async (file, c, _load_file_contents_cb, up_clos);
}

gchar *
fsp_flickr_proxy_photo_upload_finish    (FspFlickrProxy *self,
                                         GAsyncResult   *res,
                                         GError        **error)
{
  g_return_val_if_fail (FSP_IS_FLICKR_PROXY (self), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), FALSE);

  gchar *retval = NULL;

  /* Check for errors */
  if (!check_async_errors_on_finish (G_OBJECT (self), res,
                                     fsp_flickr_proxy_photo_upload_async, error))
    {
      GSimpleAsyncResult *simple = NULL;
      gpointer result = NULL;

      /* Get result */
      simple = G_SIMPLE_ASYNC_RESULT (res);
      result = g_simple_async_result_get_op_res_gpointer (simple);
      if (result != NULL)
        retval = (gchar *) result;
      else
        g_set_error_literal (error, FSP_ERROR, FSP_ERROR_OTHER,
                             "Internal error");
    }

  return retval;
}

void
fsp_flickr_proxy_photo_get_info_async   (FspFlickrProxy      *self,
                                         const gchar         *photo_id,
                                         GCancellable        *c,
                                         GAsyncReadyCallback  cb,
                                         gpointer             data)
{
  g_return_if_fail (FSP_IS_FLICKR_PROXY (self));
  g_return_if_fail (photo_id != NULL);
  g_return_if_fail (cb != NULL);

  gchar *signed_query = NULL;
  gchar *url = NULL;

  /* Build the signed url */
  signed_query = get_signed_query (self->priv->secret,
                                   "method", "flickr.photos.getInfo",
                                   "api_key", self->priv->api_key,
                                   "auth_token", self->priv->token,
                                   "photo_id", photo_id,
                                   NULL);
  url = g_strdup_printf ("%s/?%s", FLICKR_API_BASE_URL, signed_query);
  g_free (signed_query);

  /* Perform the async request */
  _perform_async_request (self, url, _photo_get_info_soup_session_cb, c, cb,
                          fsp_flickr_proxy_photo_get_info_async, data);

  g_free (url);
}

FspDataPhotoInfo *
fsp_flickr_proxy_photo_get_info_finish  (FspFlickrProxy *self,
                                         GAsyncResult   *res,
                                         GError        **error)
{
  g_return_val_if_fail (FSP_IS_FLICKR_PROXY (self), NULL);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), NULL);

  FspDataPhotoInfo *retval = NULL;

  /* Check for errors */
  if (!check_async_errors_on_finish (G_OBJECT (self), res,
                                     fsp_flickr_proxy_photo_get_info_async, error))
    {
      GSimpleAsyncResult *simple = NULL;
      gpointer result = NULL;

      /* Get result */
      simple = G_SIMPLE_ASYNC_RESULT (res);
      result = g_simple_async_result_get_op_res_gpointer (simple);
      if (result != NULL)
        retval = FSP_DATA_PHOTO_INFO (result);
      else
        g_set_error_literal (error, FSP_ERROR, FSP_ERROR_OTHER,
                             "Internal error");
    }

  return retval;
}
