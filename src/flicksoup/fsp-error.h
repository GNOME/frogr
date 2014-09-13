/*
 * fsp-error.h
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
 * You should have received a copy of the GNU General Public Lesser License
 * along with this program; if not, see <http://www.gnu.org/licenses/>
 *
 */

#ifndef _FSP_ERROR_H
#define _FSP_ERROR_H

#include <glib.h>

G_BEGIN_DECLS

/* Error domain */
#define FSP_ERROR g_quark_from_static_string ("flicksoup-error")

/* Error codes for FSP_ERROR domain */
typedef enum {
  /* Unknown errors */
  FSP_ERROR_UNKNOWN,

  /* Errors from communication layer */
  FSP_ERROR_CANCELLED,
  FSP_ERROR_NETWORK_ERROR,
  FSP_ERROR_CLIENT_ERROR,
  FSP_ERROR_SERVER_ERROR,

  /* Errors from REST response */
  FSP_ERROR_WRONG_RESPONSE,
  FSP_ERROR_MISSING_DATA,

  /* Errors from flickr API */
  FSP_ERROR_UPLOAD_MISSING_PHOTO,
  FSP_ERROR_UPLOAD_GENERAL_FAILURE,
  FSP_ERROR_UPLOAD_INVALID_FILE,
  FSP_ERROR_UPLOAD_QUOTA_PICTURE_EXCEEDED,
  FSP_ERROR_UPLOAD_QUOTA_VIDEO_EXCEEDED,

  FSP_ERROR_PHOTO_NOT_FOUND,

  FSP_ERROR_PHOTOSET_NOT_FOUND,
  FSP_ERROR_PHOTOSET_TITLE_MISSING,
  FSP_ERROR_PHOTOSET_CANT_CREATE,
  FSP_ERROR_PHOTOSET_PHOTO_ALREADY_IN,

  FSP_ERROR_GROUP_NOT_FOUND,
  FSP_ERROR_GROUP_PHOTO_ALREADY_IN,
  FSP_ERROR_GROUP_PHOTO_IN_MAX_NUM,
  FSP_ERROR_GROUP_LIMIT_REACHED,
  FSP_ERROR_GROUP_PHOTO_ADDED_TO_QUEUE,
  FSP_ERROR_GROUP_PHOTO_ALREADY_IN_QUEUE,
  FSP_ERROR_GROUP_CONTENT_NOT_ALLOWED,

  FSP_ERROR_LOCATION_INVALID_LATITUDE,
  FSP_ERROR_LOCATION_INVALID_LONGITUDE,
  FSP_ERROR_LOCATION_INVALID_ACCURACY,
  FSP_ERROR_LOCATION_NO_INFO,

  FSP_ERROR_USER_NOT_FOUND,

  FSP_ERROR_LICENSE_NOT_FOUND,

  FSP_ERROR_AUTHENTICATION_FAILED,
  FSP_ERROR_NOT_AUTHENTICATED,
  FSP_ERROR_NOT_ENOUGH_PERMISSIONS,
  FSP_ERROR_INVALID_API_KEY,
  FSP_ERROR_SERVICE_UNAVAILABLE,

  /* Errors from flicksoup only */
  FSP_ERROR_OAUTH_NOT_AUTHORIZED_YET,
  FSP_ERROR_OAUTH_VERIFIER_INVALID,
  FSP_ERROR_OAUTH_UNKNOWN_ERROR,

  /* Default fallback for other kind of errors */
  FSP_ERROR_OTHER
} FspError;

typedef enum {
  FSP_ERROR_METHOD_UNDEFINED,
  FSP_ERROR_METHOD_CHECK_TOKEN,
  FSP_ERROR_METHOD_EXCHANGE_TOKEN,
  FSP_ERROR_METHOD_GET_UPLOAD_STATUS,
  FSP_ERROR_METHOD_PHOTO_UPLOAD,
  FSP_ERROR_METHOD_PHOTO_GET_INFO,
  FSP_ERROR_METHOD_PHOTOSET_CREATE,
  FSP_ERROR_METHOD_PHOTOSET_GET_LIST,
  FSP_ERROR_METHOD_PHOTOSET_ADD_PHOTO,
  FSP_ERROR_METHOD_GROUP_GET_LIST,
  FSP_ERROR_METHOD_GROUP_ADD_PHOTO,
  FSP_ERROR_METHOD_TAG_GET_LIST,
  FSP_ERROR_METHOD_SET_LICENSE,
  FSP_ERROR_METHOD_SET_LOCATION,
  FSP_ERROR_METHOD_GET_LOCATION,
  FSP_ERROR_METHOD_SET_DATES
} FspErrorMethod;

FspError
fsp_error_get_from_response_code        (FspErrorMethod method, gint code);

G_END_DECLS

#endif
