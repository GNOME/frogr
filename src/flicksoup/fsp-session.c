/*
 * fsp-session.c
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

#include "fsp-session.h"

#include "fsp-data.h"
#include "fsp-error.h"
#include "fsp-parser.h"
#include "fsp-session.h"
#include "fsp-util.h"

#include <libsoup/soup.h>
#include <string.h>

#define FLICKR_API_BASE_URL   "http://api.flickr.com/services/rest"
#define FLICKR_API_UPLOAD_URL "http://api.flickr.com/services/upload"

#define FSP_SESSION_GET_PRIVATE(object)                 \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),               \
                                FSP_TYPE_SESSION,       \
                                FspSessionPrivate))

G_DEFINE_TYPE (FspSession, fsp_session, G_TYPE_OBJECT);


/* Private struct */
struct _FspSessionPrivate
{
  gchar *api_key;
  gchar *secret;
  gchar *token;
  gchar *frob;

  SoupSession *soup_session;
};


typedef struct
{
  AsyncRequestData *gasync_data;
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

static SoupSession *
_get_soup_session                       (FspSession *self);

static void
_get_frob_soup_session_cb               (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer     data);
static void
_get_auth_token_soup_session_cb         (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer     data);
static void
_get_upload_status_soup_session_cb      (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer     data);

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
_photo_upload_soup_session_cb           (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer     data);

static void
_photo_get_info_soup_session_cb         (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer     data);

static void
_get_photosets_soup_session_cb          (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer     data);

static void
_add_to_photoset_soup_session_cb        (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer     data);

static void
_create_photoset_soup_session_cb        (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer     data);

static void
_get_groups_soup_session_cb             (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer     data);

static void
_add_to_group_soup_session_cb           (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer     data);

static void
_get_tags_list_soup_session_cb           (SoupSession *session,
                                          SoupMessage *msg,
                                          gpointer     data);


/* Private API */

static void
fsp_session_set_property                (GObject      *object,
                                         guint         prop_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
  FspSession *self = FSP_SESSION (object);
  FspSessionPrivate *priv = self->priv;

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
      fsp_session_set_token (self, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
fsp_session_get_property                (GObject    *object,
                                         guint       prop_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
  FspSession *self = FSP_SESSION (object);

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
fsp_session_dispose                     (GObject* object)
{
  FspSession *self = FSP_SESSION (object);

  /* Unref object */
  if (self->priv->soup_session)
    {
      g_object_unref (self->priv->soup_session);
      self->priv->soup_session = NULL;
    }

  /* Call superclass */
  G_OBJECT_CLASS (fsp_session_parent_class)->dispose(object);
}

static void
fsp_session_finalize                    (GObject* object)
{
  FspSession *self = FSP_SESSION (object);

  /* Free memory */
  g_free (self->priv->api_key);
  g_free (self->priv->secret);
  g_free (self->priv->token);

  /* Call superclass */
  G_OBJECT_CLASS (fsp_session_parent_class)->finalize(object);
}

static void
fsp_session_class_init                  (FspSessionClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS(klass);
  GParamSpec *pspec = NULL;

  /* Install GOBject methods */
  obj_class->set_property = fsp_session_set_property;
  obj_class->get_property = fsp_session_get_property;
  obj_class->dispose = fsp_session_dispose;
  obj_class->finalize = fsp_session_finalize;

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
  g_type_class_add_private (obj_class, sizeof (FspSessionPrivate));
}

static void
fsp_session_init                        (FspSession *self)
{
  self->priv = FSP_SESSION_GET_PRIVATE (self);

  self->priv->api_key = NULL;
  self->priv->secret = NULL;
  self->priv->token = NULL;
  self->priv->frob = NULL;

  self->priv->soup_session = soup_session_async_new ();
}

static SoupSession *
_get_soup_session                       (FspSession *self)
{
  g_return_val_if_fail (FSP_IS_SESSION (self), NULL);
  return self->priv->soup_session;
}

static void
_get_frob_soup_session_cb               (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer data)
{
  g_assert (SOUP_IS_MESSAGE (msg));
  g_assert (data != NULL);

  /* Handle message with the right parser */
  handle_soup_response (msg,
                        (FspParserFunc) fsp_parser_get_frob,
                        data);
}

static void
_get_auth_token_soup_session_cb         (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer data)
{
  g_assert (SOUP_IS_MESSAGE (msg));
  g_assert (data != NULL);

  /* Handle message with the right parser */
  handle_soup_response (msg,
                        (FspParserFunc) fsp_parser_get_auth_token,
                        data);
}

static void
_get_upload_status_soup_session_cb      (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer     data)
{
  g_assert (SOUP_IS_MESSAGE (msg));
  g_assert (data != NULL);

  /* Handle message with the right parser */
  handle_soup_response (msg,
                        (FspParserFunc) fsp_parser_get_upload_status,
                        data);
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
  AsyncRequestData *ga_clos = NULL;
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
      FspSession *self = NULL;
      SoupSession *soup_session = NULL;
      SoupMessage *msg = NULL;

      /* Get the proxy and the associated message */
      self = FSP_SESSION (ga_clos->object);
      msg = _get_soup_message_for_upload (file, contents, length, extra_params);
      ga_clos->soup_message = msg;

      soup_session = _get_soup_session (self);

      /* Connect to the "cancelled" signal thread safely */
      if (ga_clos->cancellable)
        {
          ga_clos->cancellable_id =
            g_cancellable_connect (ga_clos->cancellable,
                                   G_CALLBACK (soup_session_cancelled_cb),
                                   ga_clos,
                                   NULL);
        }

      /* Perform the async request */
      soup_session_queue_message (soup_session, msg,
                                  _photo_upload_soup_session_cb, ga_clos);

      g_debug ("\nRequested URL:\n%s\n", FLICKR_API_UPLOAD_URL);

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
_photo_upload_soup_session_cb           (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer     data)
{
  g_assert (SOUP_IS_MESSAGE (msg));
  g_assert (data != NULL);

  /* Handle message with the right parser */
  handle_soup_response (msg,
                        (FspParserFunc) fsp_parser_get_upload_result,
                        data);
}

static void
_photo_get_info_soup_session_cb         (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer     data)
{
  g_assert (SOUP_IS_MESSAGE (msg));
  g_assert (data != NULL);

  /* Handle message with the right parser */
  handle_soup_response (msg,
                        (FspParserFunc) fsp_parser_get_photo_info,
                        data);
}

static void
_get_photosets_soup_session_cb          (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer     data)
{
  g_assert (SOUP_IS_MESSAGE (msg));
  g_assert (data != NULL);

  /* Handle message with the right parser */
  handle_soup_response (msg,
                        (FspParserFunc) fsp_parser_get_photosets_list,
                        data);
}

static void
_add_to_photoset_soup_session_cb        (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer     data)
{
  g_assert (SOUP_IS_MESSAGE (msg));
  g_assert (data != NULL);

  /* Handle message with the right parser */
  handle_soup_response (msg,
                        (FspParserFunc) fsp_parser_added_to_photoset,
                        data);
}

static void
_create_photoset_soup_session_cb        (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer     data)
{
  g_assert (SOUP_IS_MESSAGE (msg));
  g_assert (data != NULL);

  /* Handle message with the right parser */
  handle_soup_response (msg,
                        (FspParserFunc) fsp_parser_photoset_created,
                        data);
}

static void
_get_groups_soup_session_cb             (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer     data)
{
  g_assert (SOUP_IS_MESSAGE (msg));
  g_assert (data != NULL);

  /* Handle message with the right parser */
  handle_soup_response (msg,
                        (FspParserFunc) fsp_parser_get_groups_list,
                        data);
}

static void
_add_to_group_soup_session_cb           (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer     data)
{
  g_assert (SOUP_IS_MESSAGE (msg));
  g_assert (data != NULL);

  /* Handle message with the right parser */
  handle_soup_response (msg,
                        (FspParserFunc) fsp_parser_added_to_group,
                        data);
}

static void
_get_tags_list_soup_session_cb           (SoupSession *session,
                                          SoupMessage *msg,
                                          gpointer     data)
{
  g_assert (SOUP_IS_MESSAGE (msg));
  g_assert (data != NULL);

  /* Handle message with the right parser */
  handle_soup_response (msg,
                        (FspParserFunc) fsp_parser_get_tags_list,
                        data);
}



/* Public API */

FspSession *
fsp_session_new                         (const gchar *api_key,
                                         const gchar *secret,
                                         const gchar *token)
{
  g_return_val_if_fail (api_key != NULL, NULL);
  g_return_val_if_fail (secret != NULL, NULL);

  GObject *object = g_object_new(FSP_TYPE_SESSION,
                                 "api-key", api_key,
                                 "secret", secret,
                                 "token", token,
                                 NULL);
  return FSP_SESSION (object);
}

void
fsp_session_set_http_proxy              (FspSession *self,
                                         const gchar *proxy_address)
{
  g_return_if_fail (FSP_IS_SESSION (self));

  SoupURI *proxy_uri = NULL;
  if (proxy_address != NULL)
    {
      gchar *lowercase_address = NULL;
      gchar *proxy_uri_str = NULL;

      lowercase_address = g_ascii_strdown (proxy_address, -1);

      /* Check whether the 'http://' prefix is present */
      if (g_str_has_prefix (lowercase_address, "http://"))
        proxy_uri_str = g_strdup (proxy_address);
      else
        proxy_uri_str = g_strdup_printf ("http://%s", proxy_address);

      g_free (lowercase_address);

      proxy_uri = soup_uri_new (proxy_uri_str);
      g_free (proxy_uri_str);
    }

  /* Set/unset the proxy */
  g_object_set (G_OBJECT (self->priv->soup_session),
                SOUP_SESSION_PROXY_URI,
                proxy_uri,
                NULL);
}

const gchar *
fsp_session_get_api_key                 (FspSession *self)
{
  g_return_val_if_fail (FSP_IS_SESSION (self), NULL);

  return self->priv->api_key;
}

const gchar *
fsp_session_get_secret                  (FspSession *self)
{
  g_return_val_if_fail (FSP_IS_SESSION (self), NULL);

  return self->priv->secret;
}

const gchar *
fsp_session_get_token                   (FspSession *self)
{
  g_return_val_if_fail (FSP_IS_SESSION (self), NULL);

  return self->priv->token;
}

void
fsp_session_set_token                   (FspSession  *self,
                                         const gchar *token)
{
  g_return_if_fail (FSP_IS_SESSION (self));

  g_free (self->priv->token);
  self->priv->token = g_strdup (token);
}

/* Get authorization URL */

void
fsp_session_get_auth_url_async          (FspSession          *self,
                                         GCancellable        *c,
                                         GAsyncReadyCallback  cb,
                                         gpointer             data)
{
  g_return_if_fail (FSP_IS_SESSION (self));

  FspSessionPrivate *priv = self->priv;
  gchar *url = NULL;
  gchar *signed_query = NULL;

  /* Build the signed url */
  signed_query = get_signed_query (priv->secret,
                                   "method", "flickr.auth.getFrob",
                                   "api_key", priv->api_key,
                                   NULL);
  url = g_strdup_printf ("%s/?%s", FLICKR_API_BASE_URL, signed_query);
  g_free (signed_query);

  /* Perform the async request */
  perform_async_request (priv->soup_session, url,
                         _get_frob_soup_session_cb, G_OBJECT (self),
                         c, cb, fsp_session_get_auth_url_async, data);

  g_free (url);
}

gchar *
fsp_session_get_auth_url_finish         (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error)
{
  g_return_val_if_fail (FSP_IS_SESSION (self), NULL);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), NULL);

  gchar *frob = NULL;
  gchar *auth_url = NULL;

  frob = (gchar*) finish_async_request (G_OBJECT (self), res,
                                        fsp_session_get_auth_url_async, error);

  /* Build the auth URL from the frob */
  if (frob != NULL)
    {
      FspSessionPrivate *priv = self->priv;
      gchar *signed_query = NULL;

      /* Save the frob */
      priv->frob = frob;

      /* Build the authorization url */
      signed_query = get_signed_query (priv->secret,
                                       "api_key", priv->api_key,
                                       "perms", "write",
                                       "frob", priv->frob,
                                       NULL);
      auth_url = g_strdup_printf ("http://flickr.com/services/auth/?%s",
                                  signed_query);
      g_free (signed_query);
    }

  return auth_url;
}

/* Complete authorization */

void
fsp_session_complete_auth_async         (FspSession          *self,
                                         GCancellable        *c,
                                         GAsyncReadyCallback  cb,
                                         gpointer             data)
{
  g_return_if_fail (FSP_IS_SESSION (self));
  g_return_if_fail (cb != NULL);

  FspSessionPrivate *priv = self->priv;

  if (priv->frob != NULL)
    {
      gchar *signed_query = NULL;
      gchar *url = NULL;

      /* Build the signed url */
      signed_query = get_signed_query (priv->secret,
                                       "method", "flickr.auth.getToken",
                                       "api_key", priv->api_key,
                                       "frob", priv->frob,
                                       NULL);
      url = g_strdup_printf ("%s/?%s", FLICKR_API_BASE_URL, signed_query);
      g_free (signed_query);

      /* Perform the async request */
      perform_async_request (priv->soup_session, url,
                             _get_auth_token_soup_session_cb, G_OBJECT (self),
                             c, cb, fsp_session_complete_auth_async, data);

      g_free (url);
    }
  else
    {
      GError *err = NULL;

      /* Build and report error */
      err = g_error_new (FSP_ERROR, FSP_ERROR_NOFROB, "No frob defined");
      g_simple_async_report_gerror_in_idle (G_OBJECT (self),
                                            cb, data, err);
    }
}

FspDataAuthToken *
fsp_session_complete_auth_finish        (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error)
{
  g_return_val_if_fail (FSP_IS_SESSION (self), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), FALSE);

  FspDataAuthToken *auth_token = NULL;

  auth_token =
    FSP_DATA_AUTH_TOKEN (finish_async_request (G_OBJECT (self), res,
                                               fsp_session_complete_auth_async,
                                               error));

  /* Complete the authorization saving the token if present */
  if (auth_token != NULL && auth_token->token != NULL)
    fsp_session_set_token (self, auth_token->token);

  return auth_token;
}

void
fsp_session_check_auth_info_async       (FspSession          *self,
                                         GCancellable        *c,
                                         GAsyncReadyCallback  cb,
                                         gpointer             data)
{
  g_return_if_fail (FSP_IS_SESSION (self));
  g_return_if_fail (cb != NULL);

  FspSessionPrivate *priv = self->priv;

  if (priv->token != NULL)
    {
      gchar *signed_query = NULL;
      gchar *url = NULL;

      /* Build the signed url */
      signed_query = get_signed_query (priv->secret,
                                       "method", "flickr.auth.checkToken",
                                       "api_key", priv->api_key,
                                       "auth_token", priv->token,
                                       NULL);
      url = g_strdup_printf ("%s/?%s", FLICKR_API_BASE_URL, signed_query);
      g_free (signed_query);

      /* Perform the async request */
      perform_async_request (priv->soup_session, url,
                             _get_auth_token_soup_session_cb, G_OBJECT (self),
                             c, cb, fsp_session_check_auth_info_async, data);

      g_free (url);
    }
  else
    {
      GError *err = NULL;

      /* Build and report error */
      err = g_error_new (FSP_ERROR, FSP_ERROR_NOT_AUTHENTICATED, "No authenticated");
      g_simple_async_report_gerror_in_idle (G_OBJECT (self),
                                            cb, data, err);
    }
}

FspDataAuthToken *
fsp_session_check_auth_info_finish      (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error)
{
  g_return_val_if_fail (FSP_IS_SESSION (self), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), FALSE);

  FspDataAuthToken *auth_token = NULL;

  auth_token =
    FSP_DATA_AUTH_TOKEN (finish_async_request (G_OBJECT (self), res,
                                               fsp_session_check_auth_info_async,
                                               error));
  return auth_token;
}

void
fsp_session_get_upload_status_async     (FspSession          *self,
                                         GCancellable        *c,
                                         GAsyncReadyCallback cb,
                                         gpointer             data)
{
  g_return_if_fail (FSP_IS_SESSION (self));
  g_return_if_fail (cb != NULL);

  FspSessionPrivate *priv = self->priv;

  if (priv->token != NULL)
    {
      gchar *signed_query = NULL;
      gchar *url = NULL;

      /* Build the signed url */
      signed_query = get_signed_query (priv->secret,
                                       "method", "flickr.people.getUploadStatus",
                                       "api_key", priv->api_key,
                                       "auth_token", priv->token,
                                       NULL);
      url = g_strdup_printf ("%s/?%s", FLICKR_API_BASE_URL, signed_query);
      g_free (signed_query);

      /* Perform the async request */
      perform_async_request (priv->soup_session, url,
                             _get_upload_status_soup_session_cb, G_OBJECT (self),
                             c, cb, fsp_session_get_upload_status_async, data);

      g_free (url);
    }
  else
    {
      GError *err = NULL;

      /* Build and report error */
      err = g_error_new (FSP_ERROR, FSP_ERROR_NOT_AUTHENTICATED, "No authenticated");
      g_simple_async_report_gerror_in_idle (G_OBJECT (self),
                                            cb, data, err);
    }
}

FspDataUploadStatus *
fsp_session_get_upload_status_finish    (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error)
{
  g_return_val_if_fail (FSP_IS_SESSION (self), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), FALSE);

  FspDataUploadStatus *upload_status = NULL;

  upload_status =
    FSP_DATA_UPLOAD_STATUS (finish_async_request (G_OBJECT (self), res,
                                                  fsp_session_get_upload_status_async,
                                                  error));
  return upload_status;
}

void
fsp_session_upload_async                (FspSession          *self,
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
  g_return_if_fail (FSP_IS_SESSION (self));
  g_return_if_fail (filepath != NULL);

  FspSessionPrivate *priv = NULL;
  SoupSession *soup_session = NULL;
  GHashTable *extra_params = NULL;
  AsyncRequestData *ga_clos = NULL;
  UploadPhotoData *up_clos = NULL;
  GFile *file = NULL;
  gchar *api_sig = NULL;

  /* Get flickr proxy and extra params (those actually used) */
  priv = self->priv;
  soup_session = _get_soup_session (self);
  extra_params = _get_upload_extra_params (title, description, tags,
                                           is_public, is_family, is_friend,
                                           safety_level, content_type, hidden);

  /* Add remaining parameters to the hash table */
  g_hash_table_insert (extra_params, g_strdup ("api_key"),
                       g_strdup (priv->api_key));
  g_hash_table_insert (extra_params, g_strdup ("auth_token"),
                       g_strdup (priv->token));

  /* Build the api signature and add it to the hash table */
  api_sig = get_api_signature_from_hash_table (priv->secret, extra_params);
  g_hash_table_insert (extra_params, g_strdup ("api_sig"), api_sig);

  /* Save important data for the callback */
  ga_clos = g_slice_new0 (AsyncRequestData);
  ga_clos->object = G_OBJECT (self);
  ga_clos->soup_session = soup_session;
  ga_clos->soup_message = NULL;
  ga_clos->cancellable = cancellable;
  ga_clos->callback = callback;
  ga_clos->source_tag = fsp_session_upload_async;
  ga_clos->data = data;

  /* Save important data for the upload process itself */
  up_clos = g_slice_new0 (UploadPhotoData);
  up_clos->gasync_data = ga_clos;
  up_clos->extra_params = extra_params;

  /* Asynchronously load the contents of the file */
  file = g_file_new_for_path (filepath);
  g_file_load_contents_async (file, NULL, _load_file_contents_cb, up_clos);
}

gchar *
fsp_session_upload_finish               (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error)
{
  g_return_val_if_fail (FSP_IS_SESSION (self), NULL);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), NULL);

  gchar *photo_id = (gchar*) finish_async_request (G_OBJECT (self), res,
                                                   fsp_session_upload_async,
                                                   error);
  return photo_id;
}

void
fsp_session_get_info_async              (FspSession          *self,
                                         const gchar         *photo_id,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             data)
{
  g_return_if_fail (FSP_IS_SESSION (self));
  g_return_if_fail (photo_id != NULL);

  FspSessionPrivate *priv = self->priv;
  SoupSession *soup_session = NULL;
  gchar *url = NULL;
  gchar *signed_query = NULL;

  /* Build the signed url */
  signed_query = get_signed_query (priv->secret,
                                   "method", "flickr.photos.getInfo",
                                   "api_key", priv->api_key,
                                   "auth_token", priv->token,
                                   "photo_id", photo_id,
                                   NULL);

  url = g_strdup_printf ("%s/?%s", FLICKR_API_BASE_URL, signed_query);
  g_free (signed_query);

  /* Perform the async request */
  soup_session = _get_soup_session (self);
  perform_async_request (soup_session, url,
                         _photo_get_info_soup_session_cb, G_OBJECT (self),
                         cancellable, callback, fsp_session_get_info_async, data);

  g_free (url);
}

FspDataPhotoInfo *
fsp_session_get_info_finish             (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error)
{
  g_return_val_if_fail (FSP_IS_SESSION (self), NULL);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), NULL);

  FspDataPhotoInfo *photo_info = NULL;

  photo_info =
    FSP_DATA_PHOTO_INFO (finish_async_request (G_OBJECT (self), res,
                                               fsp_session_get_info_async,
                                               error));
  return photo_info;
}

void
fsp_session_get_photosets_async         (FspSession          *self,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             data)
{
  g_return_if_fail (FSP_IS_SESSION (self));

  FspSessionPrivate *priv = self->priv;
  SoupSession *soup_session = NULL;
  gchar *url = NULL;
  gchar *signed_query = NULL;

  /* Build the signed url */
  signed_query = get_signed_query (priv->secret,
                                   "method", "flickr.photosets.getList",
                                   "api_key", priv->api_key,
                                   "auth_token", priv->token,
                                   NULL);

  url = g_strdup_printf ("%s/?%s", FLICKR_API_BASE_URL, signed_query);
  g_free (signed_query);

  /* Perform the async request */
  soup_session = _get_soup_session (self);
  perform_async_request (soup_session, url,
                         _get_photosets_soup_session_cb, G_OBJECT (self),
                         cancellable, callback, fsp_session_get_photosets_async, data);

  g_free (url);
}

GSList *
fsp_session_get_photosets_finish        (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error)
{
  g_return_val_if_fail (FSP_IS_SESSION (self), NULL);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), NULL);

  GSList *photosets_list = NULL;

  photosets_list = (GSList*) finish_async_request (G_OBJECT (self), res,
                                                   fsp_session_get_photosets_async,
                                                   error);
  return photosets_list;
}

void
fsp_session_add_to_photoset_async       (FspSession          *self,
                                         const gchar         *photo_id,
                                         const gchar         *photoset_id,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             data)
{
  g_return_if_fail (FSP_IS_SESSION (self));
  g_return_if_fail (photo_id != NULL);
  g_return_if_fail (photoset_id != NULL);

  FspSessionPrivate *priv = self->priv;
  SoupSession *soup_session = NULL;
  gchar *url = NULL;
  gchar *signed_query = NULL;

  /* Build the signed url */
  signed_query = get_signed_query (priv->secret,
                                   "method", "flickr.photosets.addPhoto",
                                   "api_key", priv->api_key,
                                   "auth_token", priv->token,
                                   "photo_id", photo_id,
                                   "photoset_id", photoset_id,
                                   NULL);

  url = g_strdup_printf ("%s/?%s", FLICKR_API_BASE_URL, signed_query);
  g_free (signed_query);

  /* Perform the async request */
  soup_session = _get_soup_session (self);
  perform_async_request (soup_session, url,
                         _add_to_photoset_soup_session_cb, G_OBJECT (self),
                         cancellable, callback, fsp_session_add_to_photoset_async, data);

  g_free (url);
}

gboolean
fsp_session_add_to_photoset_finish      (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error)
{
  g_return_val_if_fail (FSP_IS_SESSION (self), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), FALSE);

  gpointer result = NULL;

  result = finish_async_request (G_OBJECT (self), res,
                                 fsp_session_add_to_photoset_async, error);

  return result ? TRUE : FALSE;
}

void
fsp_session_create_photoset_async       (FspSession          *self,
                                         const gchar         *title,
                                         const gchar         *description,
                                         const gchar         *primary_photo_id,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             data)
{
  g_return_if_fail (FSP_IS_SESSION (self));
  g_return_if_fail (title != NULL);
  g_return_if_fail (description != NULL);
  g_return_if_fail (primary_photo_id != NULL);

  FspSessionPrivate *priv = self->priv;
  SoupSession *soup_session = NULL;
  gchar *url = NULL;
  gchar *signed_query = NULL;

  /* Build the signed url */
  signed_query = get_signed_query (priv->secret,
                                   "method", "flickr.photosets.create",
                                   "api_key", priv->api_key,
                                   "auth_token", priv->token,
                                   "title", title,
                                   "description", description,
                                   "primary_photo_id", primary_photo_id,
                                   NULL);

  url = g_strdup_printf ("%s/?%s", FLICKR_API_BASE_URL, signed_query);
  g_free (signed_query);

  /* Perform the async request */
  soup_session = _get_soup_session (self);
  perform_async_request (priv->soup_session, url,
                         _create_photoset_soup_session_cb, G_OBJECT (self),
                         cancellable, callback, fsp_session_create_photoset_async, data);

  g_free (url);
}

gchar *
fsp_session_create_photoset_finish      (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error)
{
  g_return_val_if_fail (FSP_IS_SESSION (self), NULL);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), NULL);

  gchar *photoset_id = NULL;

  photoset_id =
    (gchar*) finish_async_request (G_OBJECT (self), res,
                                   fsp_session_create_photoset_async,
                                   error);
  return photoset_id;
}

void
fsp_session_get_groups_async            (FspSession          *self,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             data)
{
  g_return_if_fail (FSP_IS_SESSION (self));

  FspSessionPrivate *priv = self->priv;
  SoupSession *soup_session = NULL;
  gchar *url = NULL;
  gchar *signed_query = NULL;

  /* Build the signed url */
  signed_query = get_signed_query (priv->secret,
                                   "method", "flickr.groups.pools.getGroups",
                                   "api_key", priv->api_key,
                                   "auth_token", priv->token,
                                   NULL);

  url = g_strdup_printf ("%s/?%s", FLICKR_API_BASE_URL, signed_query);
  g_free (signed_query);

  /* Perform the async request */
  soup_session = _get_soup_session (self);
  perform_async_request (soup_session, url,
                         _get_groups_soup_session_cb, G_OBJECT (self),
                         cancellable, callback, fsp_session_get_groups_async, data);

  g_free (url);
}

GSList *
fsp_session_get_groups_finish           (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error)
{
  g_return_val_if_fail (FSP_IS_SESSION (self), NULL);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), NULL);

  GSList *groups_list = NULL;

  groups_list = (GSList*) finish_async_request (G_OBJECT (self), res,
                                                fsp_session_get_groups_async,
                                                error);
  return groups_list;
}

void
fsp_session_add_to_group_async          (FspSession          *self,
                                         const gchar         *photo_id,
                                         const gchar         *group_id,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             data)
{
  g_return_if_fail (FSP_IS_SESSION (self));
  g_return_if_fail (photo_id != NULL);
  g_return_if_fail (group_id != NULL);

  FspSessionPrivate *priv = self->priv;
  SoupSession *soup_session = NULL;
  gchar *url = NULL;
  gchar *signed_query = NULL;

  /* Build the signed url */
  signed_query = get_signed_query (priv->secret,
                                   "method", "flickr.groups.pools.add",
                                   "api_key", priv->api_key,
                                   "auth_token", priv->token,
                                   "photo_id", photo_id,
                                   "group_id", group_id,
                                   NULL);

  url = g_strdup_printf ("%s/?%s", FLICKR_API_BASE_URL, signed_query);
  g_free (signed_query);

  /* Perform the async request */
  soup_session = _get_soup_session (self);
  perform_async_request (soup_session, url,
                         _add_to_group_soup_session_cb, G_OBJECT (self),
                         cancellable, callback, fsp_session_add_to_group_async, data);

  g_free (url);
}

gboolean
fsp_session_add_to_group_finish         (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error)
{
  g_return_val_if_fail (FSP_IS_SESSION (self), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), FALSE);

  gpointer result = NULL;

  result = finish_async_request (G_OBJECT (self), res,
                                 fsp_session_add_to_group_async, error);

  return result ? TRUE : FALSE;
}

void
fsp_session_get_tags_list_async         (FspSession          *self,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             data)
{
  g_return_if_fail (FSP_IS_SESSION (self));

  FspSessionPrivate *priv = self->priv;
  SoupSession *soup_session = NULL;
  gchar *url = NULL;
  gchar *signed_query = NULL;

  /* Build the signed url */
  signed_query = get_signed_query (priv->secret,
                                   "method", "flickr.tags.getListUser",
                                   "api_key", priv->api_key,
                                   "auth_token", priv->token,
                                   NULL);

  url = g_strdup_printf ("%s/?%s", FLICKR_API_BASE_URL, signed_query);
  g_free (signed_query);

  /* Perform the async request */
  soup_session = _get_soup_session (self);
  perform_async_request (soup_session, url,
                         _get_tags_list_soup_session_cb, G_OBJECT (self),
                         cancellable, callback, fsp_session_get_tags_list_async, data);

  g_free (url);
}

GSList *
fsp_session_get_tags_list_finish        (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error)
{
  g_return_val_if_fail (FSP_IS_SESSION (self), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), FALSE);

  GSList *tags_list = NULL;

  tags_list = (GSList*) finish_async_request (G_OBJECT (self), res,
                                 fsp_session_get_tags_list_async, error);

  return tags_list;
}
