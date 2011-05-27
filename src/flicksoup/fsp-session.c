/*
 * fsp-session.c
 *
 * Copyright (C) 2010-2011 Mario Sanchez Prada
 * Authors: Mario Sanchez Prada <msanchez@igalia.com>
 *
 * Some parts of this file were based on source code from the libsoup
 * library, licensed as LGPLv2 (Copyright 2008 Red Hat, Inc.)
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

#include <config.h>

#ifdef HAVE_LIBSOUP_GNOME
#include <libsoup/soup-gnome.h>
#else
#include <libsoup/soup.h>
#endif

#include <stdarg.h>
#include <string.h>

#define FLICKR_API_BASE_URL   "http://api.flickr.com/services/rest"
#define FLICKR_API_UPLOAD_URL "http://api.flickr.com/services/upload"

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
  gchar *frob;

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
_wrote_body_data_cb                     (SoupMessage *msg,
                                         SoupBuffer  *buffer,
                                         gpointer     data);

static GHashTable *
_get_params_table_from_valist           (const gchar *first_param,
                                         va_list      args);

/* This function is based in append_form_encoded() from libsoup's
   SoupForm, licensed as LGPLv2 (Copyright 2008 Red Hat, Inc.) */
static gchar *
_encode_query_value (const char *value);

static gchar *
_get_signed_query_with_params           (const gchar      *api_sig,
                                         GHashTable       *params_table);
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

gchar *
_get_api_signature_from_hash_table      (const gchar *shared_secret,
                                         GHashTable  *params_table);
gchar *
_get_signed_query                       (const gchar *shared_secret,
                                         const gchar *first_param,
                                         ... );

void
_perform_async_request                  (SoupSession         *soup_session,
                                         const gchar         *url,
                                         SoupSessionCallback  request_cb,
                                         GObject             *source_object,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             source_tag,
                                         gpointer             data);

void
_soup_session_cancelled_cb              (GCancellable *cancellable,
                                         gpointer      data);

void
_handle_soup_response                   (SoupMessage   *msg,
                                         FspParserFunc  parserFunc,
                                         gpointer       data);

void
_build_async_result_and_complete        (AsyncRequestData *clos,
                                         gpointer    result,
                                         GError     *error);

gpointer
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
  g_free (self->priv->frob);

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
  self->priv->frob = NULL;

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
_get_frob_soup_session_cb               (SoupSession *session,
                                         SoupMessage *msg,
                                         gpointer data)
{
  g_assert (SOUP_IS_MESSAGE (msg));
  g_assert (data != NULL);

  /* Handle message with the right parser */
  _handle_soup_response (msg,
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
  _handle_soup_response (msg,
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
  gchar *fileuri;

  /* Gather needed information */
  fileuri = g_file_get_uri (file);
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
  soup_multipart_append_form_file (mpart, "photo", fileuri,
                                   mime_type, buffer);

  /* Get the associated message */
  msg = soup_form_request_new_from_multipart (FLICKR_API_UPLOAD_URL, mpart);

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
        g_hash_table_insert (table, g_strdup (p), g_strdup (v));
      else
        DEBUG ("Missing value for %s. Ignoring parameter.", p);
    }

  return table;
}

/* This function is based in append_form_encoded() from libsoup's
   SoupForm, licensed as LGPLv2 (Copyright 2008 Red Hat, Inc.) */
static gchar *
_encode_query_value (const char *value)
{
  GString *result = NULL;
  const unsigned char *str = NULL;

  result = g_string_new ("");
  str = (const unsigned char *) value;

  while (*str) {
    if (*str == ' ') {
      g_string_append_c (result, '+');
      str++;
    } else if (!g_ascii_isalnum (*str))
      g_string_append_printf (result, "%%%02X", (int)*str++);
    else
      g_string_append_c (result, *str++);
  }

  return g_string_free (result, FALSE);
}

static gchar *
_get_signed_query_with_params           (const gchar      *api_sig,
                                         GHashTable       *params_table)
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
          gchar *key = (gchar*) k->data;
          gchar *value = g_hash_table_lookup (params_table, key);
          gchar *actual_value = NULL;

          /* Do not encode basic pairs key-value */
          if (g_strcmp0 (key, "api_key") && g_strcmp0 (key, "auth_token")
              && g_strcmp0 (key, "method") && g_strcmp0 (key, "frob"))
            actual_value = _encode_query_value (value);
          else
            actual_value = g_strdup (value);

          url_params_array[i++] = g_strdup_printf ("%s=%s", key, actual_value);
          g_free (actual_value);
        }

      /* Add those to the params array (space previously reserved) */
      url_params_array[i] = g_strdup_printf ("api_sig=%s", api_sig);

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

  g_assert (SOUP_IS_MESSAGE (msg));

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

gchar *
_get_api_signature_from_hash_table      (const gchar *shared_secret,
                                         GHashTable  *params_table)
{
  GList *keys = NULL;
  gchar *api_sig = NULL;

  g_return_val_if_fail (shared_secret != NULL, NULL);
  g_return_val_if_fail (params_table != NULL, NULL);

  /* Get ownership of the table */
  g_hash_table_ref (params_table);

  /* Get a list of keys */
  keys = g_hash_table_get_keys (params_table);
  if (keys != NULL)
    {
      gchar **sign_str_array = NULL;
      gchar *sign_str = NULL;
      GList *k = NULL;
      gint i = 0;

      /* Sort the list */
      keys = g_list_sort (keys, (GCompareFunc) g_strcmp0);

      /* Build gchar** arrays for building the signature string */
      sign_str_array = g_new0 (gchar*, (2 * g_list_length (keys)) + 2);

      /* Fill arrays */
      sign_str_array[i++] = g_strdup (shared_secret);
      for (k = keys; k; k = g_list_next (k))
        {
          const gchar *key = (gchar*) k->data;
          const gchar *value = g_hash_table_lookup (params_table, key);

          sign_str_array[i++] = g_strdup (key);
          sign_str_array[i++] = g_strdup (value);
        }
      sign_str_array[i] = NULL;

      /* Get the signature string and calculate the api_sig value */
      sign_str = g_strjoinv (NULL, sign_str_array);
      api_sig = g_compute_checksum_for_string (G_CHECKSUM_MD5, sign_str, -1);

      /* Free */
      g_free (sign_str);
      g_strfreev (sign_str_array);
    }

  g_list_free (keys);
  g_hash_table_unref (params_table);

  return api_sig;
}

gchar *
_get_signed_query                       (const gchar *shared_secret,
                                         const gchar *first_param,
                                         ... )
{
  va_list args;
  GHashTable *table = NULL;
  gchar *api_sig = NULL;
  gchar *retval = NULL;

  g_return_val_if_fail (shared_secret != NULL, NULL);
  g_return_val_if_fail (first_param != NULL, NULL);

  va_start (args, first_param);

  /* Get the hash table for the params and the API signature from it */
  table = _get_params_table_from_valist (first_param, args);
  api_sig = _get_api_signature_from_hash_table (shared_secret, table);

  /* Get the signed URL with the needed params */
  if ((table != NULL) && (api_sig != NULL))
    retval = _get_signed_query_with_params (api_sig, table);

  g_hash_table_unref (table);
  g_free (api_sig);

  va_end (args);

  return retval;
}

void
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

void
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

void
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

void
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
}

gpointer
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
                                         const char *host, const char *port,
                                         const char *username, const char *password)
{
  SoupURI *proxy_uri = NULL;

  g_return_val_if_fail (FSP_IS_SESSION (self), FALSE);

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

  /* Set/unset the proxy if needed */
  if ((!self->priv->proxy_uri && proxy_uri)
      || (self->priv->proxy_uri && !proxy_uri)
      || (self->priv->proxy_uri && proxy_uri && !soup_uri_equal (self->priv->proxy_uri, proxy_uri)))
    {
      g_object_set (G_OBJECT (self->priv->soup_session),
                    SOUP_SESSION_PROXY_URI,
                    proxy_uri,
                    NULL);

      /* Save internal reference to the proxy */
      if (self->priv->proxy_uri)
        soup_uri_free (self->priv->proxy_uri);

      self->priv->proxy_uri = proxy_uri;

      /* Proxy configuration actually changed */
      return TRUE;
    }

  /* Proxy configuration has not changed */
  return FALSE;
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
  FspSessionPrivate *priv = NULL;
  gchar *url = NULL;
  gchar *signed_query = NULL;

  g_return_if_fail (FSP_IS_SESSION (self));

  /* Build the signed url */
  priv = self->priv;
  signed_query = _get_signed_query (priv->secret,
                                    "method", "flickr.auth.getFrob",
                                    "api_key", priv->api_key,
                                    NULL);
  url = g_strdup_printf ("%s/?%s", FLICKR_API_BASE_URL, signed_query);
  g_free (signed_query);

  /* Perform the async request */
  _perform_async_request (priv->soup_session, url,
                          _get_frob_soup_session_cb, G_OBJECT (self),
                          c, cb, fsp_session_get_auth_url_async, data);

  g_free (url);
}

gchar *
fsp_session_get_auth_url_finish         (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error)
{
  gchar *frob = NULL;
  gchar *auth_url = NULL;

  g_return_val_if_fail (FSP_IS_SESSION (self), NULL);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), NULL);

  frob = (gchar*) _finish_async_request (G_OBJECT (self), res,
                                         fsp_session_get_auth_url_async, error);
  /* Build the auth URL from the frob */
  if (frob != NULL)
    {
      FspSessionPrivate *priv = self->priv;
      gchar *signed_query = NULL;

      /* Save the frob */
      g_free (priv->frob);
      priv->frob = frob;

      /* Build the authorization url */
      signed_query = _get_signed_query (priv->secret,
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
  FspSessionPrivate *priv = NULL;

  g_return_if_fail (FSP_IS_SESSION (self));
  g_return_if_fail (cb != NULL);

  priv = self->priv;
  if (priv->frob != NULL)
    {
      gchar *signed_query = NULL;
      gchar *url = NULL;

      /* Build the signed url */
      signed_query = _get_signed_query (priv->secret,
                                        "method", "flickr.auth.getToken",
                                        "api_key", priv->api_key,
                                        "frob", priv->frob,
                                        NULL);
      url = g_strdup_printf ("%s/?%s", FLICKR_API_BASE_URL, signed_query);
      g_free (signed_query);

      /* Perform the async request */
      _perform_async_request (priv->soup_session, url,
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
  FspDataAuthToken *auth_token = NULL;

  g_return_val_if_fail (FSP_IS_SESSION (self), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), FALSE);

  auth_token =
    FSP_DATA_AUTH_TOKEN (_finish_async_request (G_OBJECT (self), res,
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
  FspSessionPrivate *priv = NULL;

  g_return_if_fail (FSP_IS_SESSION (self));
  g_return_if_fail (cb != NULL);

  priv = self->priv;
  if (priv->token != NULL)
    {
      gchar *signed_query = NULL;
      gchar *url = NULL;

      /* Build the signed url */
      signed_query = _get_signed_query (priv->secret,
                                        "method", "flickr.auth.checkToken",
                                        "api_key", priv->api_key,
                                        "auth_token", priv->token,
                                        NULL);
      url = g_strdup_printf ("%s/?%s", FLICKR_API_BASE_URL, signed_query);
      g_free (signed_query);

      /* Perform the async request */
      _perform_async_request (priv->soup_session, url,
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
      gchar *signed_query = NULL;
      gchar *url = NULL;

      /* Build the signed url */
      signed_query = _get_signed_query (priv->secret,
                                        "method", "flickr.people.getUploadStatus",
                                        "api_key", priv->api_key,
                                        "auth_token", priv->token,
                                        NULL);
      url = g_strdup_printf ("%s/?%s", FLICKR_API_BASE_URL, signed_query);
      g_free (signed_query);

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
  gchar *api_sig = NULL;

  g_return_if_fail (FSP_IS_SESSION (self));
  g_return_if_fail (fileuri != NULL);

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
  api_sig = _get_api_signature_from_hash_table (priv->secret, extra_params);
  g_hash_table_insert (extra_params, g_strdup ("api_sig"), api_sig);

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
  FspSessionPrivate *priv = NULL;
  SoupSession *soup_session = NULL;
  gchar *url = NULL;
  gchar *signed_query = NULL;

  g_return_if_fail (FSP_IS_SESSION (self));
  g_return_if_fail (photo_id != NULL);

  /* Build the signed url */
  priv = self->priv;
  signed_query = _get_signed_query (priv->secret,
                                    "method", "flickr.photos.getInfo",
                                    "api_key", priv->api_key,
                                    "auth_token", priv->token,
                                    "photo_id", photo_id,
                                    NULL);

  url = g_strdup_printf ("%s/?%s", FLICKR_API_BASE_URL, signed_query);
  g_free (signed_query);

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
  FspSessionPrivate *priv = NULL;;
  SoupSession *soup_session = NULL;
  gchar *url = NULL;
  gchar *signed_query = NULL;

  g_return_if_fail (FSP_IS_SESSION (self));

  /* Build the signed url */
  priv = self->priv;
  signed_query = _get_signed_query (priv->secret,
                                    "method", "flickr.photosets.getList",
                                    "api_key", priv->api_key,
                                    "auth_token", priv->token,
                                    NULL);

  url = g_strdup_printf ("%s/?%s", FLICKR_API_BASE_URL, signed_query);
  g_free (signed_query);

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
  FspSessionPrivate *priv = NULL;
  SoupSession *soup_session = NULL;
  gchar *url = NULL;
  gchar *signed_query = NULL;

  g_return_if_fail (FSP_IS_SESSION (self));
  g_return_if_fail (photo_id != NULL);
  g_return_if_fail (photoset_id != NULL);

  /* Build the signed url */
  priv = self->priv;
  signed_query = _get_signed_query (priv->secret,
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
  FspSessionPrivate *priv = NULL;
  SoupSession *soup_session = NULL;
  gchar *url = NULL;
  gchar *signed_query = NULL;

  g_return_if_fail (FSP_IS_SESSION (self));
  g_return_if_fail (title != NULL);
  g_return_if_fail (primary_photo_id != NULL);

  /* Build the signed url */
  priv = self->priv;
  signed_query = _get_signed_query (priv->secret,
                                    "method", "flickr.photosets.create",
                                    "api_key", priv->api_key,
                                    "auth_token", priv->token,
                                    "title", title,
                                    "description", description ? description : "",
                                    "primary_photo_id", primary_photo_id,
                                    NULL);

  url = g_strdup_printf ("%s/?%s", FLICKR_API_BASE_URL, signed_query);
  g_free (signed_query);

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
  FspSessionPrivate *priv = NULL;
  SoupSession *soup_session = NULL;
  gchar *url = NULL;
  gchar *signed_query = NULL;

  g_return_if_fail (FSP_IS_SESSION (self));

  /* Build the signed url */
  priv = self->priv;
  signed_query = _get_signed_query (priv->secret,
                                    "method", "flickr.groups.pools.getGroups",
                                    "api_key", priv->api_key,
                                    "auth_token", priv->token,
                                    NULL);

  url = g_strdup_printf ("%s/?%s", FLICKR_API_BASE_URL, signed_query);
  g_free (signed_query);

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
  FspSessionPrivate *priv = NULL;
  SoupSession *soup_session = NULL;
  gchar *url = NULL;
  gchar *signed_query = NULL;

  g_return_if_fail (FSP_IS_SESSION (self));
  g_return_if_fail (photo_id != NULL);
  g_return_if_fail (group_id != NULL);

  /* Build the signed url */
  priv = self->priv;
  signed_query = _get_signed_query (priv->secret,
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
  FspSessionPrivate *priv = NULL;
  SoupSession *soup_session = NULL;
  gchar *url = NULL;
  gchar *signed_query = NULL;

  g_return_if_fail (FSP_IS_SESSION (self));

  /* Build the signed url */
  priv = self->priv;
  signed_query = _get_signed_query (priv->secret,
                                    "method", "flickr.tags.getListUser",
                                    "api_key", priv->api_key,
                                    "auth_token", priv->token,
                                    NULL);

  url = g_strdup_printf ("%s/?%s", FLICKR_API_BASE_URL, signed_query);
  g_free (signed_query);

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
