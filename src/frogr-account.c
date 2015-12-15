/*
 * frogr-account.c -- User account for frogr.
 *
 * Copyright (C) 2009-2012 Mario Sanchez Prada
 *           (C) 2009 Adrian Perez
 * Authors:
 *   Adrian Perez <aperez@igalia.com>
 *   Mario Sanchez Prada <msanchez@gnome.org>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>
 */

#include "frogr-account.h"


struct _FrogrAccount
{
  GObject parent;

  gchar *token;
  gchar *token_secret;
  gchar *permissions;
  gchar *id;
  gchar *username;
  gchar *fullname;
  gchar *version;
  gboolean is_active;
  gboolean has_extra_info;

  /* Following properties won't be persistent */
  gulong remaining_bandwidth;
  gulong max_bandwidth;
  gulong max_picture_filesize;
  gulong max_video_filesize;
  guint remaining_videos;
  guint current_videos;
  gboolean is_pro;
};

G_DEFINE_TYPE (FrogrAccount, frogr_account, G_TYPE_OBJECT)


/* Properties */
enum {
  PROP_0,
  PROP_TOKEN,
  PROP_TOKEN_SECRET,
  PROP_PERMISSIONS,
  PROP_ID,
  PROP_USERNAME,
  PROP_FULLNAME,
  PROP_VERSION,
  PROP_IS_ACTIVE,
  PROP_HAS_EXTRA_INFO,
  PROP_REMAINING_BANDWIDTH,
  PROP_MAX_BANDWIDTH,
  PROP_MAX_PICTURE_FILESIZE,
  PROP_MAX_VIDEO_FILESIZE,
  PROP_REMAINING_VIDEOS,
  PROP_CURRENT_VIDEOS,
  PROP_IS_PRO
};


/* Private API bits */

static void
_frogr_account_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  FrogrAccount *self = FROGR_ACCOUNT (object);

  switch (property_id)
    {
    case PROP_TOKEN:
      frogr_account_set_token (self, g_value_get_string (value));
      break;

    case PROP_TOKEN_SECRET:
      frogr_account_set_token_secret (self, g_value_get_string (value));
      break;

    case PROP_PERMISSIONS:
      frogr_account_set_permissions (self, g_value_get_string (value));
      break;

    case PROP_ID:
      frogr_account_set_id (self, g_value_get_string (value));
      break;

    case PROP_USERNAME:
      frogr_account_set_username (self, g_value_get_string (value));
      break;

    case PROP_FULLNAME:
      frogr_account_set_fullname (self, g_value_get_string (value));
      break;

    case PROP_VERSION:
      frogr_account_set_version (self, g_value_get_string (value));
      break;

    case PROP_IS_ACTIVE:
      frogr_account_set_is_active (self, g_value_get_boolean (value));
      break;

    case PROP_HAS_EXTRA_INFO:
      frogr_account_set_has_extra_info (self, g_value_get_boolean (value));
      break;

    case PROP_REMAINING_BANDWIDTH:
      frogr_account_set_remaining_bandwidth (self, g_value_get_ulong (value));
      break;

    case PROP_MAX_BANDWIDTH:
      frogr_account_set_max_bandwidth (self, g_value_get_ulong (value));
      break;

    case PROP_MAX_PICTURE_FILESIZE:
      frogr_account_set_max_picture_filesize (self, g_value_get_ulong (value));
      break;

    case PROP_MAX_VIDEO_FILESIZE:
      frogr_account_set_max_video_filesize (self, g_value_get_ulong (value));
      break;

    case PROP_REMAINING_VIDEOS:
      frogr_account_set_remaining_videos (self, g_value_get_uint (value));
      break;

    case PROP_CURRENT_VIDEOS:
      frogr_account_set_current_videos (self, g_value_get_uint (value));
      break;

    case PROP_IS_PRO:
      frogr_account_set_is_pro (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
_frogr_account_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  FrogrAccount *account = FROGR_ACCOUNT (object);

  switch (property_id)
    {
    case PROP_TOKEN:
      g_value_set_string (value, account->token);
      break;

    case PROP_TOKEN_SECRET:
      g_value_set_string (value, account->token_secret);
      break;

    case PROP_PERMISSIONS:
      g_value_set_string (value, account->permissions);
      break;

    case PROP_ID:
      g_value_set_string (value, account->id);
      break;

    case PROP_USERNAME:
      g_value_set_string (value, account->username);
      break;

    case PROP_FULLNAME:
      g_value_set_string (value, account->fullname);
      break;

    case PROP_VERSION:
      g_value_set_string (value, account->version);
      break;

    case PROP_IS_ACTIVE:
      g_value_set_boolean (value, account->is_active);
      break;

    case PROP_HAS_EXTRA_INFO:
      g_value_set_boolean (value, account->has_extra_info);
      break;

    case PROP_REMAINING_BANDWIDTH:
      g_value_set_ulong (value, account->remaining_bandwidth);
      break;

    case PROP_MAX_BANDWIDTH:
      g_value_set_ulong (value, account->max_bandwidth);
      break;

    case PROP_MAX_PICTURE_FILESIZE:
      g_value_set_ulong (value, account->max_picture_filesize);
      break;

    case PROP_MAX_VIDEO_FILESIZE:
      g_value_set_ulong (value, account->max_video_filesize);
      break;

    case PROP_REMAINING_VIDEOS:
      g_value_set_uint (value, account->remaining_videos);
      break;

    case PROP_CURRENT_VIDEOS:
      g_value_set_uint (value, account->current_videos);
      break;

    case PROP_IS_PRO:
      g_value_set_boolean (value, account->is_pro);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
_frogr_account_finalize (GObject *object)
{
  FrogrAccount *account = FROGR_ACCOUNT (object);

  g_free (account->token);
  g_free (account->token_secret);
  g_free (account->permissions);
  g_free (account->id);
  g_free (account->username);
  g_free (account->fullname);
  g_free (account->version);

  /* Call superclass */
  G_OBJECT_CLASS (frogr_account_parent_class)->finalize (object);
}

static void
frogr_account_class_init (FrogrAccountClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  obj_class->get_property = _frogr_account_get_property;
  obj_class->set_property = _frogr_account_set_property;
  obj_class->finalize     = _frogr_account_finalize;

  pspec = g_param_spec_string ("token",
                               "token",
                               "Flickr API authentication token",
                               "",
                               G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_TOKEN, pspec);

  pspec = g_param_spec_string ("token-secret",
                               "token-secret",
                               "Flickr API authentication token secret",
                               "",
                               G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_TOKEN_SECRET, pspec);

  pspec = g_param_spec_string ("permissions",
                               "permissions",
                               "Permissions granted for the account",
                               "",
                               G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_PERMISSIONS, pspec);

  pspec = g_param_spec_string ("id",
                               "id",
                               "Unique ID for the account",
                               "",
                               G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_ID, pspec);

  pspec = g_param_spec_string ("username",
                               "username",
                               "Username for the account",
                               "",
                               G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_USERNAME, pspec);

  pspec = g_param_spec_string ("fullname",
                               "fullname",
                               "Fullname for the account's user",
                               "",
                               G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_FULLNAME, pspec);

  pspec = g_param_spec_string ("version",
                               "version",
                               "Version of the format used to store"
                               "the details for an account",
                               ACCOUNTS_CURRENT_VERSION,
                               G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_VERSION, pspec);

  pspec = g_param_spec_boolean ("is-active",
                                "is-active",
                                "Whether the account is active or not",
                                FALSE,
                                G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_IS_ACTIVE, pspec);

  pspec = g_param_spec_boolean ("has-extra-info",
                                "has-extra-info",
                                "Whether the account has been filled with "
                                "extra information as retrieved from flickr",
                                FALSE,
                                G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_HAS_EXTRA_INFO, pspec);

  pspec = g_param_spec_ulong ("remaining-bandwidth",
                              "remaining-bandwidth",
                              "Remaining monthly bandwidth in KB",
                              0, G_MAXULONG, G_MAXULONG,
                              G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_REMAINING_BANDWIDTH, pspec);

  pspec = g_param_spec_ulong ("max-bandwidth",
                              "max-bandwidth",
                              "Max monthly bandwidth in KB",
                              0, G_MAXULONG, G_MAXULONG,
                              G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_MAX_BANDWIDTH, pspec);

  pspec = g_param_spec_ulong ("max-picture-filesize",
                              "max-picture-filesize",
                              "Max allowed filesize for pictures in KB",
                              0, G_MAXULONG, G_MAXULONG,
                              G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_MAX_PICTURE_FILESIZE, pspec);

  pspec = g_param_spec_ulong ("max-video-filesize",
                              "max-video-filesize",
                              "Max allowed filesize for videos in KB",
                              0, G_MAXULONG, G_MAXULONG,
                              G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_MAX_VIDEO_FILESIZE, pspec);

  pspec = g_param_spec_uint ("remaining-videos",
                             "remaining-videos",
                             "Remaining number of videos for upload",
                             0, G_MAXUINT, G_MAXUINT,
                             G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_REMAINING_VIDEOS, pspec);

  pspec = g_param_spec_uint ("current-videos",
                             "current-videos",
                             "Current number of videos uploaded",
                             0, G_MAXUINT, G_MAXUINT,
                             G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_CURRENT_VIDEOS, pspec);

  pspec = g_param_spec_boolean ("is-pro",
                                "is-pro",
                                "Whether the user has a Pro account or not",
                                FALSE,
                                G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_IS_PRO, pspec);
}

static void
frogr_account_init (FrogrAccount *self)
{
  self->token = NULL;
  self->token_secret = NULL;
  self->permissions = NULL;
  self->id = NULL;
  self->username = NULL;
  self->fullname = NULL;
  self->version = NULL;
  self->is_active = FALSE;
  self->has_extra_info = FALSE;
  self->remaining_bandwidth = G_MAXULONG;
  self->max_bandwidth = G_MAXULONG;
  self->max_picture_filesize = G_MAXULONG;
  self->max_video_filesize = G_MAXULONG;
  self->remaining_videos = G_MAXUINT;
  self->current_videos = G_MAXUINT;
  self->is_pro = FALSE;
}

FrogrAccount *
frogr_account_new (void)
{
  return frogr_account_new_full (NULL, NULL);
}

FrogrAccount *
frogr_account_new_with_token (const gchar *token)
{
  g_return_val_if_fail (token, NULL);
  return frogr_account_new_full (token, NULL);
}

FrogrAccount*
frogr_account_new_full (const gchar *token, const gchar *token_secret)
{
  return FROGR_ACCOUNT (g_object_new (FROGR_TYPE_ACCOUNT,
                                      "token", token,
                                      "token-secret", token_secret,
                                      "version", ACCOUNTS_CURRENT_VERSION,
                                      NULL));
}

const gchar *
frogr_account_get_token (FrogrAccount *self)
{
  g_return_val_if_fail (FROGR_IS_ACCOUNT (self), NULL);
  return self->token;
}

void
frogr_account_set_token (FrogrAccount *self,
                         const gchar *token)
{
  g_return_if_fail (FROGR_IS_ACCOUNT (self));
  g_free (self->token);
  self->token = g_strdup (token);
}

const gchar*
frogr_account_get_token_secret (FrogrAccount *self)
{
  g_return_val_if_fail (FROGR_IS_ACCOUNT (self), NULL);
  return self->token_secret;
}

void
frogr_account_set_token_secret (FrogrAccount *self,
                                const gchar *token_secret)
{
  g_return_if_fail (FROGR_IS_ACCOUNT (self));
  g_free (self->token_secret);
  self->token_secret = g_strdup (token_secret);
}

const gchar*
frogr_account_get_permissions (FrogrAccount *self)
{
  g_return_val_if_fail (FROGR_IS_ACCOUNT (self), NULL);
  return self->permissions;
}

void
frogr_account_set_permissions (FrogrAccount *self, const gchar *permissions)
{
  g_return_if_fail (FROGR_IS_ACCOUNT (self));
  g_free (self->permissions);
  self->permissions = g_strdup (permissions);
}

const gchar*
frogr_account_get_id (FrogrAccount *self)
{
  g_return_val_if_fail (FROGR_IS_ACCOUNT (self), NULL);
  return self->id;
}

void
frogr_account_set_id (FrogrAccount *self, const gchar *id)
{
  g_return_if_fail (FROGR_IS_ACCOUNT (self));
  g_free (self->id);
  self->id = g_strdup (id);
}

const gchar*
frogr_account_get_username (FrogrAccount *self)
{
  g_return_val_if_fail (FROGR_IS_ACCOUNT (self), NULL);
  return self->username;
}

void
frogr_account_set_username (FrogrAccount *self, const gchar *username)
{
  g_return_if_fail (FROGR_IS_ACCOUNT (self));
  g_free (self->username);
  self->username = g_strdup (username);
}

const gchar*
frogr_account_get_fullname (FrogrAccount *self)
{
  g_return_val_if_fail (FROGR_IS_ACCOUNT (self), NULL);
  return self->fullname;
}

void
frogr_account_set_fullname (FrogrAccount *self, const gchar *fullname)
{
  g_return_if_fail (FROGR_IS_ACCOUNT (self));
  g_free (self->fullname);
  self->fullname = g_strdup (fullname);
}

gboolean
frogr_account_is_active (FrogrAccount *self)
{
  g_return_val_if_fail (FROGR_IS_ACCOUNT (self), FALSE);
  return self->is_active;
}

void
frogr_account_set_is_active (FrogrAccount *self, gboolean is_active)
{
  g_return_if_fail (FROGR_IS_ACCOUNT (self));
  self->is_active = is_active;
}

gboolean
frogr_account_has_extra_info (FrogrAccount *self)
{
  g_return_val_if_fail (FROGR_IS_ACCOUNT (self), FALSE);
  return self->has_extra_info;
}

void
frogr_account_set_has_extra_info (FrogrAccount *self, gboolean has_extra_info)
{
  g_return_if_fail (FROGR_IS_ACCOUNT (self));
  self->has_extra_info = has_extra_info;
}

gulong
frogr_account_get_remaining_bandwidth (FrogrAccount *self)
{
  g_return_val_if_fail (FROGR_IS_ACCOUNT (self), G_MAXULONG);
  return self->remaining_bandwidth;
}

void
frogr_account_set_remaining_bandwidth (FrogrAccount *self, gulong remaining_bandwidth)
{
  g_return_if_fail (FROGR_IS_ACCOUNT (self));
  self->remaining_bandwidth = remaining_bandwidth;
}

gulong
frogr_account_get_max_bandwidth (FrogrAccount *self)
{
  g_return_val_if_fail (FROGR_IS_ACCOUNT (self), G_MAXULONG);
  return self->max_bandwidth;
}

void
frogr_account_set_max_bandwidth (FrogrAccount *self, gulong max_bandwidth)
{
  g_return_if_fail (FROGR_IS_ACCOUNT (self));
  self->max_bandwidth = max_bandwidth;
}

gulong frogr_account_get_max_picture_filesize (FrogrAccount *self)
{
  g_return_val_if_fail (FROGR_IS_ACCOUNT (self), G_MAXULONG);
  return self->max_picture_filesize;
}

void frogr_account_set_max_picture_filesize (FrogrAccount *self,
                                           gulong max_filesize)
{
  g_return_if_fail (FROGR_IS_ACCOUNT (self));
  self->max_picture_filesize = max_filesize;
}

gulong
frogr_account_get_max_video_filesize (FrogrAccount *self)
{
  g_return_val_if_fail (FROGR_IS_ACCOUNT (self), G_MAXULONG);
  return self->max_video_filesize;

}

void
frogr_account_set_max_video_filesize (FrogrAccount *self, gulong max_filesize)
{
  g_return_if_fail (FROGR_IS_ACCOUNT (self));
  self->max_video_filesize = max_filesize;
}

gulong
frogr_account_get_remaining_videos (FrogrAccount *self)
{
  g_return_val_if_fail (FROGR_IS_ACCOUNT (self), G_MAXUINT);
  return self->remaining_videos;
}

void
frogr_account_set_remaining_videos (FrogrAccount *self, guint remaining_videos)
{
  g_return_if_fail (FROGR_IS_ACCOUNT (self));
  self->remaining_videos = remaining_videos;
}

guint
frogr_account_get_current_videos (FrogrAccount *self)
{
  g_return_val_if_fail (FROGR_IS_ACCOUNT (self), G_MAXUINT);
  return self->current_videos;
}

void
frogr_account_set_current_videos (FrogrAccount *self, guint current_videos)
{
  g_return_if_fail (FROGR_IS_ACCOUNT (self));
  self->current_videos = current_videos;
}

gboolean
frogr_account_is_pro (FrogrAccount *self)
{
  g_return_val_if_fail (FROGR_IS_ACCOUNT (self), FALSE);
  return self->is_pro;
}

void
frogr_account_set_is_pro (FrogrAccount *self, gboolean is_pro)
{
  g_return_if_fail (FROGR_IS_ACCOUNT (self));
  self->is_pro = is_pro;
}

gboolean
frogr_account_is_valid (FrogrAccount *self)
{
  g_return_val_if_fail (FROGR_IS_ACCOUNT (self), FALSE);

  if (self->token == NULL || self->token[0] == '\0')
    return FALSE;

  if (self->permissions == NULL || self->permissions[0] == '\0')
    return FALSE;

  if (self->id == NULL || self->id[0] == '\0')
    return FALSE;

  if (self->username == NULL || self->username[0] == '\0')
    return FALSE;

  return TRUE;
}

const gchar*
frogr_account_get_version (FrogrAccount *self)
{
  g_return_val_if_fail (FROGR_IS_ACCOUNT (self), FALSE);
  return self->version;
}

void
frogr_account_set_version (FrogrAccount *self, const gchar *version)
{
  g_return_if_fail (FROGR_IS_ACCOUNT (self));
  g_free (self->version);
  self->version = g_strdup (version);
}

gboolean
frogr_account_equal (FrogrAccount *self, FrogrAccount *other)
{
  /* This function should gracefully handle NULL values */
  if (self == other)
    return TRUE;
  else if (self == NULL || other == NULL)
    return FALSE;

  /* If not NULL, passed parameters must be instances of FrogrAccount */
  g_return_val_if_fail (FROGR_IS_ACCOUNT (self), FALSE);
  g_return_val_if_fail (FROGR_IS_ACCOUNT (other), FALSE);

  if (g_strcmp0 (self->token, other->token))
    return FALSE;

  if (g_strcmp0 (self->token_secret, other->token_secret))
    return FALSE;

  if (g_strcmp0 (self->permissions, other->permissions))
    return FALSE;

  if (g_strcmp0 (self->id, other->id))
    return FALSE;

  if (g_strcmp0 (self->username, other->username))
    return FALSE;

  if (g_strcmp0 (self->fullname, other->fullname))
    return FALSE;

  if (g_strcmp0 (self->version, other->version))
    return FALSE;

  if (self->remaining_bandwidth != other->remaining_bandwidth)
    return FALSE;

  if (self->max_bandwidth != other->max_bandwidth)
    return FALSE;

  if (self->is_pro != other->is_pro)
    return FALSE;

  return TRUE;
}
