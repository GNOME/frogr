/*
 * fsp-util.c
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

#include "fsp-util.h"

#include "fsp-error.h"

#include <stdarg.h>
#include <libsoup/soup.h>

typedef struct
{
  GCancellable        *cancellable;
  gulong               cancellable_id;
} GCancellableData;


static GHashTable *
_get_params_table_from_valist           (const gchar *first_param,
                                         va_list      args)
{
  g_return_val_if_fail (first_param != NULL, NULL);
  g_return_val_if_fail (args != NULL, NULL);

  GHashTable *table = NULL;
  gchar *p, *v;

  table = g_hash_table_new_full (g_str_hash, g_str_equal,
                                 (GDestroyNotify)g_free,
                                 (GDestroyNotify)g_free);

  /* Fill the hash table */
  for (p = (gchar *) first_param; p; p = va_arg (args, gchar*))
    {
      v = va_arg (args, gchar*);

      /* Ignore parameter with no value */
      if (v != NULL)
        g_hash_table_insert (table, g_strdup (p), soup_uri_encode (v, NULL));
      else
        g_warning ("Missing value for %s. Ignoring parameter.", p);
    }

  return table;
}

static gchar *
_get_signed_query_with_params           (const gchar      *api_sig,
                                         GHashTable       *params_table)
{
  g_return_val_if_fail (params_table != NULL, NULL);
  g_return_val_if_fail (api_sig != NULL, NULL);

  GList *keys = NULL;
  gchar *retval = NULL;

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
          gchar *value = soup_uri_decode (g_hash_table_lookup (params_table, key));
          url_params_array[i++] = g_strdup_printf ("%s=%s", key, value);
          g_free (value);
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

static void
_soup_session_cancelled_cb              (GCancellable *cancellable,
                                         gpointer      data)
{
  SoupSession *soup_session = SOUP_SESSION (data);
  soup_session_abort (soup_session);
}

gchar *
get_api_signature                      (const gchar *shared_secret,
                                        const gchar *first_param,
                                        ... )
{
  g_return_val_if_fail (shared_secret != NULL, NULL);

  va_list args;
  GHashTable *table = NULL;
  gchar *api_sig = NULL;

  va_start (args, first_param);

  /* Get the hash table for the params and the API signature from it */
  table = _get_params_table_from_valist (first_param, args);
  api_sig = get_api_signature_from_hash_table (shared_secret, table);

  g_hash_table_unref (table);
  va_end (args);

  return api_sig;
}

gchar *
get_api_signature_from_hash_table       (const gchar *shared_secret,
                                         GHashTable  *params_table)
{
  g_return_val_if_fail (shared_secret != NULL, NULL);
  g_return_val_if_fail (params_table != NULL, NULL);

  GList *keys = NULL;
  gchar *api_sig = NULL;

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
      sign_str_array[i++] = (gchar *) shared_secret;
      for (k = keys; k; k = g_list_next (k))
        {
          gchar *key = (gchar*) k->data;
          gchar *value = soup_uri_decode (g_hash_table_lookup (params_table, key));

          sign_str_array[i++] = key;
          sign_str_array[i++] = value;
        }

      /* Get the signature string and calculate the api_sig value */
      sign_str = g_strjoinv (NULL, sign_str_array);
      api_sig = g_compute_checksum_for_string (G_CHECKSUM_MD5, sign_str, -1);

      /* Free */
      g_free (sign_str);
      g_free (sign_str_array); /* Don't use g_strfreev here */
    }

  g_list_free (keys);
  g_hash_table_unref (params_table);

  return api_sig;
}

/**
 * get_signed_query:
 * @shared_secret: secret associated to the Flickr API key being used
 * @first_param: key of the first parameter
 * @...: value for the first parameter, followed optionally by more
 *  key/value parameters pairs, followed by %NULL
 *
 * Gets a signed query part for a given set of pairs
 * key/value. The returned string should be freed with g_free() 
 * when no longer needed.
 *
 * Returns: a newly-allocated @str with the signed query
 */
gchar *
get_signed_query                        (const gchar *shared_secret,
                                         const gchar *first_param,
                                         ... )
{
  g_return_val_if_fail (shared_secret != NULL, NULL);
  g_return_val_if_fail (first_param != NULL, NULL);

  va_list args;
  GHashTable *table = NULL;
  gchar *api_sig = NULL;
  gchar *retval = NULL;

  va_start (args, first_param);

  /* Get the hash table for the params and the API signature from it */
  table = _get_params_table_from_valist (first_param, args);
  api_sig = get_api_signature_from_hash_table (shared_secret, table);

  /* Get the signed URL with the needed params */
  if ((table != NULL) && (api_sig != NULL))
    retval = _get_signed_query_with_params (api_sig, table);

  g_hash_table_unref (table);
  g_free (api_sig);

  va_end (args);

  return retval;
}

gchar *
get_signed_query_from_hash_table        (const gchar *shared_secret,
                                         GHashTable  *params_table)
{
  g_return_val_if_fail (shared_secret != NULL, NULL);
  g_return_val_if_fail (params_table != NULL, NULL);

  gchar *api_sig = NULL;
  gchar *retval = NULL;

  /* Get api signature */
  api_sig = get_api_signature_from_hash_table (shared_secret, params_table);

  /* Get the signed URL with the needed params */
  if ((params_table != NULL) && (api_sig != NULL))
    retval = _get_signed_query_with_params (api_sig, params_table);

  g_free (api_sig);

  return retval;
}

gboolean
check_errors_on_soup_response           (SoupMessage  *msg,
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

void
perform_async_request                   (SoupSession         *soup_session,
                                         const gchar         *url,
                                         SoupSessionCallback  request_cb,
                                         GObject             *source_object,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             source_tag,
                                         gpointer             data)
{
  g_return_if_fail (SOUP_IS_SESSION (soup_session));
  g_return_if_fail (url != NULL);
  g_return_if_fail (request_cb != NULL);
  g_return_if_fail (callback != NULL);

  GAsyncData *clos = NULL;
  SoupMessage *msg = NULL;

  /* Save important data for the callback */
  clos = g_slice_new0 (GAsyncData);
  clos->object = source_object;
  clos->cancellable = cancellable;
  clos->callback = callback;
  clos->source_tag = source_tag;
  clos->data = data;

  /* Connect to the "cancelled" signal thread safely */
  if (clos->cancellable)
    {
      clos->cancellable_id =
        g_cancellable_connect (clos->cancellable,
                               G_CALLBACK (_soup_session_cancelled_cb),
                               soup_session,
                               NULL);
    }

  /* Build and queue the message */
  msg = soup_message_new (SOUP_METHOD_GET, url);
  soup_session_queue_message (soup_session, msg, request_cb, clos);
}

void
build_async_result_and_complete         (GAsyncData *clos,
                                         gpointer    result,
                                         GError     *error)
{
  g_assert (clos != NULL);

  GSimpleAsyncResult *res = NULL;
  GObject *object = NULL;
  GCancellableData *cancellable_data = NULL;
  GCancellable *cancellable = NULL;
  gulong cancellable_id = 0;
  GAsyncReadyCallback  callback = NULL;
  gpointer source_tag;
  gpointer data;

  /* Get data from closure, and free it */
  object = clos->object;
  cancellable = clos->cancellable;
  cancellable_id = clos->cancellable_id;
  callback = clos->callback;
  source_tag = clos->source_tag;
  data = clos->data;
  g_slice_free (GAsyncData, clos);

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

gboolean
check_async_errors_on_finish            (GObject       *object,
                                         GAsyncResult  *res,
                                         gpointer       source_tag,
                                         GError       **error)
{
  g_return_val_if_fail (G_IS_OBJECT (object), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), FALSE);

  gboolean errors_found = TRUE;

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
