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

#include <flickcurl.h>
#include "frogr-facade.h"
#include "frogr-controller.h"
#include "frogr-picture.h"

#define API_KEY "18861766601de84f0921ce6be729f925"
#define SHARED_SECRET "6233fbefd85f733a"

#define FROGR_FACADE_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), \
                                                                      FROGR_FACADE_TYPE, \
                                                                      FrogrFacadePrivate))

G_DEFINE_TYPE (FrogrFacade, frogr_facade, G_TYPE_OBJECT);

/* Private struct */
typedef struct _FrogrFacadePrivate FrogrFacadePrivate;
struct _FrogrFacadePrivate
{
  FrogrController *controller;
  gboolean authorized;
  flickcurl *fcurl;
  gchar *frob;
  gchar *auth_token;
};


/* Private API */

static void
frogr_facade_finalize (GObject* object)
{
  /* call super class */
  G_OBJECT_CLASS (frogr_facade_parent_class) -> finalize(object);
}

static void
frogr_facade_class_init(FrogrFacadeClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS(klass);

  obj_class -> finalize = frogr_facade_finalize;
  g_type_class_add_private (obj_class, sizeof (FrogrFacadePrivate));
}

static void
frogr_facade_init (FrogrFacade *ffacade)
{
  FrogrFacadePrivate *priv = FROGR_FACADE_GET_PRIVATE (ffacade);

  /* default values */
  priv -> controller = frogr_controller_get_instance ();
  priv -> fcurl = NULL;
  priv -> frob = NULL;
  priv -> authorized = FALSE;
}


/* Public API */

FrogrFacade *
frogr_facade_new (void)
{
  FrogrFacade *ffacade = g_object_new(FROGR_FACADE_TYPE, NULL);
  FrogrFacadePrivate *priv = FROGR_FACADE_GET_PRIVATE (ffacade);

  /* Init flickcurl */
  flickcurl_init();
  priv -> fcurl = flickcurl_new();

  /* Set API key and shared secret */
  flickcurl_set_api_key(priv -> fcurl, API_KEY);
  flickcurl_set_shared_secret(priv -> fcurl, SHARED_SECRET);

  return ffacade;
}

gchar *
frogr_facade_get_authorization_url (FrogrFacade *ffacade)
{
  g_return_if_fail(FROGR_IS_FACADE (ffacade));

  FrogrFacadePrivate *priv = FROGR_FACADE_GET_PRIVATE (ffacade);
  gchar *frob = flickcurl_auth_getFrob (priv -> fcurl);
  gchar *auth_url = NULL;

  if (frob)
    {
      gchar *sign_str;
      gchar *api_sig;

      /* Save frob value */
      priv -> frob = frob;

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
  g_return_if_fail(FROGR_IS_FACADE (ffacade));

  FrogrFacadePrivate *priv = FROGR_FACADE_GET_PRIVATE (ffacade);
  gchar *auth_token;

  /* Check if frob value is present */
  if (!priv -> frob)
    {
      g_debug ("No frob defined");
      return;
    }

  /* Get auth token */
  auth_token = flickcurl_auth_getToken (priv -> fcurl, priv -> frob);
  if (auth_token)
    {
      /* Set and save the auth token */
      flickcurl_set_auth_token(priv -> fcurl, auth_token);
      priv -> auth_token = auth_token;
      priv -> authorized = TRUE;
    }

  return priv -> authorized;
}

gboolean
frogr_facade_is_authorized (FrogrFacade *ffacade)
{
  g_return_if_fail(FROGR_IS_FACADE (ffacade));

  FrogrFacadePrivate *priv = FROGR_FACADE_GET_PRIVATE (ffacade);
  return priv -> authorized;
}

typedef struct {
  FrogrFacade *ffacade;
  GSList *photos_ids;
} _notify_pictures_uploaded_st;

static gboolean
_notify_pictures_uploaded (gpointer data)
{
  _notify_pictures_uploaded_st *npu_st = (_notify_pictures_uploaded_st *) data;
  FrogrFacade *ffacade = npu_st -> ffacade;
  FrogrFacadePrivate *priv = FROGR_FACADE_GET_PRIVATE (ffacade);
  GSList *photos_ids = npu_st -> photos_ids;

  frogr_controller_notify_pictures_uploaded (priv -> controller, photos_ids);

  /* Free memory */
  g_slice_free (_notify_pictures_uploaded_st, npu_st);

  return FALSE;
}

typedef struct {
  FrogrFacade *ffacade;
  GSList *fpictures;
} _upload_pictures_st;

static void
_upload_pictures_thread (gpointer data)
{
  g_return_if_fail (data != NULL);

  _upload_pictures_st *up_st = (_upload_pictures_st *) data;
  FrogrFacade *ffacade = up_st -> ffacade;
  FrogrFacadePrivate *priv = FROGR_FACADE_GET_PRIVATE (ffacade);
  _notify_pictures_uploaded_st *npu_st;

  GSList *fpictures = up_st -> fpictures;
  GSList *item;
  GSList *photos_ids;

  /* Check authorization */
  if (!flickcurl_get_auth_token (priv -> fcurl))
    {
      g_debug ("Not authorized yet");
      return;
    }

  photos_ids = NULL;
  for (item = fpictures; item; item = g_slist_next (item))
    {
      FrogrPicture *fpicture = FROGR_PICTURE (item -> data);
      const gchar *title = frogr_picture_get_title (fpicture);
      const gchar *filepath = frogr_picture_get_filepath (fpicture);
      const gchar *description = frogr_picture_get_description (fpicture);
      const gchar *tags = frogr_picture_get_tags (fpicture);
      gboolean is_public = frogr_picture_is_public (fpicture);
      gboolean is_friend = frogr_picture_is_friend (fpicture);
      gboolean is_family = frogr_picture_is_family (fpicture);
      flickcurl_upload_params *uparams;
      flickcurl_upload_status *ustatus;

      /* Prepare upload params */
      uparams = g_slice_new (flickcurl_upload_params);
      uparams -> title = title;
      uparams -> photo_file = filepath;
      uparams -> description = description;
      uparams -> tags = tags;
      uparams -> is_public = is_public;
      uparams -> is_friend = is_friend;
      uparams -> is_family = is_family;
      uparams -> safety_level = 1; /* Harcoded: 'safe' */
      uparams -> content_type = 1; /* Harcoded: 'photo' */

      /* Upload the test photo (private) */
      g_debug ("\n\nNow uploading picture %s...\n",
               frogr_picture_get_title (fpicture));

      ustatus = flickcurl_photos_upload_params (priv -> fcurl, uparams);
      if (ustatus) {
        g_debug ("[OK] Success uploading a new picture\n");

        /* Print and free upload status */
        g_debug ("\tPhoto upload status:\n");
        g_debug ("\t\tPhotoID: %s\n", ustatus -> photoid);
        g_debug ("\t\tSecret: %s\n", ustatus -> secret);
        g_debug ("\t\tOriginalSecret: %s\n", ustatus -> originalsecret);
        g_debug ("\t\tTicketID: %s\n", ustatus -> ticketid);

        /* Append photo ID to the list */
        photos_ids = g_slist_append (photos_ids, g_strdup (ustatus -> photoid));

        /* Free */
        flickcurl_free_upload_status (ustatus);

      } else {
        g_debug ("[ERRROR] Failure uploading a new picture\n");
      }

      /* Free memory */
      g_object_unref (fpicture);
      g_slice_free (flickcurl_upload_params, uparams);
    }

  /* Free memory */
  g_slist_foreach (fpictures, (GFunc)g_object_unref, NULL);
  g_slist_free (fpictures);
  g_slice_free (_upload_pictures_st, up_st);

  /* At last, just tell the ui to change its state */
  npu_st = g_slice_new (_notify_pictures_uploaded_st);
  npu_st -> ffacade = ffacade;
  npu_st -> photos_ids = photos_ids;

  g_idle_add ((GSourceFunc)_notify_pictures_uploaded, npu_st);
}

void
frogr_facade_upload_pictures (FrogrFacade *ffacade,
                              GSList *fpictures)
{
  g_return_if_fail(FROGR_IS_FACADE (ffacade));

  FrogrFacadePrivate *priv = FROGR_FACADE_GET_PRIVATE (ffacade);
  _upload_pictures_st *up_st;

  /* Check if the list of pictures is not empty */
  if (fpictures != NULL)
    {
      /* Check authorization */
      if (!flickcurl_get_auth_token (priv -> fcurl))
        {
          g_debug ("Not authorized yet");
          return;
        }

      /* Add references */
      g_slist_foreach (fpictures, (GFunc)g_object_ref, NULL);

      /* Create structure to pass to the thread */
      up_st = g_slice_new (_upload_pictures_st);
      up_st -> ffacade = ffacade;
      up_st -> fpictures = g_slist_copy (fpictures);

      /* Initiate the process in another thread */
      g_thread_create ((GThreadFunc)_upload_pictures_thread,
                       up_st, FALSE, NULL);
    }
}
