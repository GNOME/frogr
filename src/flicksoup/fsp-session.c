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

#include <string.h>

#include "fsp-session.h"

#include "fsp-data.h"
#include "fsp-error.h"
#include "fsp-flickr-proxy.h"
#include "fsp-session-priv.h"
#include "fsp-util.h"

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

  FspFlickrProxy *flickr_proxy;   /* Lazy initialization */
};


/* Properties */

enum  {
  PROP_0,
  PROP_API_KEY,
  PROP_SECRET,
  PROP_TOKEN
};


/* Prototypes */

static FspFlickrProxy *
_get_flickr_proxy                       (FspSession *self);

static void
_get_auth_url_result_cb                 (GObject      *object,
                                         GAsyncResult *result,
                                         gpointer      data);

static void
_complete_auth_cb                       (GObject      *object,
                                         GAsyncResult *result,
                                         gpointer      data);

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
      g_value_set_string (value, fsp_session_get_api_key (self));
      break;
    case PROP_SECRET:
      g_value_set_string (value, fsp_session_get_secret (self));
      break;
    case PROP_TOKEN:
      g_value_set_string (value, fsp_session_get_token (self));
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
  if (self->priv->flickr_proxy)
    g_object_unref (self->priv->flickr_proxy);

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

  self->priv->flickr_proxy = NULL;
  self->priv->api_key = NULL;
  self->priv->secret = NULL;
  self->priv->token = NULL;
  self->priv->frob = NULL;
}

static FspFlickrProxy *
_get_flickr_proxy                       (FspSession *self)
{
  g_return_val_if_fail (FSP_IS_SESSION (self), NULL);

  FspSessionPrivate *priv = self->priv;

  if (priv->flickr_proxy == NULL)
    {
      /* On-demand create the proxy */
      priv->flickr_proxy =
        fsp_flickr_proxy_new (priv->api_key, priv->secret, priv->token);
    }

  return priv->flickr_proxy;
}

static void
_get_auth_url_result_cb                 (GObject      *object,
                                         GAsyncResult *result,
                                         gpointer      data)
{
  g_return_if_fail (FSP_IS_FLICKR_PROXY (object));
  g_return_if_fail (G_IS_ASYNC_RESULT (result));
  g_return_if_fail (data != NULL);

  FspFlickrProxy *proxy = FSP_FLICKR_PROXY(object);
  FspSession *session = NULL;
  GAsyncData *clos = NULL;
  gchar *auth_url = NULL;
  gchar *frob = NULL;
  GError *error = NULL;

  clos = (GAsyncData *) data;
  session = FSP_SESSION (clos->object);

  /* Get and save the frob */
  frob = fsp_flickr_proxy_get_frob_finish (proxy, result, &error);
  if (frob != NULL)
    {
      FspSessionPrivate *priv = session->priv;
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

  build_async_result_and_complete (clos, (gpointer) auth_url, error);
}

static void _complete_auth_cb           (GObject      *object,
                                         GAsyncResult *result,
                                         gpointer      data)
{
  g_return_if_fail (FSP_IS_FLICKR_PROXY (object));
  g_return_if_fail (G_IS_ASYNC_RESULT (result));
  g_return_if_fail (data != NULL);

  FspFlickrProxy *proxy = FSP_FLICKR_PROXY (object);
  FspSession *session = NULL;
  GAsyncData *clos = NULL;
  FspDataAuthToken *auth_token = NULL;
  gboolean retval = FALSE;
  GError *error = NULL;

  clos = (GAsyncData *) data;
  session = FSP_SESSION (clos->object);

  auth_token = fsp_flickr_proxy_get_auth_token_finish (proxy, result, &error);
  if (auth_token != NULL)
    {
      fsp_session_set_token (session, auth_token->token);
      fsp_data_free (FSP_DATA (auth_token));
      retval = TRUE;
    }

  build_async_result_and_complete (clos, GINT_TO_POINTER ((gint) retval), error);
}

/* from fsp-session-priv.h */
FspFlickrProxy *
fsp_session_get_flickr_proxy            (FspSession *self)
{
  g_return_val_if_fail (FSP_IS_SESSION (self), NULL);

  FspFlickrProxy *flickr_proxy = _get_flickr_proxy (self);
  if (flickr_proxy != NULL)
    g_object_ref (flickr_proxy);

  return flickr_proxy;
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

gchar *
fsp_session_get_api_key                 (FspSession *self)
{
  g_return_val_if_fail (FSP_IS_SESSION (self), NULL);

  return g_strdup (self->priv->api_key);
}

gchar *
fsp_session_get_secret                  (FspSession *self)
{
  g_return_val_if_fail (FSP_IS_SESSION (self), NULL);

  return g_strdup (self->priv->secret);
}

gchar *
fsp_session_get_token                   (FspSession *self)
{
  g_return_val_if_fail (FSP_IS_SESSION (self), NULL);

  return g_strdup (self->priv->token);
}

void
fsp_session_set_token                   (FspSession  *self,
                                         const gchar *token)
{
  g_return_if_fail (FSP_IS_SESSION (self));

  FspFlickrProxy *proxy = _get_flickr_proxy (self);

  g_free (self->priv->token);
  self->priv->token = g_strdup (token);

  /* Update flickr proxy's token */
  fsp_flickr_proxy_set_token (proxy, token);
}

/* Get authorization URL */

void
fsp_session_get_auth_url_async          (FspSession          *self,
                                         GCancellable        *c,
                                         GAsyncReadyCallback  cb,
                                         gpointer             data)
{
  g_return_if_fail (FSP_IS_SESSION (self));

  /* We need the frob for this */
  FspFlickrProxy *proxy = _get_flickr_proxy (self);
  GAsyncData *clos = g_slice_new (GAsyncData);
  clos->object = G_OBJECT (self);
  clos->cancellable = c;
  clos->callback = cb;
  clos->source_tag = fsp_session_get_auth_url_async;
  clos->data = data;

  fsp_flickr_proxy_get_frob_async (proxy, c, _get_auth_url_result_cb, clos);
}

gchar *
fsp_session_get_auth_url_finish         (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error)
{
  g_return_val_if_fail (FSP_IS_SESSION (self), NULL);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), NULL);

  gchar *auth_url = NULL;

  /* Check for errors */
  if (!check_async_errors_on_finish (G_OBJECT (self), res,
                                     fsp_session_get_auth_url_async, error))
    {
      GSimpleAsyncResult *simple = NULL;
      gpointer result = NULL;

      /* Get result */
      simple = G_SIMPLE_ASYNC_RESULT (res);
      result = g_simple_async_result_get_op_res_gpointer (simple);
      if (result != NULL)
        auth_url = (gchar *) result;
      else
        g_set_error_literal (error, FSP_ERROR, FSP_ERROR_OTHER,
                             "Internal error");
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
  FspFlickrProxy *proxy = _get_flickr_proxy (self);
  GAsyncData *clos = g_slice_new (GAsyncData);
  clos->object = G_OBJECT (self);
  clos->cancellable = c;
  clos->callback = cb;
  clos->source_tag = fsp_session_complete_auth_async;
  clos->data = data;

  if (priv->frob != NULL)
    {
      /* We need the auth token for this */
      fsp_flickr_proxy_get_auth_token_async (proxy, priv->frob,
                                             c, _complete_auth_cb, clos);
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

gboolean
fsp_session_complete_auth_finish        (FspSession    *self,
                                         GAsyncResult  *res,
                                         GError       **error)
{
  g_return_val_if_fail (FSP_IS_SESSION (self), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), FALSE);

  gboolean auth_done = FALSE;

  /* Check for errors */
  if (!check_async_errors_on_finish (G_OBJECT (self), res,
                                     fsp_session_complete_auth_async, error))
    {
      GSimpleAsyncResult *simple = NULL;
      gpointer result = NULL;

      /* Get result */
      simple = G_SIMPLE_ASYNC_RESULT (res);
      result = g_simple_async_result_get_op_res_gpointer (simple);
      if (result != NULL)
        auth_done = (gboolean) GPOINTER_TO_INT (result);
      else
        g_set_error_literal (error, FSP_ERROR, FSP_ERROR_OTHER,
                             "Internal error");
    }

  return auth_done;
}
