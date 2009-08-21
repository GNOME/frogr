/*
 * frogr-facade.c -- Facade to interact with flickr services
 *
 * Copyright (C) 2009 Mario Sanchez Prada
 * Authors: Mario Sanchez Prada <msanchez@igalia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 3 of the GNU General Public
 * License as published by the Free Software Foundation.
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
#include <flickcurl.h>
#include "frogr-facade.h"
#include "frogr-config.h"
#include "frogr-account.h"

#define API_KEY "18861766601de84f0921ce6be729f925"
#define SHARED_SECRET "6233fbefd85f733a"

#define FROGR_FACADE_GET_PRIVATE(object)             \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object),            \
                                FROGR_FACADE_TYPE,   \
                                FrogrFacadePrivate))

G_DEFINE_TYPE (FrogrFacade, frogr_facade, G_TYPE_OBJECT);

/* Private struct */
typedef struct _FrogrFacadePrivate FrogrFacadePrivate;
struct _FrogrFacadePrivate
{
  FrogrConfig *config;
  flickcurl *fcurl;
};

/* Prototypes */

typedef struct {
  FrogrFacade *ffacade;
  FrogrPicture *fpicture;
  GFunc callback;
  gpointer object;
  gpointer data;
} upload_picture_st;

static void _picture_uploaded (upload_picture_st *up_st);
static void _upload_picture_thread (upload_picture_st *up_st);

/* Private API */

static void
_picture_uploaded (upload_picture_st *up_st)
{
  g_return_if_fail (up_st != NULL);
  GFunc callback = up_st->callback;
  gpointer object = up_st->object;
  gpointer data = up_st->data;

  /* Free memory */
  g_slice_free (upload_picture_st, up_st);

  /* Execute callback */
  if (callback)
    callback (object, data);
}

static void
_upload_picture_thread (upload_picture_st *up_st)
{
  g_return_if_fail (up_st != NULL);
  FrogrFacade *ffacade = up_st->ffacade;
  FrogrPicture *fpicture = up_st->fpicture;

  FrogrFacadePrivate *priv = FROGR_FACADE_GET_PRIVATE (ffacade);
  flickcurl_upload_params *uparams;
  flickcurl_upload_status *ustatus;

  /* Upload picture and gather photo ID */

  /* Prepare upload params */
  uparams = g_slice_new (flickcurl_upload_params);
  uparams->title = frogr_picture_get_title (fpicture);
  uparams->photo_file = frogr_picture_get_filepath (fpicture);
  uparams->description = frogr_picture_get_description (fpicture);
  uparams->tags = frogr_picture_get_tags (fpicture);
  uparams->is_public = frogr_picture_is_public (fpicture);
  uparams->is_friend = frogr_picture_is_friend (fpicture);
  uparams->is_family = frogr_picture_is_family (fpicture);
  uparams->safety_level = 1; /* Harcoded: 'safe' */
  uparams->content_type = 1; /* Harcoded: 'photo' */

  /* Upload the test photo (private) */
  g_debug ("\n\nNow uploading picture %s...\n",
           frogr_picture_get_title (fpicture));

  ustatus = flickcurl_photos_upload_params (priv->fcurl, uparams);
  if (ustatus) {
    g_debug ("[OK] Success uploading a new picture\n");

    /* Save photo ID along with the picture */
    frogr_picture_set_id (fpicture, ustatus->photoid);

    /* Print and free upload status */
    g_debug ("\tPhoto upload status:");
    g_debug ("\t\tPhotoID: %s", ustatus->photoid);
    g_debug ("\t\tSecret: %s", ustatus->secret);
    g_debug ("\t\tOriginalSecret: %s", ustatus->originalsecret);
    g_debug ("\t\tTicketID: %s", ustatus->ticketid);

    /* Free */
    flickcurl_free_upload_status (ustatus);

  } else {
    g_debug ("[ERRROR] Failure uploading a new picture\n");
  }

  /* Free memory */
  g_slice_free (flickcurl_upload_params, uparams);

  /* At last, just call the return callback */
  g_idle_add ((GSourceFunc)_picture_uploaded, up_st);
}

static void
frogr_facade_finalize (GObject* object)
{
  FrogrFacadePrivate *priv = FROGR_FACADE_GET_PRIVATE (object);

  /* Free memory */
  g_object_unref (priv->config);
  flickcurl_free (priv->fcurl);

  /* Call superclass */
  G_OBJECT_CLASS (frogr_facade_parent_class)->finalize(object);
}

static void
frogr_facade_class_init(FrogrFacadeClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS(klass);

  obj_class->finalize = frogr_facade_finalize;
  g_type_class_add_private (obj_class, sizeof (FrogrFacadePrivate));
}

static void
frogr_facade_init (FrogrFacade *ffacade)
{
  FrogrFacadePrivate *priv = FROGR_FACADE_GET_PRIVATE (ffacade);
  FrogrAccount *faccount;
  const gchar *token;

  /* Get config */
  priv->config = frogr_config_get_instance ();

  /* Init flickcurl */
  flickcurl_init();
  priv->fcurl = flickcurl_new();

  /* Set API key and shared secret */
  flickcurl_set_api_key(priv->fcurl, API_KEY);
  flickcurl_set_shared_secret(priv->fcurl, SHARED_SECRET);

  /* If available, set token */
  faccount = frogr_config_get_account (priv->config);
  token = frogr_account_get_token (faccount);
  if (token != NULL)
    flickcurl_set_auth_token (priv->fcurl, token);
}


/* Public API */

FrogrFacade *
frogr_facade_new (void)
{
  return FROGR_FACADE (g_object_new(FROGR_FACADE_TYPE, NULL));
}

gchar *
frogr_facade_get_authorization_url (FrogrFacade *ffacade)
{
  g_return_val_if_fail(FROGR_IS_FACADE (ffacade), NULL);

  FrogrFacadePrivate *priv = FROGR_FACADE_GET_PRIVATE (ffacade);
  gchar *frob = flickcurl_auth_getFrob (priv->fcurl);
  gchar *auth_url = NULL;


  /* Get auth url */
  if (frob)
    {
      FrogrConfig *fconfig = frogr_config_get_instance ();
      gchar *sign_str;
      gchar *api_sig;

      /* Save frob value */
      frogr_account_set_frob (frogr_config_get_account (fconfig), frob);

      /* Build the authorization url */
      sign_str = g_strdup_printf ("%sapi_key%sfrob%spermswrite",
                                  SHARED_SECRET, API_KEY, frob);
      api_sig = g_compute_checksum_for_string (G_CHECKSUM_MD5, sign_str, -1);
      auth_url = g_strdup_printf ("http://flickr.com/services/auth/?api_key=%s"
                                  "&perms=write&frob=%s&api_sig=%s",
                                  API_KEY, frob, api_sig);
      /* Free */
      g_free (sign_str);
      g_free (api_sig);
    }

  return auth_url;
}

gboolean
frogr_facade_complete_authorization (FrogrFacade *ffacade)
{
  g_return_val_if_fail(FROGR_IS_FACADE (ffacade), FALSE);

  FrogrFacadePrivate *priv = FROGR_FACADE_GET_PRIVATE (ffacade);
  FrogrConfig *fconfig = frogr_config_get_instance ();
  FrogrAccount *faccount = frogr_config_get_account (fconfig);
  gchar *auth_token = NULL;
  gchar *frob = NULL;

  /* Check if frob value is present */
  frob = (gchar *)frogr_account_get_frob (faccount);
  if (frob == NULL)
    {
      g_debug ("No frob defined");
      return FALSE;
    }

  /* Get auth token */
  auth_token = flickcurl_auth_getToken (priv->fcurl, frob);
  if (auth_token)
    {
      /* Set and save the auth token and the settings to disk */
      flickcurl_set_auth_token(priv->fcurl, auth_token);
      frogr_account_set_token (faccount, auth_token);
      frogr_config_save (fconfig);
    }

  return (auth_token != NULL);
}

gboolean
frogr_facade_is_authorized (FrogrFacade *ffacade)
{
  g_return_val_if_fail(FROGR_IS_FACADE (ffacade), FALSE);

  FrogrFacadePrivate *priv = FROGR_FACADE_GET_PRIVATE (ffacade);
  return (flickcurl_get_auth_token (priv->fcurl) != NULL);
}

void
frogr_facade_upload_picture (FrogrFacade *ffacade,
                             FrogrPicture *fpicture,
                             GFunc callback,
                             gpointer object,
                             gpointer data)
{
  g_return_if_fail(FROGR_IS_FACADE (ffacade));

  upload_picture_st *up_st;

  /* Create structure to pass to the thread */
  up_st = g_slice_new (upload_picture_st);
  up_st->ffacade = ffacade;
  up_st->fpicture = fpicture;
  up_st->callback = callback;
  up_st->object = object;
  up_st->data = data;

  /* Initiate the process in another thread */
  g_thread_create ((GThreadFunc)_upload_picture_thread,
                   up_st, FALSE, NULL);
}
