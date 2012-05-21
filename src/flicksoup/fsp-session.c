/*
 * fsp-session.c
 *
 * Copyright (C) 2010-2012 Mario Sanchez Prada
 * Authors: Mario Sanchez Prada <msanchez@igalia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 3 of the GNU Lesser General
 * Public License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public Lesser License
 * along with this program; if not, see <http://www.gnu.org/licenses/>
 *
 */

#include "fsp-session.h"

#include "fsp-data.h"
#include "fsp-error.h"
#include "fsp-parser.h"
#include "fsp-session.h"

#include <config.h>
#include <gcrypt.h>

#ifdef HAVE_LIBSOUP_GNOME
#include <libsoup/soup-gnome.h>
#else
#include <libsoup/soup.h>
#endif

#include <stdarg.h>
#include <string.h>

#define FLICKR_API_BASE_URL   "http://api.flickr.com/services/rest"
#define FLICKR_API_UPLOAD_URL "http://api.flickr.com/services/upload"
#define FLICKR_REQUEST_TOKEN_OAUTH_URL "http://www.flickr.com/services/oauth/request_token"
#define FLICKR_ACCESS_TOKEN_OAUTH_URL "http://www.flickr.com/services/oauth/access_token"

#define OAUTH_CALLBACK_URL "oob"
#define OAUTH_SIGNATURE_METHOD "HMAC-SHA1"
#define OAUTH_VERSION "1.0"

#if DEBUG_ENABLED
#define DEBUG(...) g_debug (__VA_ARGS__)
#else
#define DEBUG(...)
#endif

#define FSP_SESSION_GET_PRIVATE(object)                 \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),               \
                                FSP_TYPE_SESSION,       \
                                FspSessionPrivate))

G_DEFINE_TYPE (FspSession, fsp_session, G_TYPE_OBJECT)


/* Private struct */
struct _FspSessionPrivate
{
  gchar *api_key;
  gchar *secret;
  gchar *token;
  gchar *token_secret;

  gboolean using_gnome_proxy;
  SoupURI *proxy_uri;
  SoupSession *soup_session;
};

/* Signals */
enum {
  DATA_FRACTION_SENT,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };

typedef struct
{
  GObject             *object;
  SoupSession         *soup_session;
  SoupMessage         *soup_message;
  GCancellable        *cancellable;
  gulong               cancellable_id;
  GAsyncReadyCallback  callback;
  gpointer             source_tag;
  gpointer             data;
  gdouble              progress;
} AsyncRequestData;

typedef struct
{
  AsyncRequestData *ar_data;
  GHashTable *extra_params;
} UploadPhotoData;

typedef struct
{
  GCancellable        *cancellable;
  gulong               cancellable_id;
} GCancellableData;

typedef enum {
  AUTHORIZATION_METHOD_ORIGINAL,
  AUTHORIZATION_METHOD_OAUTH_1
} AuthorizationMethod;

/* Properties */

enum  {
  PROP_0,
  PROP_API_KEY,
  PROP_SECRET,
  PROP_TOKEN,
  PROP_TOKEN_SECRET
};


/* Prototypes */

static SoupSession *
_get_soup_session                       (FspSession *self);

static void
_get_request_token_session_cb           (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer     data);
static void
_get_access_token_soup_session_cb       (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer     data);
static void
_check_token_soup_session_cb            (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer data);
static void
_exchange_token_soup_session_cb         (SoupSession *session,
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

static void
_encode_param_from_table_for_signature  (GHashTable *table,
                                         const gchar *key);

static void
_decode_param_from_table_for_signature  (GHashTable *table,
                                         const gchar *key);

static SoupMessage *
_get_soup_message_for_upload            (GFile       *file,
                                         const gchar *contents,
                                         gsize        length,
                                         GHashTable  *table);

static void
_load_file_contents_cb                  (GObject      *object,
                                         GAsyncResult *res,
                                         gpointer      data);

static void
_wrote_body_data_cb                     (SoupMessage *msg,
                                         SoupBuffer  *buffer,
                                         gpointer     data);

static gchar *
_encode_uri                             (const gchar *uri);

static gchar *
_hmac_sha1_signature                    (const gchar *message,
                                         const gchar *signing_key);

static GHashTable *
_get_params_table_from_valist           (const gchar *first_param,
                                         va_list      args);

static gchar *
_get_signed_query_with_params           (const gchar         *api_sig,
                                         GHashTable          *params_table,
                                         AuthorizationMethod  auth_method);

static gboolean
_disconnect_cancellable_on_idle (GCancellableData *clos);

static gboolean
_check_errors_on_soup_response           (SoupMessage  *msg,
                                          GError      **error);

static gboolean
_check_async_errors_on_finish           (GObject       *object,
                                         GAsyncResult  *res,
                                         gpointer       source_tag,
                                         GError       **error);

static gchar *
_get_params_str_for_signature           (GHashTable          *params_table,
                                         const gchar         *signing_key,
                                         AuthorizationMethod  auth_method);

static gchar *
_calculate_api_signature                (const gchar          *url,
                                         const gchar          *params_str,
                                         const gchar          *signing_key,
                                         const gchar          *http_method,
                                         AuthorizationMethod   auth_method);

static gchar *
_get_api_signature_from_hash_table      (const gchar         *url,
                                         GHashTable          *params_table,
                                         const gchar         *signing_key,
                                         const gchar         *http_method,
                                         AuthorizationMethod  auth_method);

static void
_fill_hash_table_with_oauth_params      (GHashTable  *table,
                                         const gchar *api_key,
                                         const gchar *token);

static gchar *
_get_signed_url                       (FspSession          *self,
                                       const gchar         *url,
                                       AuthorizationMethod  auth_method,
                                       const gchar         *first_param,
                                       ... );

static void
_clear_token_information              (FspSession *self);

static void
_perform_async_request                  (SoupSession         *soup_session,
                                         const gchar         *url,
                                         SoupSessionCallback  request_cb,
                                         GObject             *source_object,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             source_tag,
                                         gpointer             data);

static void
_soup_session_cancelled_cb              (GCancellable *cancellable,
                                         gpointer      data);

static void
_handle_soup_response                   (SoupMessage   *msg,
                                         FspParserFunc  parserFunc,
                                         gpointer       data);

static void
_build_async_result_and_complete        (AsyncRequestData *clos,
                                         gpointer    result,
                                         GError     *error);

static gpointer
_finish_async_request                   (GObject       *object,
                                         GAsyncResult  *res,
                                         gpointer       source_tag,
                                         GError       **error);

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

static void
_set_license_soup_session_cb            (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer     data);

static void
_set_location_soup_session_cb            (SoupSession *session,
                                          SoupMessage *msg,
                                          gpointer     data);

static void
_get_location_soup_session_cb            (SoupSession *session,
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
    case PROP_TOKEN_SECRET:
      fsp_session_set_token_secret (self, g_value_get_string (value));
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
    case PROP_TOKEN_SECRET:
      g_value_set_string (value, self->priv->token_secret);
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

  if (self->priv->proxy_uri)
    {
      soup_uri_free (self->priv->proxy_uri);
      self->priv->proxy_uri = NULL;
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
  g_free (self->priv->token_secret);

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

  pspec = g_param_spec_string ("token-secret", "token-secret", "Authorized token secret", NULL,
                               G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_TOKEN_SECRET, pspec);

  /* Signals */
  signals[DATA_FRACTION_SENT] =
    g_signal_new ("data-fraction-sent",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__DOUBLE,
                  G_TYPE_NONE, 1, G_TYPE_DOUBLE);

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
  self->priv->token_secret = NULL;

  self->priv->using_gnome_proxy = FALSE;
  self->priv->proxy_uri = NULL;

  self->priv->soup_session = soup_session_async_new ();
}

static SoupSession *
_get_soup_session                       (FspSession *self)
{
  g_return_val_if_fail (FSP_IS_SESSION (self), NULL);
  return self->priv->soup_session;
}

static void
_get_request_token_session_cb           (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer data)
{
  g_assert (SOUP_IS_MESSAGE (msg));
  g_assert (data != NULL);

  /* Handle message with the right parser */
  _handle_soup_response (msg,
                         (FspParserFunc) fsp_parser_get_request_token,
                         data);
}

static void
_get_access_token_soup_session_cb       (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer data)
{
  g_assert (SOUP_IS_MESSAGE (msg));
  g_assert (data != NULL);

  /* Handle message with the right parser */
  _handle_soup_response (msg,
                         (FspParserFunc) fsp_parser_get_access_token,
                         data);
}

static void
_check_token_soup_session_cb            (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer data)
{
  g_assert (SOUP_IS_MESSAGE (msg));
  g_assert (data != NULL);

  /* Handle message with the right parser */
  _handle_soup_response (msg,
                         (FspParserFunc) fsp_parser_check_token,
                         data);
}

static void
_exchange_token_soup_session_cb         (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer     data)
{
  g_assert (SOUP_IS_MESSAGE (msg));
  g_assert (data != NULL);

  /* Handle message with the right parser */
  _handle_soup_response (msg,
                         (FspParserFunc) fsp_parser_exchange_token,
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
  _handle_soup_response (msg,
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

static void
_encode_param_from_table_for_signature  (GHashTable *table,
                                         const gchar *key)
{
  gchar *current = NULL;
  gchar *new = NULL;

  current = g_hash_table_lookup (table, key);
  if (current != NULL)
    {
      new = _encode_uri (current);
      g_hash_table_replace (table, g_strdup (key), new);
    }
}

static void
_decode_param_from_table_for_signature  (GHashTable *table,
                                         const gchar *key)
{
  gchar *current = NULL;
  gchar *new = NULL;

  current = g_hash_table_lookup (table, key);
  if (current != NULL)
    {
      new = soup_uri_decode (current);
      g_hash_table_replace (table, g_strdup (key), new);
    }
}

static SoupMessage *
_get_soup_message_for_upload            (GFile       *file,
                                         const gchar *contents,
                                         gsize        length,
                                         GHashTable  *table)
{
  GFileInfo *file_info = NULL;
  SoupMessage *msg = NULL;
  SoupMultipart *mpart = NULL;
  SoupBuffer *buffer = NULL;
  GHashTableIter iter;
  const gchar *key, *value;
  gchar *mime_type = NULL;
  gchar *fileuri = NULL;
  gchar *auth_header = NULL;

  /* Gather needed information */
  fileuri = g_file_get_uri (file);
  file_info = g_file_query_info (file,
                                 G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                                 G_FILE_QUERY_INFO_NONE,
                                 NULL,
                                 NULL);
  /* Check mimetype */
  mime_type = g_strdup (g_file_info_get_content_type (file_info));
  g_object_unref (G_OBJECT (file_info));

  /* Init multipart container */
  mpart = soup_multipart_new (SOUP_FORM_MIME_TYPE_MULTIPART);

  /* Traverse table to append them to the message */
  g_hash_table_iter_init (&iter, table);

  while (g_hash_table_iter_next (&iter, (void **) &key, (void **) &value))
    {
      /* OAuth info goes to the Authorization header, the rest
         of the info goes to the body of the HTTP message */
      if (g_str_has_prefix (key, "oauth"))
        {
          gchar *current = NULL;

          if (!auth_header)
            current = g_strdup_printf ("OAuth %s=\"%s\"", key, value);
          else
            current = g_strdup_printf ("%s,%s=\"%s\"", auth_header, key, value);

          g_free (auth_header);
          auth_header = current;
        }
      else
        soup_multipart_append_form_string (mpart, key, value);
    }

  /* Append the content of the file */
  buffer = soup_buffer_new (SOUP_MEMORY_TAKE, contents, length);
  soup_multipart_append_form_file (mpart, "photo", fileuri,
                                   mime_type, buffer);

  /* Get the associated message */
  msg = soup_form_request_new_from_multipart (FLICKR_API_UPLOAD_URL, mpart);

  /* Append the Authorization header */
  soup_message_headers_append (msg->request_headers, "Authorization", auth_header);
  g_free (auth_header);

  /* Free */
  soup_multipart_free (mpart);
  g_free (fileuri);
  g_free (mime_type);

  /* Return message */
  return msg;
}

static void
_load_file_contents_cb                  (GObject      *object,
                                         GAsyncResult *res,
                                         gpointer      data)
{
  UploadPhotoData *up_clos = NULL;
  AsyncRequestData *ard_clos = NULL;
  GHashTable *extra_params = NULL;
  GFile *file = NULL;
  GError *error = NULL;
  gchar *contents;
  gsize length;

  g_return_if_fail (G_IS_FILE (object));
  g_return_if_fail (G_IS_ASYNC_RESULT (res));
  g_return_if_fail (data != NULL);

  /* Get data from UploadPhotoData closure, and free it */
  up_clos = (UploadPhotoData *) data;
  ard_clos = up_clos->ar_data;
  extra_params = up_clos->extra_params;
  g_slice_free (UploadPhotoData, up_clos);

  file = G_FILE (object);
  if (g_file_load_contents_finish (file, res, &contents, &length, NULL, &error))
    {
      FspSession *self = NULL;
      SoupSession *soup_session = NULL;
      SoupMessage *msg = NULL;

      self = FSP_SESSION (ard_clos->object);
      msg = _get_soup_message_for_upload (file, contents, length, extra_params);
      ard_clos->soup_message = msg;

      soup_session = _get_soup_session (self);

      /* Connect to the "cancelled" signal thread safely */
      if (ard_clos->cancellable)
        {
          ard_clos->cancellable_id =
            g_cancellable_connect (ard_clos->cancellable,
                                   G_CALLBACK (_soup_session_cancelled_cb),
                                   ard_clos,
                                   NULL);
        }

      /* Connect to this to report progress */
      g_signal_connect (G_OBJECT (msg), "wrote-body-data",
                        G_CALLBACK (_wrote_body_data_cb),
                        ard_clos);

      /* Perform the async request */
      soup_session_queue_message (soup_session, msg,
                                  _photo_upload_soup_session_cb, ard_clos);

      DEBUG ("\nRequested URL:\n%s\n", FLICKR_API_UPLOAD_URL);

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
      _build_async_result_and_complete (ard_clos, NULL, error);
    }
}

static void
_wrote_body_data_cb                     (SoupMessage *msg,
                                         SoupBuffer  *buffer,
                                         gpointer     data)
{
  FspSession *self = NULL;
  AsyncRequestData *clos = NULL;

  goffset msg_len = 0;
  gsize buffer_len = 0;
  gdouble fraction = 0.0;

  /* Sanity check */
  if (!buffer || !msg || !msg->request_body)
    return;

  msg_len = msg->request_body->length;
  buffer_len = buffer->length;

  clos = (AsyncRequestData *) data;
  self = FSP_SESSION (clos->object);

  fraction = (msg_len > 0) ? (double)buffer_len / msg_len : 0.0;
  clos->progress += fraction;

  g_signal_emit (self, signals[DATA_FRACTION_SENT], 0, clos->progress);
}

static gchar *
_encode_uri                             (const gchar *uri)
{
  return soup_uri_encode (uri, "%!*'();:@&=+$,/?#[] ");
}

static gchar *
_hmac_sha1_signature                    (const gchar *message,
                                         const gchar *signing_key)
{
  gchar *signature = NULL;
  gchar *encoded_signature = NULL;
  gcry_md_hd_t digest_obj;
  unsigned char *hmac_digest;
  guint digest_len;

  gcry_md_open(&digest_obj,
	       GCRY_MD_SHA1,
	       GCRY_MD_FLAG_SECURE | GCRY_MD_FLAG_HMAC);
  gcry_md_setkey(digest_obj, signing_key, strlen (signing_key));
  gcry_md_write (digest_obj, message, strlen (message));
  gcry_md_final (digest_obj);
  hmac_digest = gcry_md_read (digest_obj, 0);

  digest_len = gcry_md_get_algo_dlen (GCRY_MD_SHA1);
  signature = g_base64_encode (hmac_digest, digest_len);

  gcry_md_close (digest_obj);

  encoded_signature = _encode_uri (signature);
  g_free(signature);

  return encoded_signature;
}

static GHashTable *
_get_params_table_from_valist           (const gchar *first_param,
                                         va_list      args)
{
  GHashTable *table = NULL;
  gchar *p = NULL;
  gchar *v = NULL;

  g_return_val_if_fail (first_param != NULL, NULL);

  table = g_hash_table_new_full (g_str_hash, g_str_equal,
                                 (GDestroyNotify)g_free,
                                 (GDestroyNotify)g_free);

  /* Fill the hash table */
  for (p = (gchar *) first_param; p; p = va_arg (args, gchar*))
    {
      v = va_arg (args, gchar*);

      /* Ignore parameter with no value */
      if (v != NULL)
        /* Values should be always encoded */
        g_hash_table_insert (table, g_strdup (p), _encode_uri (v));
      else
        DEBUG ("Missing value for %s. Ignoring parameter.", p);
    }

  return table;
}

static gchar *
_get_signed_query_with_params           (const gchar         *api_sig,
                                         GHashTable          *params_table,
                                         AuthorizationMethod  auth_method)
{
  GList *keys = NULL;
  gchar *retval = NULL;

  g_return_val_if_fail (params_table != NULL, NULL);
  g_return_val_if_fail (api_sig != NULL, NULL);

  /* Get ownership of the table */
  g_hash_table_ref (params_table);

  /* Get a list of keys */
  keys = g_hash_table_get_keys (params_table);
  if (keys != NULL)
    {
      gchar **url_params_array = NULL;
      GList *k = NULL;
      gint i = 0;

      /* Build gchar** arrays for building the final
         string to be used as the list of GET params */
      url_params_array = g_new0 (gchar*, g_list_length (keys) + 2);

      /* Fill arrays */
      for (k = keys; k; k = g_list_next (k))
        {
          gchar *key = NULL;
          gchar *actual_value = NULL;

          key = (gchar*) k->data;
          actual_value = g_strdup (g_hash_table_lookup (params_table, key));

          url_params_array[i++] = g_strdup_printf ("%s=%s", key, actual_value);
          g_free (actual_value);
        }

      /* Add those to the params array (space previously reserved) */
      if (auth_method == AUTHORIZATION_METHOD_ORIGINAL)
        url_params_array[i] = g_strdup_printf ("api_sig=%s", api_sig);
      else
        url_params_array[i] = g_strdup_printf ("oauth_signature=%s", api_sig);

      /* Build the signed query */
      retval = g_strjoinv ("&", url_params_array);

      /* Free */
      g_strfreev (url_params_array);
    }
  g_list_free (keys);
  g_hash_table_unref (params_table);

  return retval;
}

static gboolean
_disconnect_cancellable_on_idle (GCancellableData *clos)
{
  GCancellable *cancellable = NULL;
  gulong cancellable_id = 0;

  /* Get data from closure, and free it */
  cancellable = clos->cancellable;
  cancellable_id = clos->cancellable_id;
  g_slice_free (GCancellableData, clos);

  /* Disconnect from the "cancelled" signal if needed */
  if (cancellable)
    {
      g_cancellable_disconnect (cancellable, cancellable_id);
      g_object_unref (cancellable);
    }

  return FALSE;
}

static gboolean
_check_errors_on_soup_response           (SoupMessage  *msg,
                                          GError      **error)
{
  GError *err = NULL;
  gchar *response_str = NULL;

  g_assert (SOUP_IS_MESSAGE (msg));

  /* First check it's not an OAuth problem */
  response_str = g_strndup (msg->response_body->data, msg->response_body->length);
  if (response_str && g_str_has_prefix (response_str, "oauth_problem="))
    {
      if (!g_strcmp0 (&response_str[14], "token_rejected"))
        err = g_error_new (FSP_ERROR, FSP_ERROR_OAUTH_NOT_AUTHORIZED_YET,
                           "[OAuth]: The access token has been rejected");
      else if (!g_strcmp0 (&response_str[14], "verifier_invalid"))
        err = g_error_new (FSP_ERROR, FSP_ERROR_OAUTH_VERIFIER_INVALID,
                           "[OAuth] The verification code is invalid");
      else
        err = g_error_new (FSP_ERROR, FSP_ERROR_OAUTH_UNKNOWN_ERROR,
                           "[OAuth] unknown error");
    }
  else if (!SOUP_STATUS_IS_SUCCESSFUL (msg->status_code))
    {
      /* Check non-succesful SoupMessage's only */
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

  g_free (response_str);

  /* Propagate error */
  if (err != NULL)
    g_propagate_error (error, err);

  /* Return result */
  return (err != NULL);
}

static gboolean
_check_async_errors_on_finish           (GObject       *object,
                                         GAsyncResult  *res,
                                         gpointer       source_tag,
                                         GError       **error)
{
  gboolean errors_found = TRUE;

  g_return_val_if_fail (G_IS_OBJECT (object), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), FALSE);

  if (g_simple_async_result_is_valid (res, object, source_tag))
    {
      GSimpleAsyncResult *simple = NULL;

      /* Check error */
      simple = G_SIMPLE_ASYNC_RESULT (res);
      if (!g_simple_async_result_propagate_error (simple, error))
	errors_found = FALSE;
    }
  else
    g_set_error_literal (error, FSP_ERROR, FSP_ERROR_OTHER, "Internal error");

  return errors_found;
}


static gchar *
_get_params_str_for_signature           (GHashTable          *params_table,
                                         const gchar         *signing_key,
                                         AuthorizationMethod  auth_method)
{
  GList *keys = NULL;
  gchar **params_str_array = NULL;
  gchar *params_str = NULL;
  GList *k = NULL;
  gboolean old_auth_method = FALSE;
  gint i = 0;

  /* Get a list of keys */
  keys = g_hash_table_get_keys (params_table);
  if (!keys)
    return NULL;

  /* Sort the list */
  keys = g_list_sort (keys, (GCompareFunc) g_strcmp0);

  /* Build gchar** arrays for building the signature string */
  old_auth_method = (auth_method == AUTHORIZATION_METHOD_ORIGINAL);
  if (old_auth_method)
    {
      params_str_array = g_new0 (gchar*, (2 * g_list_length (keys)) + 2);
      params_str_array[i++] = g_strdup (signing_key);
    }
  else
    params_str_array = g_new0 (gchar*, g_list_length (keys) + 1);

  /* Fill arrays */
  for (k = keys; k; k = g_list_next (k))
    {
      const gchar *key = (gchar*) k->data;
      const gchar *value = g_hash_table_lookup (params_table, key);

      if (old_auth_method)
        {
          params_str_array[i++] = g_strdup (key);
          params_str_array[i++] = g_strdup (value);
        }
      else
        params_str_array[i++] = g_strdup_printf ("%s=%s", key, value);
    }
  params_str_array[i] = NULL;

  params_str = g_strjoinv (old_auth_method ? NULL : "&", params_str_array);
  g_strfreev (params_str_array);

  g_list_free (keys);

  return params_str;
}

static gchar *
_calculate_api_signature                (const gchar          *url,
                                         const gchar          *params_str,
                                         const gchar          *signing_key,
                                         const gchar          *http_method,
                                         AuthorizationMethod   auth_method)
{
  gchar *base_string = NULL;
  gchar *encoded_params = NULL;
  gchar *encoded_url = NULL;
  gchar *api_sig = NULL;

  if (!params_str)
    return NULL;

  if (auth_method == AUTHORIZATION_METHOD_ORIGINAL)
    return g_compute_checksum_for_string (G_CHECKSUM_MD5, params_str, -1);

  /* Using the new OAuth-based authorization API */
  encoded_url = _encode_uri (url);
  encoded_params = _encode_uri (params_str);
  base_string = g_strdup_printf ("%s&%s&%s", http_method, encoded_url, encoded_params);
  DEBUG ("Base string for signing: %s", base_string);
  g_free (encoded_url);
  g_free (encoded_params);

  api_sig = _hmac_sha1_signature (base_string, signing_key);
  g_free (base_string);

  return api_sig;
}

static gchar *
_get_api_signature_from_hash_table      (const gchar         *url,
                                         GHashTable          *params_table,
                                         const gchar         *signing_key,
                                         const gchar         *http_method,
                                         AuthorizationMethod  auth_method)
{
  gchar *api_sig = NULL;
  gchar *params_str = NULL;

  g_return_val_if_fail (params_table != NULL, NULL);

  /* Get the signature string and calculate the api_sig value */
  params_str = _get_params_str_for_signature (params_table, signing_key, auth_method);
  api_sig = _calculate_api_signature (url, params_str, signing_key, http_method, auth_method);
  g_free (params_str);

  return api_sig;
}

static void
_fill_hash_table_with_oauth_params      (GHashTable  *table,
                                         const gchar *api_key,
                                         const gchar *token)
{
  gchar *timestamp = NULL;
  gchar *random_str = NULL;
  gchar *nonce = NULL;

  /* Add mandatory parameters to the hash table */
  timestamp = g_strdup_printf ("%d", (gint) time(NULL));
  random_str = g_strdup_printf ("%d_%s", g_random_int (), timestamp);
  nonce = g_compute_checksum_for_string (G_CHECKSUM_MD5, random_str, -1);
  g_free (random_str);

  g_hash_table_insert (table, g_strdup ("oauth_timestamp"), timestamp);
  g_hash_table_insert (table, g_strdup ("oauth_nonce"), nonce);
  g_hash_table_insert (table, g_strdup ("oauth_consumer_key"), g_strdup (api_key));
  g_hash_table_insert (table, g_strdup ("oauth_signature_method"), g_strdup (OAUTH_SIGNATURE_METHOD));
  g_hash_table_insert (table, g_strdup ("oauth_version"), g_strdup (OAUTH_VERSION));

  /* The token is option (would not be available when authorizing) */
  if (token)
    g_hash_table_insert (table, g_strdup ("oauth_token"), g_strdup (token));
}

static gchar *
_get_signed_url                       (FspSession          *self,
                                       const gchar         *url,
                                       AuthorizationMethod  auth_method,
                                       const gchar         *first_param,
                                       ... )
{
  FspSessionPrivate *priv = NULL;
  va_list args;
  GHashTable *table = NULL;
  gchar *signing_key = NULL;
  gchar *signed_query = NULL;
  gchar *api_sig = NULL;
  gchar *retval = NULL;

  g_return_val_if_fail (FSP_IS_SESSION (self), NULL);
  g_return_val_if_fail (url != NULL, NULL);
  g_return_val_if_fail (first_param != NULL, NULL);

  priv = self->priv;
  va_start (args, first_param);

  /* Get the hash table for the params */
  table = _get_params_table_from_valist (first_param, args);

  /* Fill the table with mandatory parameters */
  if (auth_method == AUTHORIZATION_METHOD_OAUTH_1)
    {
      _fill_hash_table_with_oauth_params (table, priv->api_key, priv->token);
      signing_key = g_strdup_printf ("%s&%s", priv->secret, priv->token_secret ? priv->token_secret : "");
    }
  else
    {
      g_hash_table_insert (table, g_strdup ("api_key"), g_strdup (priv->api_key));
      if (priv->token)
        g_hash_table_insert (table, g_strdup ("auth_token"), g_strdup (priv->token));
      signing_key = g_strdup (priv->secret);
    }

  api_sig = _get_api_signature_from_hash_table (url, table, signing_key, "GET", auth_method);
  g_free (signing_key);

  /* Get the signed URL with the needed params */
  if ((table != NULL) && (api_sig != NULL))
    signed_query = _get_signed_query_with_params (api_sig, table, auth_method);

  g_hash_table_unref (table);
  g_free (api_sig);

  va_end (args);

  retval = g_strdup_printf ("%s?%s", url, signed_query);
  g_free (signed_query);

  return retval;
}

static void
_clear_token_information              (FspSession *self)
{
  FspSessionPrivate *priv = NULL;

  g_return_if_fail (FSP_IS_SESSION (self));

  priv = self->priv;
  if (priv->token)
    {
      g_free (priv->token);
      priv->token = NULL;
    }

  if (priv->token_secret)
    {
      g_free (priv->token_secret);
      priv->token_secret = NULL; 
    }
}

static void
_perform_async_request                  (SoupSession         *soup_session,
                                         const gchar         *url,
                                         SoupSessionCallback  request_cb,
                                         GObject             *source_object,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             source_tag,
                                         gpointer             data)
{
  AsyncRequestData *clos = NULL;
  SoupMessage *msg = NULL;

  g_return_if_fail (SOUP_IS_SESSION (soup_session));
  g_return_if_fail (url != NULL);
  g_return_if_fail (request_cb != NULL);
  g_return_if_fail (callback != NULL);

  /* Build and queue the message */
  msg = soup_message_new (SOUP_METHOD_GET, url);

  /* Save important data for the callback */
  clos = g_slice_new0 (AsyncRequestData);
  clos->object = source_object;
  clos->soup_session = soup_session;
  clos->soup_message = msg;
  clos->cancellable = cancellable;
  clos->callback = callback;
  clos->source_tag = source_tag;
  clos->data = data;
  clos->progress = 0.0;

  /* Connect to the "cancelled" signal thread safely */
  if (clos->cancellable)
    {
      clos->cancellable_id =
        g_cancellable_connect (clos->cancellable,
                               G_CALLBACK (_soup_session_cancelled_cb),
                               clos,
                               NULL);
    }

  /* Connect to this to report progress */
  g_signal_connect (G_OBJECT (msg), "wrote-body-data",
                    G_CALLBACK (_wrote_body_data_cb),
                    clos);

  /* Queue the message */
  soup_session_queue_message (soup_session, msg, request_cb, clos);

  DEBUG ("\nRequested URL:\n%s\n", url);
}

static void
_soup_session_cancelled_cb              (GCancellable *cancellable,
                                         gpointer      data)
{
  AsyncRequestData *clos = NULL;
  SoupSession *session = NULL;
  SoupMessage *message = NULL;

  clos = (AsyncRequestData *) data;

  /* Get data from closure, and free it */
  session = clos->soup_session;
  message = clos->soup_message;

  soup_session_cancel_message (session, message, SOUP_STATUS_CANCELLED);

  DEBUG ("%s", "Remote request cancelled!");
}

static void
_handle_soup_response                   (SoupMessage   *msg,
                                         FspParserFunc  parserFunc,
                                         gpointer       data)
{
  FspParser *parser = NULL;
  AsyncRequestData *clos = NULL;
  gpointer result = NULL;
  GError *err = NULL;
  gchar *response_str = NULL;
  gulong response_len = 0;

  g_assert (SOUP_IS_MESSAGE (msg));
  g_assert (parserFunc != NULL);
  g_assert (data != NULL);

  parser = fsp_parser_get_instance ();
  clos = (AsyncRequestData *) data;

  /* Stop reporting progress */
  g_signal_handlers_disconnect_by_func (msg, _wrote_body_data_cb, clos);

  response_str = g_strndup (msg->response_body->data, msg->response_body->length);
  response_len = (gulong) msg->response_body->length;
  if (response_str)
    DEBUG ("\nResponse got:\n%s\n", response_str);

  /* Get value from response */
  if (!_check_errors_on_soup_response (msg, &err))
    result = parserFunc (parser, response_str, response_len, &err);

  /* Build response and call async callback */
  _build_async_result_and_complete (clos, result, err);

  g_free (response_str);
}

static void
_build_async_result_and_complete        (AsyncRequestData *clos,
                                         gpointer    result,
                                         GError     *error)
{
  GSimpleAsyncResult *res = NULL;
  GObject *object = NULL;
  GCancellableData *cancellable_data = NULL;
  GCancellable *cancellable = NULL;
  gulong cancellable_id = 0;
  GAsyncReadyCallback  callback = NULL;
  gpointer source_tag;
  gpointer data;

  g_assert (clos != NULL);

  /* Get data from closure, and free it */
  object = clos->object;
  cancellable = clos->cancellable;
  cancellable_id = clos->cancellable_id;
  callback = clos->callback;
  source_tag = clos->source_tag;
  data = clos->data;
  g_slice_free (AsyncRequestData, clos);

  /* Make sure the "cancelled" signal gets disconnected in another
     iteration of the main loop to avoid a dead-lock with itself */
  cancellable_data = g_slice_new0 (GCancellableData);
  cancellable_data->cancellable = cancellable ? g_object_ref (cancellable) : NULL;
  cancellable_data->cancellable_id = cancellable_id;

  g_idle_add ((GSourceFunc) _disconnect_cancellable_on_idle, cancellable_data);

  /* Build response and call async callback */
  res = g_simple_async_result_new (object, callback,
                                   data, source_tag);

  /* Return the given value or an error otherwise */
  if (error != NULL)
    g_simple_async_result_set_from_error (res, error);
  else
    g_simple_async_result_set_op_res_gpointer (res, result, NULL);

  /* Execute the callback */
  g_simple_async_result_complete_in_idle (res);

  g_object_unref (G_OBJECT (res));
}

static gpointer
_finish_async_request                   (GObject       *object,
                                         GAsyncResult  *res,
                                         gpointer       source_tag,
                                         GError       **error)
{
  gpointer retval = NULL;

  g_return_val_if_fail (G_IS_OBJECT (object), NULL);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), NULL);

  /* Check for errors */
  if (!_check_async_errors_on_finish (object, res, source_tag, error))
    {
      GSimpleAsyncResult *simple = NULL;
      gpointer result = NULL;

      /* Get result */
      simple = G_SIMPLE_ASYNC_RESULT (res);
      result = g_simple_async_result_get_op_res_gpointer (simple);
      if (result != NULL)
        retval = result;
      else
        g_set_error_literal (error, FSP_ERROR, FSP_ERROR_OTHER,
                             "Internal error");
    }

  return retval;
}

static void
_photo_upload_soup_session_cb           (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer     data)
{
  AsyncRequestData *ard_clos = NULL;

  g_assert (SOUP_IS_MESSAGE (msg));
  g_assert (data != NULL);

  ard_clos = (AsyncRequestData *) data;
  g_signal_handlers_disconnect_by_func (msg, _wrote_body_data_cb, ard_clos->object);

  /* Handle message with the right parser */
  _handle_soup_response (msg,
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
  _handle_soup_response (msg,
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
  _handle_soup_response (msg,
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
  _handle_soup_response (msg,
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
  _handle_soup_response (msg,
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
  _handle_soup_response (msg,
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
  _handle_soup_response (msg,
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
  _handle_soup_response (msg,
                         (FspParserFunc) fsp_parser_get_tags_list,
                         data);
}

static void
_set_license_soup_session_cb            (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer     data)
{
  g_assert (SOUP_IS_MESSAGE (msg));
  g_assert (data != NULL);

  /* Handle message with the right parser */
  _handle_soup_response (msg,
                         (FspParserFunc) fsp_parser_set_license,
                         data);
}

static void
_set_location_soup_session_cb            (SoupSession *session,
                                          SoupMessage *msg,
                                          gpointer     data)
{
  g_assert (SOUP_IS_MESSAGE (msg));
  g_assert (data != NULL);

  /* Handle message with the right parser */
  _handle_soup_response (msg,
                         (FspParserFunc) fsp_parser_set_location,
                         data);
}

static void
_get_location_soup_session_cb            (SoupSession *session,
                                          SoupMessage *msg,
                                          gpointer     data)
{
  g_assert (SOUP_IS_MESSAGE (msg));
  g_assert (data != NULL);

  /* Handle message with the right parser */
  _handle_soup_response (msg,
                         (FspParserFunc) fsp_parser_get_location,
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

  return FSP_SESSION (g_object_new(FSP_TYPE_SESSION,
                                   "api-key", api_key,
                                   "secret", secret,
                                   "token", token,
                                   NULL));
}

gboolean
fsp_session_set_http_proxy              (FspSession *self,
                                         gboolean use_gnome_proxy,
                                         const char *host, const char *port,
                                         const char *username, const char *password)
{
  SoupURI *proxy_uri = NULL;

#ifdef HAVE_LIBSOUP_GNOME
  gboolean using_gnome_proxy_before = FALSE;
#endif

  g_return_val_if_fail (FSP_IS_SESSION (self), FALSE);

#ifdef HAVE_LIBSOUP_GNOME
  /* We're gonna need this to make a good decision later */
  using_gnome_proxy_before = self->priv->using_gnome_proxy;

  if (using_gnome_proxy_before != use_gnome_proxy)
    {
      void (*soup_feature_func) (SoupSession *, GType) = NULL;
      if (use_gnome_proxy)
        soup_feature_func = soup_session_add_feature_by_type;
      else
        soup_feature_func = soup_session_remove_feature_by_type;

      /* Add or remove the feature */
      soup_feature_func (self->priv->soup_session,
                         SOUP_TYPE_PROXY_RESOLVER_GNOME);
    }
  self->priv->using_gnome_proxy = use_gnome_proxy;
#else
  /* Just ignore the using_gnome_proxy parameter in this case */
  self->priv->using_gnome_proxy = FALSE;
#endif

  if (!self->priv->using_gnome_proxy)
    {
      /* If using the GNOME proxy we just forget about the rest. */
      if (host != NULL)
        {
          const gchar *actual_user = NULL;
          const gchar *actual_password = NULL;
          guint actual_port = 0;

          /* Ensure no garbage is used for username */
          if (username == NULL || *username == '\0')
            actual_user = NULL;
          else
            actual_user = username;

          /* Ensure no garbage is used for password */
          if (password == NULL || *password == '\0')
            actual_password = NULL;
          else
            actual_password = password;

          /* We need a numeric port */
          actual_port = (guint) g_ascii_strtoll ((gchar *) port, NULL, 10);

          /* Build the actual SoupURI object */
          proxy_uri = soup_uri_new (NULL);
          soup_uri_set_scheme (proxy_uri, SOUP_URI_SCHEME_HTTP);
          soup_uri_set_host (proxy_uri, host);
          soup_uri_set_port (proxy_uri, actual_port);
          soup_uri_set_user (proxy_uri, actual_user);
          soup_uri_set_password (proxy_uri, actual_password);
        }
    }

  /* Set/unset the proxy */
  g_object_set (G_OBJECT (self->priv->soup_session),
                SOUP_SESSION_PROXY_URI,
                proxy_uri,
                NULL);

  /* Update internal values if needed */
  if ((!self->priv->proxy_uri && proxy_uri)
      || (self->priv->proxy_uri && !proxy_uri)
      || (self->priv->proxy_uri && proxy_uri && !soup_uri_equal (self->priv->proxy_uri, proxy_uri)))
    {
      /* Save internal reference to the proxy */
      if (self->priv->proxy_uri)
        soup_uri_free (self->priv->proxy_uri);

      self->priv->proxy_uri = proxy_uri;

      /* Proxy configuration actually changed */
      return TRUE;
    }

#ifdef HAVE_LIBSOUP_GNOME
  return using_gnome_proxy_before != use_gnome_proxy;
#else
  /* Proxy configuration has not changed */
  return FALSE;
#endif
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

const gchar *
fsp_session_get_token_secret            (FspSession *self)
{
  g_return_val_if_fail (FSP_IS_SESSION (self), NULL);

  return self->priv->token_secret;
}

void
fsp_session_set_token_secret            (FspSession  *self,
                                         const gchar *token_secret)
{
  g_return_if_fail (FSP_IS_SESSION (self));

  g_free (self->priv->token_secret);
  self->priv->token_secret = g_strdup (token_secret);
}

/* Get authorization URL */

void
fsp_session_get_auth_url_async          (FspSession          *self,
                                         GCancellable        *c,
                                         GAsyncReadyCallback  cb,
                                         gpointer             data)
{
  gchar *url = NULL;

  g_return_if_fail (FSP_IS_SESSION (self));

  /* If we're here, we're no longer interested in former tokens */
  _clear_token_information (self);

  /* Build the signed url */
  url = _get_signed_url (self,
                         FLICKR_REQUEST_TOKEN_OAUTH_URL,
                         AUTHORIZATION_METHOD_OAUTH_1,
                         "oauth_callback", OAUTH_CALLBACK_URL,
                         NULL);

  /* Perform the async request */
  _perform_async_request (self->priv->soup_session, url,
                          _get_request_token_session_cb, G_OBJECT (self),
                          c, cb, fsp_session_get_auth_url_async, data);

  g_free (url);
}

gchar *
fsp_session_get_auth_url_finish         (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error)
{
  FspDataAuthToken *auth_token = NULL;
  gchar *auth_url = NULL;

  g_return_val_if_fail (FSP_IS_SESSION (self), NULL);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), NULL);

  auth_token =
    FSP_DATA_AUTH_TOKEN (_finish_async_request (G_OBJECT (self), res,
                                                fsp_session_get_auth_url_async,
                                                error));
  /* Build the auth URL from the request token */
  if (auth_token != NULL)
    {
      /* Save the data */
      fsp_session_set_token (self, auth_token->token);
      fsp_session_set_token_secret (self, auth_token->token_secret);

      /* Build the authorization url */
      auth_url = g_strdup_printf ("http://www.flickr.com/services/oauth/authorize"
                                  "?oauth_token=%s", auth_token->token);

      fsp_data_free (FSP_DATA (auth_token));
    }

  return auth_url;
}

/* Complete authorization */

void
fsp_session_complete_auth_async         (FspSession          *self,
                                         const gchar         *code,
                                         GCancellable        *c,
                                         GAsyncReadyCallback  cb,
                                         gpointer             data)
{
  FspSessionPrivate *priv = NULL;

  g_return_if_fail (FSP_IS_SESSION (self));
  g_return_if_fail (cb != NULL);

  priv = self->priv;
  if (priv->token != NULL && priv->token_secret != NULL)
    {
      gchar *url = NULL;

      /* Build the signed url */
      url = _get_signed_url (self,
                             FLICKR_ACCESS_TOKEN_OAUTH_URL,
                             AUTHORIZATION_METHOD_OAUTH_1,
                             "oauth_verifier", code,
                             NULL);

      /* Perform the async request */
      _perform_async_request (priv->soup_session, url,
                              _get_access_token_soup_session_cb, G_OBJECT (self),
                              c, cb, fsp_session_complete_auth_async, data);

      g_free (url);
    }
  else
    {
      GError *err = NULL;

      /* Build and report error */
      err = g_error_new (FSP_ERROR, FSP_ERROR_OAUTH_NOT_AUTHORIZED_YET, "Not authorized yet");
      g_simple_async_report_gerror_in_idle (G_OBJECT (self),
                                            cb, data, err);
      /* Ensure we clean things up */
      _clear_token_information (self);
    }
}

FspDataAuthToken *
fsp_session_complete_auth_finish        (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error)
{
  FspDataAuthToken *auth_token = NULL;

  g_return_val_if_fail (FSP_IS_SESSION (self), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), FALSE);

  auth_token =
    FSP_DATA_AUTH_TOKEN (_finish_async_request (G_OBJECT (self), res,
                                                fsp_session_complete_auth_async,
                                                error));

  /* Complete the authorization saving the token if present */
  if (auth_token != NULL)
    {
      fsp_session_set_token (self, auth_token->token);
      fsp_session_set_token_secret (self, auth_token->token_secret);
    }

  return auth_token;
}

void
fsp_session_exchange_token_async        (FspSession          *self,
                                         GCancellable        *c,
                                         GAsyncReadyCallback cb,
                                         gpointer             data)
{
  FspSessionPrivate *priv = NULL;

  g_return_if_fail (FSP_IS_SESSION (self));
  g_return_if_fail (cb != NULL);

  priv = self->priv;
  if (priv->token != NULL)
    {
      gchar *url = NULL;

      /* Build the signed url */
      url = _get_signed_url (self,
                             FLICKR_API_BASE_URL,
                             AUTHORIZATION_METHOD_ORIGINAL,
                             "method", "flickr.auth.oauth.getAccessToken",
                             NULL);

      /* Perform the async request */
      _perform_async_request (priv->soup_session, url,
                              _exchange_token_soup_session_cb, G_OBJECT (self),
                              c, cb, fsp_session_exchange_token_async, data);

      g_free (url);
    }
  else
    {
      GError *err = NULL;

      /* Build and report error */
      err = g_error_new (FSP_ERROR, FSP_ERROR_OAUTH_NOT_AUTHORIZED_YET, "Not authorized yet");
      g_simple_async_report_gerror_in_idle (G_OBJECT (self),
                                            cb, data, err);
      /* Ensure we clean things up */
      _clear_token_information (self);
    }
}

void
fsp_session_exchange_token_finish       (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error)
{
  FspDataAuthToken *auth_token = NULL;

  g_return_if_fail (FSP_IS_SESSION (self));
  g_return_if_fail (G_IS_ASYNC_RESULT (res));

  auth_token =
    FSP_DATA_AUTH_TOKEN (_finish_async_request (G_OBJECT (self), res,
                                                fsp_session_exchange_token_async,
                                                error));
  if (auth_token != NULL)
    {
      fsp_session_set_token (self, auth_token->token);
      fsp_session_set_token_secret (self, auth_token->token_secret);
      fsp_data_free (FSP_DATA (auth_token));
    }
}

void
fsp_session_check_auth_info_async       (FspSession          *self,
                                         GCancellable        *c,
                                         GAsyncReadyCallback  cb,
                                         gpointer             data)
{
  FspSessionPrivate *priv = NULL;

  g_return_if_fail (FSP_IS_SESSION (self));
  g_return_if_fail (cb != NULL);

  priv = self->priv;
  if (priv->token != NULL)
    {
      gchar *url = NULL;

      /* Build the signed url */
      url = _get_signed_url (self,
                             FLICKR_API_BASE_URL,
                             AUTHORIZATION_METHOD_OAUTH_1,
                             "method", "flickr.auth.oauth.checkToken",
                             NULL);

      /* Perform the async request */
      _perform_async_request (priv->soup_session, url,
                              _check_token_soup_session_cb, G_OBJECT (self),
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
  FspDataAuthToken *auth_token = NULL;

  g_return_val_if_fail (FSP_IS_SESSION (self), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), FALSE);

  auth_token =
    FSP_DATA_AUTH_TOKEN (_finish_async_request (G_OBJECT (self), res,
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
  FspSessionPrivate *priv = NULL;

  g_return_if_fail (FSP_IS_SESSION (self));
  g_return_if_fail (cb != NULL);

  priv = self->priv;
  if (priv->token != NULL)
    {
      gchar *url = NULL;

      /* Build the signed url */
      url = _get_signed_url (self,
                             FLICKR_API_BASE_URL,
                             AUTHORIZATION_METHOD_OAUTH_1,
                             "method", "flickr.people.getUploadStatus",
                             NULL);

      /* Perform the async request */
      _perform_async_request (priv->soup_session, url,
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
  FspDataUploadStatus *upload_status = NULL;

  g_return_val_if_fail (FSP_IS_SESSION (self), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), FALSE);

  upload_status =
    FSP_DATA_UPLOAD_STATUS (_finish_async_request (G_OBJECT (self), res,
                                                   fsp_session_get_upload_status_async,
                                                   error));
  return upload_status;
}

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
                                         gpointer             data)
{
  FspSessionPrivate *priv = NULL;
  SoupSession *soup_session = NULL;
  GHashTable *extra_params = NULL;
  AsyncRequestData *ard_clos = NULL;
  UploadPhotoData *up_clos = NULL;
  GFile *file = NULL;
  gchar *signing_key = NULL;
  gchar *api_sig = NULL;

  g_return_if_fail (FSP_IS_SESSION (self));
  g_return_if_fail (fileuri != NULL);

  /* Get flickr proxy */
  priv = self->priv;
  soup_session = _get_soup_session (self);

  /* Get extra params (those actually used for the picture) */
  extra_params = _get_upload_extra_params (title, description, tags,
                                           is_public, is_family, is_friend,
                                           safety_level, content_type, hidden);

  /* Add mandatory parameters according to OAuth specification */
  _fill_hash_table_with_oauth_params (extra_params, priv->api_key, priv->token);

  /* OAuth requires to encode some fields just to calculate the
     signature, so do some extra encoding now and undo it later */
  _encode_param_from_table_for_signature (extra_params, "title");
  _encode_param_from_table_for_signature (extra_params, "description");
  _encode_param_from_table_for_signature (extra_params, "tags");

  /* Build the api signature and add it to the hash table */
  signing_key = g_strdup_printf ("%s&%s", priv->secret, priv->token_secret);
  api_sig = _get_api_signature_from_hash_table (FLICKR_API_UPLOAD_URL, extra_params, signing_key,
                                                "POST", AUTHORIZATION_METHOD_OAUTH_1);

  /* Signature is calculated, decode the fields we encoded before */
  _decode_param_from_table_for_signature (extra_params, "title");
  _decode_param_from_table_for_signature (extra_params, "description");
  _decode_param_from_table_for_signature (extra_params, "tags");

  g_hash_table_insert (extra_params, g_strdup ("oauth_signature"), api_sig);
  g_free (signing_key);

  /* Save important data for the callback */
  ard_clos = g_slice_new0 (AsyncRequestData);
  ard_clos->object = G_OBJECT (self);
  ard_clos->soup_session = soup_session;
  ard_clos->soup_message = NULL;
  ard_clos->cancellable = cancellable;
  ard_clos->callback = callback;
  ard_clos->source_tag = fsp_session_upload_async;
  ard_clos->data = data;

  /* Save important data for the upload process itself */
  up_clos = g_slice_new0 (UploadPhotoData);
  up_clos->ar_data = ard_clos;
  up_clos->extra_params = extra_params;

  /* Asynchronously load the contents of the file */
  file = g_file_new_for_uri (fileuri);
  g_file_load_contents_async (file, NULL, _load_file_contents_cb, up_clos);
}

gchar *
fsp_session_upload_finish               (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error)
{
  g_return_val_if_fail (FSP_IS_SESSION (self), NULL);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), NULL);

  return (gchar*) _finish_async_request (G_OBJECT (self), res,
                                         fsp_session_upload_async,
                                         error);
}

void
fsp_session_get_info_async              (FspSession          *self,
                                         const gchar         *photo_id,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             data)
{
  SoupSession *soup_session = NULL;
  gchar *url = NULL;

  g_return_if_fail (FSP_IS_SESSION (self));
  g_return_if_fail (photo_id != NULL);

  /* Build the signed url */
  url = _get_signed_url (self,
                         FLICKR_API_BASE_URL,
                         AUTHORIZATION_METHOD_OAUTH_1,
                         "method", "flickr.photos.getInfo",
                         "photo_id", photo_id,
                         NULL);

  /* Perform the async request */
  soup_session = _get_soup_session (self);
  _perform_async_request (soup_session, url,
                          _photo_get_info_soup_session_cb, G_OBJECT (self),
                          cancellable, callback, fsp_session_get_info_async, data);

  g_free (url);
}

FspDataPhotoInfo *
fsp_session_get_info_finish             (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error)
{
  FspDataPhotoInfo *photo_info = NULL;

  g_return_val_if_fail (FSP_IS_SESSION (self), NULL);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), NULL);

  photo_info =
    FSP_DATA_PHOTO_INFO (_finish_async_request (G_OBJECT (self), res,
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
  SoupSession *soup_session = NULL;
  gchar *url = NULL;

  g_return_if_fail (FSP_IS_SESSION (self));

  /* Build the signed url */
  url = _get_signed_url (self,
                         FLICKR_API_BASE_URL,
                         AUTHORIZATION_METHOD_OAUTH_1,
                         "method", "flickr.photosets.getList",
                         NULL);

  /* Perform the async request */
  soup_session = _get_soup_session (self);
  _perform_async_request (soup_session, url,
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

  return (GSList*) _finish_async_request (G_OBJECT (self), res,
                                          fsp_session_get_photosets_async,
                                          error);
}

void
fsp_session_add_to_photoset_async       (FspSession          *self,
                                         const gchar         *photo_id,
                                         const gchar         *photoset_id,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             data)
{
  SoupSession *soup_session = NULL;
  gchar *url = NULL;

  g_return_if_fail (FSP_IS_SESSION (self));
  g_return_if_fail (photo_id != NULL);
  g_return_if_fail (photoset_id != NULL);

  /* Build the signed url */
  url = _get_signed_url (self,
                         FLICKR_API_BASE_URL,
                         AUTHORIZATION_METHOD_OAUTH_1,
                         "method", "flickr.photosets.addPhoto",
                         "photo_id", photo_id,
                         "photoset_id", photoset_id,
                         NULL);

  /* Perform the async request */
  soup_session = _get_soup_session (self);
  _perform_async_request (soup_session, url,
                          _add_to_photoset_soup_session_cb, G_OBJECT (self),
                          cancellable, callback, fsp_session_add_to_photoset_async, data);

  g_free (url);
}

gboolean
fsp_session_add_to_photoset_finish      (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error)
{
  gpointer result = NULL;

  g_return_val_if_fail (FSP_IS_SESSION (self), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), FALSE);

  result = _finish_async_request (G_OBJECT (self), res,
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
  SoupSession *soup_session = NULL;
  gchar *url = NULL;

  g_return_if_fail (FSP_IS_SESSION (self));
  g_return_if_fail (title != NULL);
  g_return_if_fail (primary_photo_id != NULL);

  /* Build the signed url */
  url = _get_signed_url (self,
                         FLICKR_API_BASE_URL,
                         AUTHORIZATION_METHOD_OAUTH_1,
                         "method", "flickr.photosets.create",
                         "title", title,
                         "description", description ? description : "",
                         "primary_photo_id", primary_photo_id,
                         NULL);

  /* Perform the async request */
  soup_session = _get_soup_session (self);
  _perform_async_request (soup_session, url,
                          _create_photoset_soup_session_cb, G_OBJECT (self),
                          cancellable, callback, fsp_session_create_photoset_async, data);

  g_free (url);
}

gchar *
fsp_session_create_photoset_finish      (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error)
{
  gchar *photoset_id = NULL;

  g_return_val_if_fail (FSP_IS_SESSION (self), NULL);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), NULL);

  photoset_id =
    (gchar*) _finish_async_request (G_OBJECT (self), res,
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
  SoupSession *soup_session = NULL;
  gchar *url = NULL;

  g_return_if_fail (FSP_IS_SESSION (self));

  /* Build the signed url */
  url = _get_signed_url (self,
                         FLICKR_API_BASE_URL,
                         AUTHORIZATION_METHOD_OAUTH_1,
                         "method", "flickr.groups.pools.getGroups",
                         NULL);

  /* Perform the async request */
  soup_session = _get_soup_session (self);
  _perform_async_request (soup_session, url,
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

  return (GSList*) _finish_async_request (G_OBJECT (self), res,
                                          fsp_session_get_groups_async,
                                          error);
}

void
fsp_session_add_to_group_async          (FspSession          *self,
                                         const gchar         *photo_id,
                                         const gchar         *group_id,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             data)
{
  SoupSession *soup_session = NULL;
  gchar *url = NULL;

  g_return_if_fail (FSP_IS_SESSION (self));
  g_return_if_fail (photo_id != NULL);
  g_return_if_fail (group_id != NULL);

  /* Build the signed url */
  url = _get_signed_url (self,
                         FLICKR_API_BASE_URL,
                         AUTHORIZATION_METHOD_OAUTH_1,
                         "method", "flickr.groups.pools.add",
                         "photo_id", photo_id,
                         "group_id", group_id,
                         NULL);

  /* Perform the async request */
  soup_session = _get_soup_session (self);
  _perform_async_request (soup_session, url,
                          _add_to_group_soup_session_cb, G_OBJECT (self),
                          cancellable, callback, fsp_session_add_to_group_async, data);

  g_free (url);
}

gboolean
fsp_session_add_to_group_finish         (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error)
{
  gpointer result = NULL;

  g_return_val_if_fail (FSP_IS_SESSION (self), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), FALSE);

  result = _finish_async_request (G_OBJECT (self), res,
                                  fsp_session_add_to_group_async, error);

  return result ? TRUE : FALSE;
}

void
fsp_session_get_tags_list_async         (FspSession          *self,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             data)
{
  SoupSession *soup_session = NULL;
  gchar *url = NULL;

  g_return_if_fail (FSP_IS_SESSION (self));

  /* Build the signed url */
  url = _get_signed_url (self,
                         FLICKR_API_BASE_URL,
                         AUTHORIZATION_METHOD_OAUTH_1,
                         "method", "flickr.tags.getListUser",
                         NULL);

  /* Perform the async request */
  soup_session = _get_soup_session (self);
  _perform_async_request (soup_session, url,
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

  return (GSList*) _finish_async_request (G_OBJECT (self), res,
                                          fsp_session_get_tags_list_async, error);
}

void
fsp_session_set_license_async           (FspSession          *self,
                                         const gchar         *photo_id,
                                         FspLicense          license,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             data)
{
  SoupSession *soup_session = NULL;
  gchar *license_str = NULL;
  gchar *url = NULL;

  g_return_if_fail (FSP_IS_SESSION (self));
  g_return_if_fail (photo_id != NULL);

  /* Build the signed url */
  license_str = g_strdup_printf ("%d", license);
  url = _get_signed_url (self,
                         FLICKR_API_BASE_URL,
                         AUTHORIZATION_METHOD_OAUTH_1,
                         "method", "flickr.photos.licenses.setLicense",
                         "photo_id", photo_id,
                         "license_id", license_str,
                         NULL);
  g_free (license_str);

  /* Perform the async request */
  soup_session = _get_soup_session (self);
  _perform_async_request (soup_session, url,
                          _set_license_soup_session_cb, G_OBJECT (self),
                          cancellable, callback, fsp_session_set_license_async, data);

  g_free (url);
}

gboolean
fsp_session_set_license_finish          (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error)
{
  gpointer result = NULL;

  g_return_val_if_fail (FSP_IS_SESSION (self), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), FALSE);

  result = _finish_async_request (G_OBJECT (self), res,
                                  fsp_session_set_license_async, error);

  return result ? TRUE : FALSE;
}

void
fsp_session_set_location_async           (FspSession          *self,
                                          const gchar         *photo_id,
                                          FspDataLocation     *location,
                                          GCancellable        *cancellable,
                                          GAsyncReadyCallback  callback,
                                          gpointer             data)
{
  SoupSession *soup_session = NULL;
  gchar lat_str[G_ASCII_DTOSTR_BUF_SIZE];
  gchar lon_str[G_ASCII_DTOSTR_BUF_SIZE];
  gchar *accuracy_str = NULL;
  gchar *context_str = NULL;
  gchar *url = NULL;

  g_return_if_fail (FSP_IS_SESSION (self));
  g_return_if_fail (photo_id != NULL);

  /* Build the signed url */
  g_ascii_dtostr (lat_str, sizeof (lat_str), location->latitude);
  g_ascii_dtostr (lon_str, sizeof (lon_str), location->longitude);
  if ((location->accuracy >= 1) && (location->accuracy <= 16))
    {
      /* restricting to [1..16] since these are the values accepted by flickr */
      accuracy_str = g_strdup_printf ("%d", location->accuracy);
    }
  context_str = g_strdup_printf ("%d", location->context);
  /* FIXME: not sure how to handle the optional 'accuracy' here... */
  url = _get_signed_url (self,
                         FLICKR_API_BASE_URL,
                         AUTHORIZATION_METHOD_OAUTH_1,
                         "method", "flickr.photos.geo.setLocation",
                         "photo_id", photo_id,
                         "lat", lat_str,
                         "lon", lon_str,
                         "accuracy", accuracy_str,
                         "context", context_str,
                         NULL);
  g_free (accuracy_str);
  g_free (context_str);

  /* Perform the async request */
  soup_session = _get_soup_session (self);
  _perform_async_request (soup_session, url,
                          _set_location_soup_session_cb, G_OBJECT (self),
                          cancellable, callback, fsp_session_set_location_async, data);

  g_free (url);
}

gboolean
fsp_session_set_location_finish          (FspSession    *self,
                                          GAsyncResult  *res,
                                          GError       **error)
{
  gpointer result = NULL;

  g_return_val_if_fail (FSP_IS_SESSION (self), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), FALSE);

  result = _finish_async_request (G_OBJECT (self), res,
                                  fsp_session_set_location_async, error);

  return result ? TRUE : FALSE;
}

void
fsp_session_get_location_async           (FspSession          *self,
                                          const gchar         *photo_id,
                                          GCancellable        *cancellable,
                                          GAsyncReadyCallback  callback,
                                          gpointer             data)
{
  SoupSession *soup_session = NULL;
  gchar *url = NULL;

  g_return_if_fail (FSP_IS_SESSION (self));
  g_return_if_fail (photo_id != NULL);

  url = _get_signed_url (self,
                         FLICKR_API_BASE_URL,
                         AUTHORIZATION_METHOD_OAUTH_1,
                         "method", "flickr.photos.geo.getLocation",
                         "photo_id", photo_id,
                         NULL);

  /* Perform the async request */
  soup_session = _get_soup_session (self);
  _perform_async_request (soup_session, url,
                          _get_location_soup_session_cb, G_OBJECT (self),
                          cancellable, callback, fsp_session_get_location_async, data);

  g_free (url);
}

FspDataLocation *
fsp_session_get_location_finish          (FspSession    *self,
                                          GAsyncResult  *res,
                                          GError       **error)
{
  FspDataLocation *location = NULL;

  g_return_val_if_fail (FSP_IS_SESSION (self), NULL);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), NULL);

  location =
    FSP_DATA_LOCATION (_finish_async_request (G_OBJECT (self), res,
                                              fsp_session_get_location_async,
                                              error));
  return location;
}
