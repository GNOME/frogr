/*
 * example.c -- Random example to test the library
 *
 * Copyright (C) 2010-2011 Mario Sanchez Prada
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
static gchar *first_group_id = NULL;

/* Prototypes */

void upload_cb (GObject *object, GAsyncResult *res, gpointer source_func);
void added_to_group_cb (GObject *object, GAsyncResult *res, gpointer unused);
void get_groups_cb (GObject *object, GAsyncResult *res, gpointer unused);
void added_to_photoset_cb (GObject *object, GAsyncResult *res, gpointer unused);
void photoset_created_cb (GObject *object, GAsyncResult *res, gpointer unused);
void get_photosets_cb (GObject *object, GAsyncResult *res, gpointer unused);
void photo_get_info_cb (GObject *object, GAsyncResult *res, gpointer unused);
void get_tags_list_cb (GObject *object, GAsyncResult *res, gpointer unused);
void get_upload_status_cb (GObject *object, GAsyncResult *res, gpointer unused);
void check_auth_info_cb (GObject *object, GAsyncResult *res, gpointer unused);
void complete_auth_cb (GObject *object, GAsyncResult *res, gpointer unused);
void get_auth_url_cb (GObject *object, GAsyncResult *res, gpointer unused);
gboolean do_work (gpointer unused);

/* Implementations */

void
upload_cb                               (GObject      *object,
                                         GAsyncResult *res,
                                         gpointer      source_func)
{
  FspSession *session = FSP_SESSION (object);
  GError *error = NULL;

  g_free (uploaded_photo_id);
  uploaded_photo_id = fsp_session_upload_finish (session, res, &error);

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

      if (source_func == get_tags_list_cb)
        {
          /* Continue getting info about the picture */
          g_print ("Getting info for photo %s...\n", uploaded_photo_id);
          fsp_session_get_info_async (session, uploaded_photo_id, NULL,
                                      photo_get_info_cb, NULL);
        }
      else if (source_func == photoset_created_cb)
        {
          /* Continue adding the picture to the photoset */
          g_print ("Creatine a new photoset...\n");
          fsp_session_add_to_photoset_async (session,
                                             uploaded_photo_id,
                                             created_photoset_id,
                                             NULL, added_to_photoset_cb, NULL);
        }
      else if (source_func == get_groups_cb)
        {
          /* Continue adding the picture to the photoset */
          g_print ("Creatine a new photoset...\n");
          fsp_session_add_to_group_async (session,
                                          uploaded_photo_id,
                                          first_group_id,
                                          NULL, added_to_group_cb, NULL);
        }
    }
}

void
added_to_group_cb                       (GObject      *object,
                                         GAsyncResult *res,
                                         gpointer      user_data)
{
  FspSession *session = FSP_SESSION (object);
  GError *error = NULL;
  gboolean result = FALSE;

  result = fsp_session_add_to_group_finish (session, res, &error);
  if (error != NULL)
    {
      g_print ("Error creating group: %s\n", error->message);
      g_error_free (error);
    }
  else
    {
      g_print ("[added_to_groups_cb]::Success! (%s)\n\n",
               result ? "OK" : "FAIL");
    }
}

void
get_groups_cb                           (GObject      *object,
                                         GAsyncResult *res,
                                         gpointer      user_data)
{
  FspSession *session = FSP_SESSION (object);
  GError *error = NULL;
  GSList *groups_list = NULL;

  groups_list = fsp_session_get_groups_finish (session, res, &error);
  if (error != NULL)
    {
      g_print ("Error getting groups: %s\n", error->message);
      g_error_free (error);
    }
  else
    {
      gint i = 0;
      GSList *item = NULL;
      FspDataGroup *group = NULL;

      g_print ("[get_groups_cb]::Success! Number of groups found: %d\n",
               g_slist_length (groups_list));

      for (item = groups_list; item; item = g_slist_next (item))
        {
          group = FSP_DATA_GROUP (item->data);
          g_print ("[get_groups_cb]::\tGroup #%d\n", i++);
          g_print ("[get_groups_cb]::\t\tGroup id: %s\n", group->id);
          g_print ("[get_groups_cb]::\t\tGroup name: %s\n", group->name);
          g_print ("[get_groups_cb]::\t\tGroup privacy: %d\n", group->privacy);
          g_print ("[get_groups_cb]::\t\tGroup number of photos: %d\n", group->n_photos);

          /* Store the ID of the first group to add a picture later */
          if (item == groups_list)
            first_group_id = g_strdup (group->id);

          fsp_data_free (FSP_DATA (group));
        }

      getchar ();

      /* Continue adding a picture to the group, but first upload a new one */
      g_print ("Uploading a new picture to be added to the group...");
      fsp_session_upload_async (session,
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
                                NULL, upload_cb,
                                (gpointer) get_groups_cb);

      g_slist_free (groups_list);
    }
}

void
added_to_photoset_cb                    (GObject      *object,
                                         GAsyncResult *res,
                                         gpointer      user_data)
{
  FspSession *session = FSP_SESSION (object);
  GError *error = NULL;
  gboolean result = FALSE;

  result = fsp_session_add_to_photoset_finish (session, res, &error);
  if (error != NULL)
    {
      g_print ("Error adding to photoset: %s\n", error->message);
      g_error_free (error);
    }
  else
    {
      g_print ("[added_to_photosets_cb]::Success! (%s)\n\n",
               result ? "OK" : "FAIL");

      /* Make a pause before continuing */
      g_print ("Press ENTER to continue...\n\n");
      getchar ();

      /* Continue getting the list of groups */
      g_print ("Getting list of groups...\n");
      fsp_session_get_groups_async (session, NULL,
                                    get_groups_cb, NULL);
    }
}

void
photoset_created_cb                     (GObject      *object,
                                         GAsyncResult *res,
                                         gpointer      user_data)
{
  FspSession *session = FSP_SESSION (object);
  GError *error = NULL;
  created_photoset_id =
    fsp_session_create_photoset_finish (session, res, &error);

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
      fsp_session_upload_async (session,
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
                                NULL, upload_cb,
                                (gpointer) photoset_created_cb);
    }
}

void
get_photosets_cb                        (GObject      *object,
                                         GAsyncResult *res,
                                         gpointer      user_data)
{
  FspSession *session = FSP_SESSION (object);
  GError *error = NULL;
  GSList *photosets_list =
    fsp_session_get_photosets_finish (session, res, &error);

  if (error != NULL)
    {
      g_print ("Error getting photosets: %s\n", error->message);
      g_error_free (error);
    }
  else
    {
      gint i = 0;
      GSList *item = NULL;
      FspDataPhotoSet *photoset = NULL;

      g_print ("[get_photosets_cb]::Success! Number of photosets found: %d\n",
               g_slist_length (photosets_list));

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
      fsp_session_create_photoset_async (session,
                                         "Photoset's title",
                                         "Photoset's description\nasdasda",
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
  FspSession *session = FSP_SESSION (object);
  GError *error = NULL;
  FspDataPhotoInfo *photo_info =
    fsp_session_get_info_finish (session, res, &error);

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
      fsp_session_get_photosets_async (session, NULL,
                                       get_photosets_cb, NULL);

      fsp_data_free (FSP_DATA (photo_info));
    }
}

void
get_tags_list_cb (GObject *object, GAsyncResult *res, gpointer unused)
{
  FspSession* session = FSP_SESSION (object);
  GSList *tags_list = NULL;
  GError *error = NULL;

  tags_list = fsp_session_get_tags_list_finish (session, res, &error);
  if (error != NULL)
    {
      g_print ("Error retrieving tags: %s\n", error->message);
      g_error_free (error);
    }
  else
    {
      GSList *item = NULL;
      gchar *tag = NULL;

      g_print ("[get_tags_list_cb]::Success! Number of tags found: %d\n",
               g_slist_length (tags_list));

      for (item = tags_list; item; item = g_slist_next (item))
        {
          tag = (gchar *) item->data;
          g_print ("[get_tags_list_cb]::\tTag: %s\n", tag);
          g_free (tag);
        }
      g_slist_free (tags_list);

      /* Make a pause before continuing */
      g_print ("Press ENTER to continue...\n\n");
      getchar ();

      /* Continue uploading a picture */
      g_print ("Uploading a picture...\n");
      fsp_session_upload_async (session,
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
                                NULL, upload_cb,
                                (gpointer) get_tags_list_cb);
    }
}

void
get_upload_status_cb (GObject *object, GAsyncResult *res, gpointer unused)
{
  FspSession* session = FSP_SESSION (object);
  FspDataUploadStatus *upload_status = NULL;
  GError *error = NULL;

  upload_status = fsp_session_get_upload_status_finish (session, res, &error);
  if (error != NULL)
    {
      g_print ("Error retrieving upload status: %s\n", error->message);
      g_error_free (error);
    }
  else
    {
      g_print ("[get_upload_status_cb]::Success! Upload status:\n");
      g_print ("[get_upload_status_cb]::\tUser id: %s\n", upload_status->id);
      g_print ("[get_upload_status_cb]::\tUser is pro?: %s\n",
               upload_status->pro_user ? "YES" : "NO");
      g_print ("[get_upload_status_cb]::\tBandwitdh Max KB: %lu\n",
               upload_status->bw_max_kb);
      g_print ("[get_upload_status_cb]::\tBandwitdh Used KB: %lu\n",
               upload_status->bw_used_kb);
      g_print ("[get_upload_status_cb]::\tBandwitdh Remaining KB: %lu\n",
               upload_status->bw_remaining_kb);
      g_print ("[get_upload_status_cb]::\tFilesize Max KB: %lu\n",
               upload_status->fs_max_kb);

      /* Make a pause before continuing */
      g_print ("Press ENTER to continue...\n\n");
      getchar ();

      /* Continue getting the list of tags */
      g_print ("Getting the list of tags...\n");
      fsp_session_get_tags_list_async (session, NULL,
                                       get_tags_list_cb, NULL);

      fsp_data_free (FSP_DATA (upload_status));
    }
}

void
check_auth_info_cb (GObject *object, GAsyncResult *res, gpointer unused)
{
  FspSession* session = FSP_SESSION (object);
  FspDataAuthToken *auth_token = NULL;
  GError *error = NULL;

  auth_token = fsp_session_check_auth_info_finish (session, res, &error);
  if (error != NULL)
    {
      g_print ("Error checking authorization information: %s\n", error->message);
      g_error_free (error);
    }
  else
    {
      g_print ("[check_auth_info_cb]::Result: Success!\n\n");

      g_print ("[check_auth_info_cb]::Auth token\n");
      g_print ("[check_auth_info_cb]::\ttoken = %s\n", auth_token->token);
      g_print ("[check_auth_info_cb]::\tpermissions = %s\n", auth_token->permissions);
      g_print ("[check_auth_info_cb]::\tnsid = %s\n", auth_token->nsid);
      g_print ("[check_auth_info_cb]::\tusername = %s\n", auth_token->username);
      g_print ("[check_auth_info_cb]::\tfullname = %s\n", auth_token->fullname);

      /* Make a pause before continuing */
      g_print ("Press ENTER to continue...\n\n");
      getchar ();

      /* Continue getting the upload status */
      g_print ("Retrieving upload status...\n");
      fsp_session_get_upload_status_async (session, NULL, get_upload_status_cb, NULL);

      fsp_data_free (FSP_DATA (auth_token));
    }
}

void
complete_auth_cb                        (GObject      *object,
                                         GAsyncResult *res,
                                         gpointer      user_data)
{
  FspSession* session = FSP_SESSION (object);
  FspDataAuthToken *auth_token = NULL;
  GError *error = NULL;

  auth_token = fsp_session_complete_auth_finish (session, res, &error);
  if (error != NULL)
    {
      g_print ("Error completing authorization: %s\n", error->message);
      g_error_free (error);
    }
  else
    {
      g_print ("[complete_auth_cb]::Result: Success!\n\n");

      g_print ("[complete_auth_cb]::Auth token\n");
      g_print ("[complete_auth_cb]::\ttoken = %s\n", auth_token->token);
      g_print ("[complete_auth_cb]::\tpermissions = %s\n", auth_token->permissions);
      g_print ("[complete_auth_cb]::\tnsid = %s\n", auth_token->nsid);
      g_print ("[complete_auth_cb]::\tusername = %s\n", auth_token->username);
      g_print ("[complete_auth_cb]::\tfullname = %s\n", auth_token->fullname);

      /* Make a pause before continuing */
      g_print ("Press ENTER to continue...\n\n");
      getchar ();

      /* Continue checking the authorization information */
      g_print ("Checking the authorization information...\n");
      fsp_session_check_auth_info_async (session, NULL, check_auth_info_cb, NULL);

      fsp_data_free (FSP_DATA (auth_token));
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
  FspSession *session = NULL;
  const gchar *api_key = NULL;
  const gchar *secret = NULL;

  session = fsp_session_new (API_KEY, SHARED_SECRET, NULL);
  g_print ("Created FspSession:\n");

  api_key = fsp_session_get_api_key (session);
  secret = fsp_session_get_secret (session);

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

