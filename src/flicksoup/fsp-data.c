/* fsp-data.c
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
  g_return_val_if_fail ((type > FSP_UNKNOWN) && (type < FSP_DATA_LAST), NULL);

  FspData *new_data;

  new_data = g_slice_new0 (FspData);

  new_data->type = type;
  switch (type)
    {
    case FSP_USER_PROFILE:
      new_data->user_profile.id = NULL;
      new_data->user_profile.username = NULL;
      new_data->user_profile.fullname = NULL;
      new_data->user_profile.url = NULL;
      break;

    case FSP_AUTH_TOKEN:
      new_data->auth_token.token = NULL;
      new_data->auth_token.permissions = NULL;
      new_data->auth_token.user_profile = NULL;
      break;

    case FSP_PHOTO_INFO:
      new_data->photo_info.id = NULL;
      new_data->photo_info.secret = NULL;
      new_data->photo_info.server = NULL;
      new_data->photo_info.is_favorite = FALSE;
      new_data->photo_info.license = FSP_LICENSE_UNKNOWN;
      new_data->photo_info.rotation = FSP_ROTATION_NONE;
      new_data->photo_info.orig_secret = NULL;
      new_data->photo_info.orig_format = NULL;

      new_data->photo_info.title = NULL;
      new_data->photo_info.description = NULL;
      new_data->photo_info.owner = NULL;

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
      new_data->photo_set.url = NULL;
      new_data->photo_set.n_photos = -1;
      break;

    default:
      break;
    }

  return new_data;
}

FspData*
fsp_data_copy                           (const FspData *data)
{
  g_return_val_if_fail (data != NULL, NULL);

  FspData *new_data = NULL;

  new_data = fsp_data_new (FSP_UNKNOWN);
  *new_data = *data;

  switch (data->type)
    {
    case FSP_USER_PROFILE:
      new_data->user_profile.id = g_strdup (data->user_profile.id);
      new_data->user_profile.username = g_strdup (data->user_profile.username);
      new_data->user_profile.fullname = g_strdup (data->user_profile.fullname);
      new_data->user_profile.url = g_strdup (data->user_profile.url);
      break;

    case FSP_AUTH_TOKEN:
      new_data->auth_token.token = g_strdup (data->auth_token.token);
      new_data->auth_token.permissions = g_strdup (data->auth_token.permissions);
      if (data->auth_token.user_profile)
        {
          FspData *user_profile = FSP_DATA (data->auth_token.user_profile);
          new_data->auth_token.user_profile =
            FSP_DATA_USER_PROFILE (fsp_data_copy (user_profile));
        }
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
      if (data->auth_token.user_profile)
        {
          FspData *owner = FSP_DATA (data->photo_info.owner);
          new_data->photo_info.owner = FSP_DATA_USER_PROFILE (fsp_data_copy (owner));
        }

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
      new_data->photo_set.url = g_strdup(data->photo_set.url);
      new_data->photo_set.n_photos = data->photo_set.n_photos;
      break;

    default:
      break;
    }

  return new_data;
}

void
fsp_data_free                           (FspData *data)
{
  g_return_if_fail (data != NULL);

  switch (data->type)
    {
    case FSP_USER_PROFILE:
      g_free (data->user_profile.id);
      g_free (data->user_profile.username);
      g_free (data->user_profile.fullname);
      g_free (data->user_profile.url);
      break;

    case FSP_AUTH_TOKEN:
      g_free (data->auth_token.token);
      g_free (data->auth_token.permissions);

      if (data->auth_token.user_profile)
        fsp_data_free (FSP_DATA (data->auth_token.user_profile));
      break;

    case FSP_PHOTO_INFO:
      g_free (data->photo_info.id);
      g_free (data->photo_info.secret);
      g_free (data->photo_info.server);
      g_free (data->photo_info.orig_secret);
      g_free (data->photo_info.orig_format);

      g_free(data->photo_info.title);
      g_free (data->photo_info.description);
      if (data->auth_token.user_profile)
        fsp_data_free (FSP_DATA (data->photo_info.owner));
      break;

    case FSP_PHOTO_SET:
      g_free (data->photo_set.id);
      g_free (data->photo_set.title);
      g_free (data->photo_set.description);
      g_free (data->photo_set.primary_photo_id);
      g_free (data->photo_set.url);
      break;

    default:
      break;
    }
  g_slice_free (FspData, (FspData*) data);
}
