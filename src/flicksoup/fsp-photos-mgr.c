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
#include "fsp-global-defs.h"
#include "fsp-flickr-parser.h"
#include "fsp-session-priv.h"
#include "fsp-session.h"
#include "fsp-util.h"

#include <libsoup/soup.h>

#define FSP_PHOTOS_MGR_GET_PRIVATE(object)            \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),             \
                                FSP_TYPE_PHOTOS_MGR,  \
                                FspPhotosMgrPrivate))

G_DEFINE_TYPE (FspPhotosMgr, fsp_photos_mgr, G_TYPE_OBJECT);


/* Private struct */
struct _FspPhotosMgrPrivate
{
  FspSession *session;
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
  PROP_SESSION
};


/* Prototypes */

static SoupSession *
_get_soup_session                       (FspPhotosMgr *self);

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
_upload_cancelled_cb                    (GCancellable *cancellable,
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

  /* Unref object */
  if (self->priv->soup_session)
    {
      g_object_unref (self->priv->soup_session);
      self->priv->soup_session = NULL;
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
  self->priv->soup_session = NULL;
}

static SoupSession *
_get_soup_session                       (FspPhotosMgr *self)
{
  g_return_val_if_fail (FSP_IS_PHOTOS_MGR (self), NULL);

  FspPhotosMgrPrivate *priv = self->priv;

  if (priv->soup_session == NULL)
    {
      SoupSession *session = NULL;
      session = fsp_session_get_soup_session (priv->session);
      priv->soup_session = session ? g_object_ref (session) : NULL;
    }

  return priv->soup_session;
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
      FspPhotosMgr *self = NULL;
      SoupSession *soup_session = NULL;
      SoupMessage *msg = NULL;

      /* Get the proxy and the associated message */
      self = FSP_PHOTOS_MGR (ga_clos->object);
      msg = _get_soup_message_for_upload (file, contents, length, extra_params);

      soup_session = _get_soup_session (self);

      /* Connect to the "cancelled" signal thread safely */
      if (ga_clos->cancellable)
        {
          ga_clos->cancellable_id =
            g_cancellable_connect (ga_clos->cancellable,
                                   G_CALLBACK (_upload_cancelled_cb),
                                   soup_session,
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
_upload_cancelled_cb                    (GCancellable *cancellable,
                                         gpointer      data)
{
  SoupSession *soup_session = SOUP_SESSION (data);
  soup_session_abort (soup_session);

  g_debug ("Upload cancelled!");
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
                        (FspFlickrParserFunc) fsp_flickr_parser_get_upload_result,
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
                        (FspFlickrParserFunc) fsp_flickr_parser_get_photo_info,
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
                        (FspFlickrParserFunc) fsp_flickr_parser_get_photosets_list,
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
                        (FspFlickrParserFunc) fsp_flickr_parser_added_to_photoset,
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
                        (FspFlickrParserFunc) fsp_flickr_parser_photoset_created,
                        data);
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

  FspPhotosMgrPrivate *priv = NULL;
  SoupSession *soup_session = NULL;
  GHashTable *extra_params = NULL;
  GAsyncData *ga_clos = NULL;
  UploadPhotoData *up_clos = NULL;
  GFile *file = NULL;
  const gchar *secret = NULL;
  gchar *api_sig = NULL;

  /* Get flickr proxy and extra params (those actually used) */
  priv = self->priv;
  soup_session = _get_soup_session (self);
  extra_params = _get_upload_extra_params (title, description, tags,
                                           is_public, is_family, is_friend,
                                           safety_level, content_type, hidden);

  /* Add remaining parameters to the hash table */
  g_hash_table_insert (extra_params, g_strdup ("api_key"),
                       g_strdup (fsp_session_get_api_key (priv->session)));
  g_hash_table_insert (extra_params, g_strdup ("auth_token"),
                       g_strdup (fsp_session_get_token (priv->session)));

  /* Build the api signature and add it to the hash table */
  secret = fsp_session_get_secret (priv->session);
  api_sig = get_api_signature_from_hash_table (secret, extra_params);
  g_hash_table_insert (extra_params, g_strdup ("api_sig"), api_sig);

  /* Save important data for the callback */
  ga_clos = g_slice_new0 (GAsyncData);
  ga_clos->object = G_OBJECT (self);
  ga_clos->cancellable = cancellable;
  ga_clos->callback = callback;
  ga_clos->source_tag = fsp_photos_mgr_upload_async;
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
fsp_photos_mgr_upload_finish            (FspPhotosMgr  *self,
                                         GAsyncResult  *res,
                                         GError       **error)
{
  g_return_val_if_fail (FSP_IS_PHOTOS_MGR (self), NULL);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), NULL);

  gchar *photo_id = (gchar*) finish_async_request (G_OBJECT (self), res,
                                                   fsp_photos_mgr_upload_async,
                                                   error);
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

  FspPhotosMgrPrivate *priv = self->priv;
  SoupSession *soup_session = NULL;
  const gchar *secret = NULL;
  gchar *url = NULL;
  gchar *signed_query = NULL;

  /* Build the signed url */
  secret = fsp_session_get_secret (priv->session);
  signed_query = get_signed_query (secret,
                                   "method", "flickr.photos.getInfo",
                                   "api_key", fsp_session_get_api_key (priv->session),
                                   "auth_token", fsp_session_get_token (priv->session),
                                   "photo_id", photo_id,
                                   NULL);

  url = g_strdup_printf ("%s/?%s", FLICKR_API_BASE_URL, signed_query);
  g_free (signed_query);

  /* Perform the async request */
  soup_session = _get_soup_session (self);
  perform_async_request (soup_session, url,
                         _photo_get_info_soup_session_cb, G_OBJECT (self),
                         cancellable, callback, fsp_photos_mgr_get_info_async, data);

  g_free (url);
}

FspDataPhotoInfo *
fsp_photos_mgr_get_info_finish          (FspPhotosMgr  *self,
                                         GAsyncResult  *res,
                                         GError       **error)
{
  g_return_val_if_fail (FSP_IS_PHOTOS_MGR (self), NULL);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), NULL);

  FspDataPhotoInfo *photo_info = NULL;

  photo_info =
    FSP_DATA_PHOTO_INFO (finish_async_request (G_OBJECT (self), res,
                                               fsp_photos_mgr_get_info_async,
                                               error));
  return photo_info;
}

void
fsp_photos_mgr_get_photosets_async      (FspPhotosMgr        *self,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             data)
{
  g_return_if_fail (FSP_IS_PHOTOS_MGR (self));

  FspPhotosMgrPrivate *priv = self->priv;
  SoupSession *soup_session = NULL;
  const gchar *secret = NULL;
  gchar *url = NULL;
  gchar *signed_query = NULL;

  /* Build the signed url */
  secret = fsp_session_get_secret (priv->session);
  signed_query = get_signed_query (secret,
                                   "method", "flickr.photosets.getList",
                                   "api_key", fsp_session_get_api_key (priv->session),
                                   "auth_token", fsp_session_get_token (priv->session),
                                   NULL);

  url = g_strdup_printf ("%s/?%s", FLICKR_API_BASE_URL, signed_query);
  g_free (signed_query);

  /* Perform the async request */
  soup_session = _get_soup_session (self);
  perform_async_request (soup_session, url,
                         _get_photosets_soup_session_cb, G_OBJECT (self),
                         cancellable, callback, fsp_photos_mgr_get_photosets_async, data);

  g_free (url);
}

GSList *
fsp_photos_mgr_get_photosets_finish     (FspPhotosMgr  *self,
                                         GAsyncResult  *res,
                                         GError       **error)
{
  g_return_val_if_fail (FSP_IS_PHOTOS_MGR (self), NULL);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), NULL);

  GSList *photosets_list = NULL;

  photosets_list = (GSList*) finish_async_request (G_OBJECT (self), res,
                                                   fsp_photos_mgr_get_photosets_async,
                                                   error);
  return photosets_list;
}

void
fsp_photos_mgr_add_to_photoset_async    (FspPhotosMgr        *self,
                                         const gchar         *photo_id,
                                         const gchar         *photoset_id,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             data)
{
  g_return_if_fail (FSP_IS_PHOTOS_MGR (self));
  g_return_if_fail (photo_id != NULL);
  g_return_if_fail (photoset_id != NULL);

  FspPhotosMgrPrivate *priv = self->priv;
  SoupSession *soup_session = NULL;
  const gchar *secret = NULL;
  gchar *url = NULL;
  gchar *signed_query = NULL;

  /* Build the signed url */
  secret = fsp_session_get_secret (priv->session);
  signed_query = get_signed_query (secret,
                                   "method", "flickr.photosets.addPhoto",
                                   "api_key", fsp_session_get_api_key (priv->session),
                                   "auth_token", fsp_session_get_token (priv->session),
                                   "photo_id", photo_id,
                                   "photoset_id", photoset_id,
                                   NULL);

  url = g_strdup_printf ("%s/?%s", FLICKR_API_BASE_URL, signed_query);
  g_free (signed_query);

  /* Perform the async request */
  soup_session = _get_soup_session (self);
  perform_async_request (soup_session, url,
                         _add_to_photoset_soup_session_cb, G_OBJECT (self),
                         cancellable, callback, fsp_photos_mgr_add_to_photoset_async, data);

  g_free (url);
}

gboolean
fsp_photos_mgr_add_to_photoset_finish   (FspPhotosMgr  *self,
                                         GAsyncResult  *res,
                                         GError       **error)
{
  g_return_val_if_fail (FSP_IS_PHOTOS_MGR (self), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), FALSE);

  gpointer result = NULL;

  result = finish_async_request (G_OBJECT (self), res,
                                 fsp_photos_mgr_add_to_photoset_async, error);

  return result ? TRUE : FALSE;
}

void
fsp_photos_mgr_create_photoset_async    (FspPhotosMgr        *self,
                                         const gchar         *title,
                                         const gchar         *description,
                                         const gchar         *primary_photo_id,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             data)
{
  g_return_if_fail (FSP_IS_PHOTOS_MGR (self));
  g_return_if_fail (title != NULL);
  g_return_if_fail (description != NULL);
  g_return_if_fail (primary_photo_id != NULL);

  FspPhotosMgrPrivate *priv = self->priv;
  SoupSession *soup_session = NULL;
  const gchar *secret = NULL;
  gchar *url = NULL;
  gchar *signed_query = NULL;

  /* Build the signed url */
  secret = fsp_session_get_secret (priv->session);
  signed_query = get_signed_query (secret,
                                   "method", "flickr.photosets.create",
                                   "api_key", fsp_session_get_api_key (priv->session),
                                   "auth_token", fsp_session_get_token (priv->session),
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
                         cancellable, callback, fsp_photos_mgr_create_photoset_async, data);

  g_free (url);
}

gchar *
fsp_photos_mgr_create_photoset_finish   (FspPhotosMgr  *self,
                                         GAsyncResult  *res,
                                         GError       **error)
{
  g_return_val_if_fail (FSP_IS_PHOTOS_MGR (self), NULL);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), NULL);

  gchar *photoset_id = NULL;

  photoset_id =
    (gchar*) finish_async_request (G_OBJECT (self), res,
                                   fsp_photos_mgr_create_photoset_async,
                                   error);
  return photoset_id;
}
