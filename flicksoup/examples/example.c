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

void
photo_get_info_cb                       (GObject      *obj,
                                         GAsyncResult *res,
                                         gpointer      user_data)
{
  FspPhotosMgr *photos_mgr = FSP_PHOTOS_MGR (user_data);
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
    }
}

void
upload_cb                               (GObject      *obj,
                                         GAsyncResult *res,
                                         gpointer      user_data)
{
  FspPhotosMgr *photos_mgr = FSP_PHOTOS_MGR (user_data);
  GError *error = NULL;
  gchar *photo_id = fsp_photos_mgr_upload_finish (photos_mgr, res, &error);

  if (error != NULL)
    {
      g_print ("Error uploading picture: %s\n", error->message);
      g_error_free (error);
    }
  else
    {
      g_print ("[upload_cb]::Success! Photo ID: %s\n\n", photo_id);

      /* Make a pause before continuing */
      g_print ("Press ENTER to continue...\n\n");
      getchar ();

      /* Continue finishing the authorization */
      g_print ("Getting info for photo %s...\n", photo_id);
      fsp_photos_mgr_get_info_async (photos_mgr, photo_id, NULL,
                                     photo_get_info_cb, photos_mgr);
    }
}

void
complete_auth_cb                        (GObject      *obj,
                                         GAsyncResult *res,
                                         gpointer      user_data)
{
  FspSession *session = FSP_SESSION (user_data);
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
          gchar *token = NULL;

          token = fsp_session_get_token (session);
          g_print ("[complete_auth_cb]::Auth token = %s\n", token);
          g_free (token);

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
                                       NULL, upload_cb, photos_mgr);
        }
    }
}

void
get_auth_url_cb                         (GObject      *obj,
                                         GAsyncResult *res,
                                         gpointer      user_data)
{
  FspSession *session = FSP_SESSION (user_data);
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
      fsp_session_complete_auth_async (session, NULL, complete_auth_cb, session);
    }
}

gboolean
do_work (gpointer unused)
{
  FspSession *session = fsp_session_new (API_KEY, SHARED_SECRET, NULL);
  g_print ("Created FspSession:\n");

  gchar *api_key = fsp_session_get_api_key (session);
  gchar *secret = fsp_session_get_secret (session);

  g_print ("\tAPI key: %s\n\tSecret: %s\n\n", api_key, secret);

  g_free (api_key);
  g_free (secret);

  g_print ("Getting authorization URL...\n");
  fsp_session_get_auth_url_async (session, NULL, get_auth_url_cb, session);

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

