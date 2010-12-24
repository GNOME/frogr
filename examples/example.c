/*
 * example.c -- Random example to test the library
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

#include <stdio.h>
#include <glib.h>
#include <flicksoup/flicksoup.h>

#define API_KEY "18861766601de84f0921ce6be729f925"
#define SHARED_SECRET "6233fbefd85f733a"

#define TEST_PHOTO "./examples/testphoto.png"

static gchar *uploaded_photo_id = NULL;
static gchar *created_photoset_id = NULL;

/* Prototypes */

void upload_cb (GObject *object, GAsyncResult *res, gpointer source_func);
void added_to_photoset_cb (GObject *object, GAsyncResult *res, gpointer unused);
void photoset_created_cb (GObject *object, GAsyncResult *res, gpointer unused);
void get_photosets_cb (GObject *object, GAsyncResult *res, gpointer unused);
void photo_get_info_cb (GObject *object, GAsyncResult *res, gpointer unused);
void complete_auth_cb (GObject *object, GAsyncResult *res, gpointer unused);
void get_auth_url_cb (GObject *object, GAsyncResult *res, gpointer unused);
gboolean do_work (gpointer unused);

/* Implementations */

void
upload_cb                               (GObject      *object,
                                         GAsyncResult *res,
                                         gpointer      source_func)
{
  FspPhotosMgr *photos_mgr = FSP_PHOTOS_MGR (object);
  GError *error = NULL;

  g_free (uploaded_photo_id);
  uploaded_photo_id = fsp_photos_mgr_upload_finish (photos_mgr, res, &error);

  if (error != NULL)
    {
      g_print ("Error uploading picture: %s\n", error->message);
      g_error_free (error);
    }
  else
    {
      g_print ("[upload_cb]::Success! Photo ID: %s\n\n", uploaded_photo_id);

      /* Make a pause before continuing */
      g_print ("Press ENTER to continue...\n\n");
      getchar ();

      if (source_func == complete_auth_cb)
        {
          /* Continue getting info about the picture */
          g_print ("Getting info for photo %s...\n", uploaded_photo_id);
          fsp_photos_mgr_get_info_async (photos_mgr, uploaded_photo_id, NULL,
                                         photo_get_info_cb, NULL);
        }
      else if (source_func == photoset_created_cb)
        {
          /* Continue adding the picture to the photoset */
          g_print ("Creatine a new photoset...\n");
          fsp_photos_mgr_add_to_photoset_async (photos_mgr,
                                                uploaded_photo_id,
                                                created_photoset_id,
                                                NULL, added_to_photoset_cb, NULL);
        }
    }
}

void
added_to_photoset_cb                    (GObject      *object,
                                         GAsyncResult *res,
                                         gpointer      user_data)
{
  FspPhotosMgr *photos_mgr = FSP_PHOTOS_MGR (object);
  GError *error = NULL;
  gboolean result = FALSE;

  result = fsp_photos_mgr_add_to_photoset_finish (photos_mgr, res, &error);
  if (error != NULL)
    {
      g_print ("Error creating photoset: %s\n", error->message);
      g_error_free (error);
    }
  else
    {
      g_print ("[added_to_photosets_cb]::Success! (%s)\n\n",
               result ? "OK" : "FAIL");
    }
}

void
photoset_created_cb                     (GObject      *object,
                                         GAsyncResult *res,
                                         gpointer      user_data)
{
  FspPhotosMgr *photos_mgr = FSP_PHOTOS_MGR (object);
  GError *error = NULL;
  created_photoset_id =
    fsp_photos_mgr_create_photoset_finish (photos_mgr, res, &error);

  if (error != NULL)
    {
      g_print ("Error creating photoset: %s\n", error->message);
      g_error_free (error);
    }
  else
    {
      g_print ("[photosets_created_cb]::Success! Photo set Id: %s\n\n",
               created_photoset_id);

      getchar ();

      /* Continue adding a picture to the photoset, but first upload a new one */
      g_print ("Uploading a new picture to be added to the photoset...");
      fsp_photos_mgr_upload_async (photos_mgr,
                                   TEST_PHOTO,
                                   "Yet another title",
                                   "Yet another description ",
                                   "yet some other tags",
                                   FSP_VISIBILITY_NO,
                                   FSP_VISIBILITY_YES,
                                   FSP_VISIBILITY_NONE,
                                   FSP_SAFETY_LEVEL_NONE,
                                   FSP_CONTENT_TYPE_PHOTO,
                                   FSP_SEARCH_SCOPE_NONE,
                                   NULL, upload_cb, photoset_created_cb);
    }
}

void
get_photosets_cb                        (GObject      *object,
                                         GAsyncResult *res,
                                         gpointer      user_data)
{
  FspPhotosMgr *photos_mgr = FSP_PHOTOS_MGR (object);
  GError *error = NULL;
  GSList *photosets_list =
    fsp_photos_mgr_get_photosets_finish (photos_mgr, res, &error);

  if (error != NULL)
    {
      g_print ("Error getting photosets: %s\n", error->message);
      g_error_free (error);
    }
  else
    {
      g_print ("[get_photosets_cb]::Success! Number of photosets found: %d\n",
               g_slist_length (photosets_list));

      gint i = 0;
      GSList *item = NULL;
      FspDataPhotoSet *photoset = NULL;
      for (item = photosets_list; item; item = g_slist_next (item))
        {
          photoset = FSP_DATA_PHOTO_SET (item->data);
          g_print ("[get_photosets_cb]::\tPhotoset #%d\n", i++);
          g_print ("[get_photosets_cb]::\t\tPhotoset id: %s\n", photoset->id);
          g_print ("[get_photosets_cb]::\t\tPhotoset title: %s\n", photoset->title);
          g_print ("[get_photosets_cb]::\t\tPhotoset description: %s\n", photoset->description);
          g_print ("[get_photosets_cb]::\t\tPhotoset primary photo id: %s\n", photoset->primary_photo_id);
          g_print ("[get_photosets_cb]::\t\tPhotoset number of photos: %d\n", photoset->n_photos);

          fsp_data_free (FSP_DATA (photoset));
        }

      getchar ();

      /* Continue creating a new photoset */
      g_print ("Creatine a new photoset...\n");
      fsp_photos_mgr_create_photoset_async (photos_mgr,
                                            "Photoset's title",
                                            "Photoset's description",
                                            uploaded_photo_id,
                                            NULL, photoset_created_cb, NULL);
      g_slist_free (photosets_list);
    }
}

void
photo_get_info_cb                       (GObject      *object,
                                         GAsyncResult *res,
                                         gpointer      user_data)
{
  FspPhotosMgr *photos_mgr = FSP_PHOTOS_MGR (object);
  GError *error = NULL;
  FspDataPhotoInfo *photo_info =
    fsp_photos_mgr_get_info_finish (photos_mgr, res, &error);

  if (error != NULL)
    {
      g_print ("Error uploading picture: %s\n", error->message);
      g_error_free (error);
    }
  else
    {
      g_print ("[photo_get_info_cb]::Success! Photo Info:\n");
      g_print ("[photo_get_info_cb]::\tPhoto id: %s\n", photo_info->id);
      g_print ("[photo_get_info_cb]::\tPhoto secret: %s\n", photo_info->secret);
      g_print ("[photo_get_info_cb]::\tPhoto server: %s\n", photo_info->server);
      g_print ("[photo_get_info_cb]::\tPhoto isfavorite: %s\n",
               photo_info->is_favorite ? "Yes" : "No");
      g_print ("[photo_get_info_cb]::\tPhoto license: %d\n", photo_info->license);
      g_print ("[photo_get_info_cb]::\tPhoto rotation: %d\n", photo_info->rotation);
      g_print ("[photo_get_info_cb]::\tPhoto orig_secret: %s\n", photo_info->orig_secret);
      g_print ("[photo_get_info_cb]::\tPhoto orig_format: %s\n", photo_info->orig_format);
      g_print ("[photo_get_info_cb]::\tPhoto title: %s\n", photo_info->title);
      g_print ("[photo_get_info_cb]::\tPhoto description: %s\n", photo_info->description);
      g_print ("[photo_get_info_cb]::\tPhoto ispublic: %s\n",
               photo_info->is_public ? "Yes" : "No");
      g_print ("[photo_get_info_cb]::\tPhoto isfamily: %s\n",
               photo_info->is_family ? "Yes" : "No");
      g_print ("[photo_get_info_cb]::\tPhoto isfriend: %s\n",
               photo_info->is_friend ? "Yes" : "No");
      g_print ("[photo_get_info_cb]::\tPhoto permcomment: %d\n", photo_info->perm_comment);
      g_print ("[photo_get_info_cb]::\tPhoto permaddmeta: %d\n", photo_info->perm_add_meta);
      g_print ("[photo_get_info_cb]::\tPhoto cancomment: %d\n", photo_info->can_comment);
      g_print ("[photo_get_info_cb]::\tPhoto canaddmeta: %d\n", photo_info->can_add_meta);

      /* Make a pause before continuing */
      g_print ("Press ENTER to continue...\n\n");
      getchar ();

      /* Continue getting the list of photosets */
      g_print ("Getting list of photosets...\n");
      fsp_photos_mgr_get_photosets_async (photos_mgr, NULL,
                                          get_photosets_cb, NULL);

      fsp_data_free (FSP_DATA (photo_info));
    }
}

void
complete_auth_cb                        (GObject      *object,
                                         GAsyncResult *res,
                                         gpointer      user_data)
{
  FspSession* session = FSP_SESSION (object);
  GError *error = NULL;
  gboolean auth_done = fsp_session_complete_auth_finish (session, res, &error);

  if (error != NULL)
    {
      g_print ("Error completing authorization: %s\n", error->message);
      g_error_free (error);
    }
  else
    {
      g_print ("[complete_auth_cb]::Result: %s\n\n",
               auth_done ? "Success!" : "Not authorized");
      if (auth_done)
        {

          FspPhotosMgr *photos_mgr = NULL;
          const gchar *token = NULL;

          token = fsp_session_get_token (session);
          g_print ("[complete_auth_cb]::Auth token = %s\n", token);

          /* Continue uploading a picture */
          g_print ("Uploading a picture...\n");
          photos_mgr = fsp_photos_mgr_new (session);
          fsp_photos_mgr_upload_async (photos_mgr,
                                       TEST_PHOTO,
                                       "title",
                                       "description",
                                       "áèïôu "
                                       "çÇ*+[]{} "
                                       "qwerty "
                                       "!·$%&/(@#~^*+ "
                                       "\"Tag With Spaces\"",
                                       FSP_VISIBILITY_NO,
                                       FSP_VISIBILITY_YES,
                                       FSP_VISIBILITY_NONE,
                                       FSP_SAFETY_LEVEL_NONE,
                                       FSP_CONTENT_TYPE_PHOTO,
                                       FSP_SEARCH_SCOPE_NONE,
                                       NULL, upload_cb, complete_auth_cb);
        }
    }
}

void
get_auth_url_cb                         (GObject      *object,
                                         GAsyncResult *res,
                                         gpointer      user_data)
{
  FspSession* session = FSP_SESSION (object);
  GError *error = NULL;
  gchar *auth_url = fsp_session_get_auth_url_finish (session, res, &error);

  if (error != NULL)
    {
      g_print ("Error getting auth URL: %s\n", error->message);
      g_error_free (error);
    }
  else
    {
      g_print ("[get_auth_url_cb]::Result: %s\n\n",
               auth_url ? auth_url : "No URL got");

      /* Make a pause before continuing */
      g_print ("Press ENTER to continue (after the authorization)...\n\n");
      getchar ();

      /* Continue finishing the authorization */
      g_print ("Finishing authorization...\n");
      fsp_session_complete_auth_async (session, NULL, complete_auth_cb, NULL);

      g_free (auth_url);
    }
}

gboolean
do_work (gpointer unused)
{
  FspSession *session = fsp_session_new (API_KEY, SHARED_SECRET, NULL);
  g_print ("Created FspSession:\n");

  const gchar *api_key = fsp_session_get_api_key (session);
  const gchar *secret = fsp_session_get_secret (session);

  g_print ("\tAPI key: %s\n\tSecret: %s\n\n", api_key, secret);

  g_print ("Getting authorization URL...\n");
  fsp_session_get_auth_url_async (session, NULL, get_auth_url_cb, NULL);

  return FALSE;
}

int
main                                    (int    argc,
                                         char **args)
{
  GMainLoop *mainloop;
  g_type_init ();
  g_thread_init (NULL);

  g_print ("Running flicksoup example...\n\n");

  /* Queue the work in the main context */
  g_idle_add (do_work, NULL);

  /* Run the main loop */
  mainloop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (mainloop);

  return 0;
}

