/* fsp-data.c
 *
 * Copyright (C) 2010-2012 Mario Sanchez Prada
 * Authors: Mario Sanchez Prada <msanchez@gnome.org>
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
 * You should have received a copy of the GNU General Lesser Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>
 *
 */

#include "fsp-data.h"

GType
fsp_data_get_type                       (void)
{
  static GType our_type = 0;

  if (our_type == 0)
    our_type = g_boxed_type_register_static (g_intern_static_string ("FspData"),
					     (GBoxedCopyFunc)fsp_data_copy,
					     (GBoxedFreeFunc)fsp_data_free);
  return our_type;
}

FspData*
fsp_data_new                            (FspDataType type)
{
  FspData *new_data = NULL;

  g_return_val_if_fail ((type > FSP_UNKNOWN) && (type < FSP_DATA_LAST), NULL);

  new_data = g_slice_new0 (FspData);

  new_data->type = type;
  switch (type)
    {
    case FSP_AUTH_TOKEN:
      new_data->auth_token.token = NULL;
      new_data->auth_token.token_secret = NULL;
      new_data->auth_token.permissions = NULL;
      new_data->auth_token.nsid = NULL;
      new_data->auth_token.username = NULL;
      new_data->auth_token.fullname = NULL;
      break;

    case FSP_UPLOAD_STATUS:
      new_data->upload_status.id = NULL;
      new_data->upload_status.username = NULL;
      new_data->upload_status.pro_user = FALSE;
      new_data->upload_status.bw_max_kb = G_MAXUINT32;
      new_data->upload_status.bw_used_kb = G_MAXUINT32;
      new_data->upload_status.bw_used_videos = G_MAXUINT;
      new_data->upload_status.bw_remaining_videos = G_MAXUINT;
      new_data->upload_status.bw_remaining_kb = G_MAXUINT32;
      new_data->upload_status.picture_fs_max_kb = G_MAXUINT32;
      new_data->upload_status.video_fs_max_kb = G_MAXUINT32;
      break;

    case FSP_PHOTO_INFO:
      new_data->photo_info.id = NULL;
      new_data->photo_info.secret = NULL;
      new_data->photo_info.server = NULL;
      new_data->photo_info.is_favorite = FALSE;
      new_data->photo_info.license = FSP_LICENSE_NONE;
      new_data->photo_info.rotation = FSP_ROTATION_NONE;
      new_data->photo_info.orig_secret = NULL;
      new_data->photo_info.orig_format = NULL;

      new_data->photo_info.title = NULL;
      new_data->photo_info.description = NULL;

      new_data->photo_info.is_public = FSP_VISIBILITY_NONE;
      new_data->photo_info.is_family = FSP_VISIBILITY_NONE;
      new_data->photo_info.is_friend = FSP_VISIBILITY_NONE;

      new_data->photo_info.perm_comment = FSP_PERMISSION_UNKNOWN;
      new_data->photo_info.perm_add_meta = FSP_PERMISSION_UNKNOWN;
      new_data->photo_info.can_comment = FSP_PERMISSION_UNKNOWN;
      new_data->photo_info.can_add_meta = FSP_PERMISSION_UNKNOWN;
      break;

    case FSP_PHOTO_SET:
      new_data->photo_set.id = NULL;
      new_data->photo_set.title = NULL;
      new_data->photo_set.description = NULL;
      new_data->photo_set.primary_photo_id = NULL;
      new_data->photo_set.n_photos = -1;
      break;

    case FSP_GROUP:
      new_data->group.id = NULL;
      new_data->group.name = NULL;
      new_data->group.privacy = FSP_GROUP_PRIVACY_NONE;
      new_data->group.n_photos = -1;
      break;

    case FSP_LOCATION:
      new_data->location.latitude = 0.0;
      new_data->location.longitude = 0.0;
      new_data->location.accuracy = 16;
      new_data->location.context = FSP_LOCATION_CONTEXT_UNKNOWN;
      break;

    default:
      break;
    }

  return new_data;
}

FspData*
fsp_data_copy                           (const FspData *data)
{
  FspData *new_data = NULL;

  g_return_val_if_fail (data != NULL, NULL);

  new_data = fsp_data_new (data->type);

  switch (data->type)
    {
    case FSP_AUTH_TOKEN:
      new_data->auth_token.token = g_strdup (data->auth_token.token);
      new_data->auth_token.token_secret = g_strdup (data->auth_token.token_secret);
      new_data->auth_token.permissions = g_strdup (data->auth_token.permissions);
      new_data->auth_token.nsid = g_strdup (data->auth_token.nsid);
      new_data->auth_token.username = g_strdup (data->auth_token.username);
      new_data->auth_token.fullname = g_strdup (data->auth_token.fullname);
      break;

    case FSP_UPLOAD_STATUS:
      new_data->upload_status.id = g_strdup (data->upload_status.id);
      new_data->upload_status.username = g_strdup (data->upload_status.username);
      new_data->upload_status.pro_user = data->upload_status.pro_user;
      new_data->upload_status.bw_max_kb = data->upload_status.bw_max_kb;
      new_data->upload_status.bw_used_kb = data->upload_status.bw_used_kb;
      new_data->upload_status.bw_remaining_kb = data->upload_status.bw_remaining_kb;
      new_data->upload_status.bw_used_videos = data->upload_status.bw_used_videos;
      new_data->upload_status.bw_remaining_videos = data->upload_status.bw_remaining_videos;
      new_data->upload_status.picture_fs_max_kb = data->upload_status.picture_fs_max_kb;
      new_data->upload_status.video_fs_max_kb = data->upload_status.video_fs_max_kb;
      break;

    case FSP_PHOTO_INFO:
      new_data->photo_info.id = g_strdup(data->photo_info.id);
      new_data->photo_info.secret = g_strdup(data->photo_info.secret);
      new_data->photo_info.server = g_strdup(data->photo_info.server);
      new_data->photo_info.is_favorite = data->photo_info.is_favorite;
      new_data->photo_info.license = data->photo_info.license;
      new_data->photo_info.rotation = data->photo_info.rotation;
      new_data->photo_info.orig_secret = g_strdup(data->photo_info.orig_secret);
      new_data->photo_info.orig_format = g_strdup(data->photo_info.orig_format);

      new_data->photo_info.title = g_strdup(data->photo_info.title);
      new_data->photo_info.description = g_strdup(data->photo_info.description);

      new_data->photo_info.is_public = data->photo_info.is_public;
      new_data->photo_info.is_family = data->photo_info.is_family;
      new_data->photo_info.is_friend = data->photo_info.is_friend;

      new_data->photo_info.perm_comment = data->photo_info.perm_comment;
      new_data->photo_info.perm_add_meta = data->photo_info.perm_add_meta;
      new_data->photo_info.can_comment = data->photo_info.can_add_meta;
      new_data->photo_info.can_add_meta = data->photo_info.can_add_meta;
      break;

    case FSP_PHOTO_SET:
      new_data->photo_set.id = g_strdup(data->photo_set.id);
      new_data->photo_set.title = g_strdup(data->photo_set.title);
      new_data->photo_set.description = g_strdup(data->photo_set.description);
      new_data->photo_set.primary_photo_id = g_strdup(data->photo_set.primary_photo_id);
      new_data->photo_set.n_photos = data->photo_set.n_photos;
      break;

    case FSP_GROUP:
      new_data->group.id = g_strdup (data->group.id);
      new_data->group.name = g_strdup (data->group.name);
      new_data->group.privacy = data->group.privacy;
      new_data->group.n_photos = data->group.n_photos;
      break;

    case FSP_LOCATION:
      new_data->location.latitude = data->location.latitude;
      new_data->location.longitude = data->location.longitude;
      new_data->location.accuracy = data->location.accuracy;
      new_data->location.context = data->location.context;
      break;

    default:
      break;
    }

  return new_data;
}

void
fsp_data_free                           (FspData *data)
{
  /* Do nothing if passed NULL */
  if (data == NULL)
    return;

  switch (data->type)
    {
    case FSP_AUTH_TOKEN:
      g_free (data->auth_token.token);
      g_free (data->auth_token.token_secret);
      g_free (data->auth_token.permissions);
      g_free (data->auth_token.nsid);
      g_free (data->auth_token.username);
      g_free (data->auth_token.fullname);
      break;

    case FSP_UPLOAD_STATUS:
      g_free (data->upload_status.id);
      g_free (data->upload_status.username);
      break;

    case FSP_PHOTO_INFO:
      g_free (data->photo_info.id);
      g_free (data->photo_info.secret);
      g_free (data->photo_info.server);
      g_free (data->photo_info.orig_secret);
      g_free (data->photo_info.orig_format);

      g_free(data->photo_info.title);
      g_free (data->photo_info.description);
      break;

    case FSP_PHOTO_SET:
      g_free (data->photo_set.id);
      g_free (data->photo_set.title);
      g_free (data->photo_set.description);
      g_free (data->photo_set.primary_photo_id);
      break;

    case FSP_GROUP:
      g_free (data->group.id);
      g_free (data->group.name);
      break;

    case FSP_LOCATION:
      /* Nothing to do here (no memory allocated) */
      break;

    default:
      break;
    }
  g_slice_free (FspData, (FspData*) data);
}
