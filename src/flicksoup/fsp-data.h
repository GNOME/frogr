/* fsp-data.h
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

#ifndef _FSP_DATA_H
#define _FSP_DATA_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define FSP_TYPE_DATA               (fsp_data_get_type ())
#define FSP_DATA(data)              ((FspData*) data)
#define FSP_DATA_USER_PROFILE(data) ((FspDataUserProfile*) data)
#define FSP_DATA_AUTH_TOKEN(data)   ((FspDataAuthToken*) data)
#define FSP_DATA_PHOTO_INFO(data)   ((FspDataPhotoInfo*) data)
#define FSP_DATA_PHOTO_SET(data)    ((FspDataPhotoSet*) data)

typedef struct _FspDataUserProfile FspDataUserProfile;
typedef struct _FspDataAuthToken   FspDataAuthToken;
typedef struct _FspDataPhotoInfo   FspDataPhotoInfo;
typedef struct _FspDataPhotoSet    FspDataPhotoSet;

typedef union  _FspData	    FspData;

typedef enum
{
  FSP_UNKNOWN	   = -1,
  FSP_USER_PROFILE = 0,
  FSP_AUTH_TOKEN   = 1,
  FSP_PHOTO_INFO   = 2,
  FSP_PHOTO_SET    = 3,
  FSP_DATA_LAST
} FspDataType;

typedef enum {
  FSP_SAFETY_LEVEL_NONE       = 0,
  FSP_SAFETY_LEVEL_SAFE       = 1,
  FSP_SAFETY_LEVEL_MODERATE   = 2,
  FSP_SAFETY_LEVEL_RESTRICTED = 3
} FspSafetyLevel;

typedef enum {
  FSP_CONTENT_TYPE_NONE       = 0,
  FSP_CONTENT_TYPE_PHOTO      = 1,
  FSP_CONTENT_TYPE_SCREENSHOT = 2,
  FSP_CONTENT_TYPE_OTHER      = 3
} FspContentType;

typedef enum {
  FSP_VISIBILITY_NONE = -1,
  FSP_VISIBILITY_NO   =  0,
  FSP_VISIBILITY_YES  =  1,
} FspVisibility;

typedef enum {
  FSP_PERMISSION_UNKNOWN        = -1,
  FSP_PERMISSION_NOBODY         = 0,
  FSP_PERMISSION_FRIENDS_FAMILY = 1,
  FSP_PERMISSION_CONTACTS       = 2,
  FSP_PERMISSION_EVERYBODY      = 3,
  FSP_PERMISSION_LAST           = 3,
} FspPermission;

typedef enum {
  FSP_LICENSE_UNKNOWN    = -1,
  FSP_LICENSE_ALL_RIGHTS = 0,
  FSP_LICENSE_AT_NC_SA   = 1,
  FSP_LICENSE_AT_NC      = 2,
  FSP_LICENSE_AT_NC_ND   = 3,
  FSP_LICENSE_AT         = 4,
  FSP_LICENSE_AT_SA      = 5,
  FSP_LICENSE_ND         = 6,
  FSP_LICENSE_LAST       = 7,
} FspLicense;

typedef enum {
  FSP_ROTATION_NONE = 0,
  FSP_ROTATION_90   = 90,
  FSP_ROTATION_180  = 180,
  FSP_ROTATION_270  = 270,
} FspRotation;

typedef enum {
  FSP_SEARCH_SCOPE_NONE   = 0,
  FSP_SEARCH_SCOPE_PUBLIC = 1,
  FSP_SEARCH_SCOPE_HIDDEN = 2
} FspSearchScope;

struct _FspDataUserProfile
{
  FspDataType type;

  gchar *id;
  gchar *username;
  gchar *fullname;
  gchar *url;
  gchar *location;
};

struct _FspDataAuthToken
{
  FspDataType         type;
  gchar              *token;
  gchar              *permissions;
  FspDataUserProfile *user_profile;
};

struct _FspDataPhotoInfo
{
  FspDataType         type;
  gchar              *id;
  gchar              *secret;
  gchar              *server;
  gboolean            is_favorite;
  FspLicense          license;
  FspRotation         rotation;
  gchar              *orig_secret;
  gchar              *orig_format;

  gchar              *title;
  gchar              *description;

  FspVisibility       is_public;
  FspVisibility       is_family;
  FspVisibility       is_friend;

  FspPermission       perm_comment;
  FspPermission       perm_add_meta;
  FspPermission       can_comment;
  FspPermission       can_add_meta;
};

struct _FspDataPhotoSet
{
  FspDataType  type;
  gchar       *id;
  gchar       *title;
  gchar       *description;
  gchar       *primary_photo_id;
  gint         n_photos;
};

union _FspData
{
  FspDataType        type;
  FspDataUserProfile user_profile;
  FspDataAuthToken   auth_token;
  FspDataPhotoInfo   photo_info;
  FspDataPhotoSet    photo_set;
};

GType
fsp_data_get_type                       (void) G_GNUC_CONST;

FspData *
fsp_data_new                            (FspDataType type);

FspData *
fsp_data_copy                           (const FspData *data);

void
fsp_data_free     		        (FspData *data);

G_END_DECLS

#endif /* _FSP_DATA_H */
