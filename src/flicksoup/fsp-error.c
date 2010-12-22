/*
 * fsp-error.c
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

#include "fsp-error.h"

#define N_SPECIFIC_ERRORS 10
#define N_GENERAL_ERRORS 110
#define N_ERRORS (N_SPECIFIC_ERRORS + N_GENERAL_ERRORS)

static const FspError photo_upload_translations [N_SPECIFIC_ERRORS] = {
  FSP_ERROR_UNKNOWN,                    // 1
  FSP_ERROR_UPLOAD_MISSING_PHOTO,       // 2
  FSP_ERROR_UPLOAD_GENERAL_FAILURE,     // 3
  FSP_ERROR_UPLOAD_INVALID_FILE,        // 4
  FSP_ERROR_UPLOAD_INVALID_FILE,        // 5
  FSP_ERROR_UPLOAD_QUOTA_EXCEEDED,      // 6
  FSP_ERROR_UNKNOWN,                    // 7
  FSP_ERROR_UNKNOWN,                    // 8
  FSP_ERROR_UNKNOWN,                    // 9
  FSP_ERROR_UNKNOWN,                    // 10
};

static const FspError photo_get_info_translations [N_SPECIFIC_ERRORS] = {
  FSP_ERROR_PHOTO_NOT_FOUND,            // 1
  FSP_ERROR_UNKNOWN,                    // 2
  FSP_ERROR_UNKNOWN,                    // 3
  FSP_ERROR_UNKNOWN,                    // 4
  FSP_ERROR_UNKNOWN,                    // 5
  FSP_ERROR_UNKNOWN,                    // 6
  FSP_ERROR_UNKNOWN,                    // 7
  FSP_ERROR_UNKNOWN,                    // 8
  FSP_ERROR_UNKNOWN,                    // 9
  FSP_ERROR_UNKNOWN,                    // 10
};

static const FspError photoset_create_translations [N_SPECIFIC_ERRORS] = {
  FSP_ERROR_PHOTOSET_TITLE_MISSING,     // 1
  FSP_ERROR_PHOTO_NOT_FOUND,            // 2
  FSP_ERROR_PHOTOSET_CANT_CREATE,        // 3
  FSP_ERROR_UNKNOWN,                    // 4
  FSP_ERROR_UNKNOWN,                    // 5
  FSP_ERROR_UNKNOWN,                    // 6
  FSP_ERROR_UNKNOWN,                    // 7
  FSP_ERROR_UNKNOWN,                    // 8
  FSP_ERROR_UNKNOWN,                    // 9
  FSP_ERROR_UNKNOWN,                    // 10
};

static const FspError photoset_get_list_translations [N_SPECIFIC_ERRORS] = {
  FSP_ERROR_USER_NOT_FOUND,             // 1
  FSP_ERROR_UNKNOWN,                    // 2
  FSP_ERROR_UNKNOWN,                    // 3
  FSP_ERROR_UNKNOWN,                    // 4
  FSP_ERROR_UNKNOWN,                    // 5
  FSP_ERROR_UNKNOWN,                    // 6
  FSP_ERROR_UNKNOWN,                    // 7
  FSP_ERROR_UNKNOWN,                    // 8
  FSP_ERROR_UNKNOWN,                    // 9
  FSP_ERROR_UNKNOWN,                    // 10
};

static const FspError photoset_add_photo_translations [N_SPECIFIC_ERRORS] = {
  FSP_ERROR_PHOTOSET_NOT_FOUND,         // 1
  FSP_ERROR_PHOTO_NOT_FOUND,            // 2
  FSP_ERROR_ALREADY_IN_PHOTOSET,        // 3
  FSP_ERROR_UNKNOWN,                    // 4
  FSP_ERROR_UNKNOWN,                    // 5
  FSP_ERROR_UNKNOWN,                    // 6
  FSP_ERROR_UNKNOWN,                    // 7
  FSP_ERROR_UNKNOWN,                    // 8
  FSP_ERROR_UNKNOWN,                    // 9
  FSP_ERROR_UNKNOWN,                    // 10
};

static const FspError general_translations [N_GENERAL_ERRORS] = {
  FSP_ERROR_UNKNOWN,                    // 11
  FSP_ERROR_UNKNOWN,                    // 12
  FSP_ERROR_UNKNOWN,                    // 13
  FSP_ERROR_UNKNOWN,                    // 14
  FSP_ERROR_UNKNOWN,                    // 15
  FSP_ERROR_UNKNOWN,                    // 16
  FSP_ERROR_UNKNOWN,                    // 17
  FSP_ERROR_UNKNOWN,                    // 18
  FSP_ERROR_UNKNOWN,                    // 19
  FSP_ERROR_UNKNOWN,                    // 20
  FSP_ERROR_UNKNOWN,                    // 21
  FSP_ERROR_UNKNOWN,                    // 22
  FSP_ERROR_UNKNOWN,                    // 23
  FSP_ERROR_UNKNOWN,                    // 24
  FSP_ERROR_UNKNOWN,                    // 25
  FSP_ERROR_UNKNOWN,                    // 26
  FSP_ERROR_UNKNOWN,                    // 27
  FSP_ERROR_UNKNOWN,                    // 28
  FSP_ERROR_UNKNOWN,                    // 29
  FSP_ERROR_UNKNOWN,                    // 30
  FSP_ERROR_UNKNOWN,                    // 31
  FSP_ERROR_UNKNOWN,                    // 32
  FSP_ERROR_UNKNOWN,                    // 33
  FSP_ERROR_UNKNOWN,                    // 34
  FSP_ERROR_UNKNOWN,                    // 35
  FSP_ERROR_UNKNOWN,                    // 36
  FSP_ERROR_UNKNOWN,                    // 37
  FSP_ERROR_UNKNOWN,                    // 38
  FSP_ERROR_UNKNOWN,                    // 39
  FSP_ERROR_UNKNOWN,                    // 40
  FSP_ERROR_UNKNOWN,                    // 41
  FSP_ERROR_UNKNOWN,                    // 42
  FSP_ERROR_UNKNOWN,                    // 43
  FSP_ERROR_UNKNOWN,                    // 44
  FSP_ERROR_UNKNOWN,                    // 45
  FSP_ERROR_UNKNOWN,                    // 46
  FSP_ERROR_UNKNOWN,                    // 47
  FSP_ERROR_UNKNOWN,                    // 48
  FSP_ERROR_UNKNOWN,                    // 49
  FSP_ERROR_UNKNOWN,                    // 50
  FSP_ERROR_UNKNOWN,                    // 51
  FSP_ERROR_UNKNOWN,                    // 52
  FSP_ERROR_UNKNOWN,                    // 53
  FSP_ERROR_UNKNOWN,                    // 54
  FSP_ERROR_UNKNOWN,                    // 55
  FSP_ERROR_UNKNOWN,                    // 56
  FSP_ERROR_UNKNOWN,                    // 57
  FSP_ERROR_UNKNOWN,                    // 58
  FSP_ERROR_UNKNOWN,                    // 59
  FSP_ERROR_UNKNOWN,                    // 60
  FSP_ERROR_UNKNOWN,                    // 61
  FSP_ERROR_UNKNOWN,                    // 62
  FSP_ERROR_UNKNOWN,                    // 63
  FSP_ERROR_UNKNOWN,                    // 64
  FSP_ERROR_UNKNOWN,                    // 65
  FSP_ERROR_UNKNOWN,                    // 66
  FSP_ERROR_UNKNOWN,                    // 67
  FSP_ERROR_UNKNOWN,                    // 68
  FSP_ERROR_UNKNOWN,                    // 69
  FSP_ERROR_UNKNOWN,                    // 70
  FSP_ERROR_UNKNOWN,                    // 71
  FSP_ERROR_UNKNOWN,                    // 72
  FSP_ERROR_UNKNOWN,                    // 73
  FSP_ERROR_UNKNOWN,                    // 74
  FSP_ERROR_UNKNOWN,                    // 75
  FSP_ERROR_UNKNOWN,                    // 76
  FSP_ERROR_UNKNOWN,                    // 77
  FSP_ERROR_UNKNOWN,                    // 78
  FSP_ERROR_UNKNOWN,                    // 79
  FSP_ERROR_UNKNOWN,                    // 80
  FSP_ERROR_UNKNOWN,                    // 81
  FSP_ERROR_UNKNOWN,                    // 82
  FSP_ERROR_UNKNOWN,                    // 83
  FSP_ERROR_UNKNOWN,                    // 84
  FSP_ERROR_UNKNOWN,                    // 85
  FSP_ERROR_UNKNOWN,                    // 86
  FSP_ERROR_UNKNOWN,                    // 87
  FSP_ERROR_UNKNOWN,                    // 88
  FSP_ERROR_UNKNOWN,                    // 89
  FSP_ERROR_UNKNOWN,                    // 90
  FSP_ERROR_UNKNOWN,                    // 91
  FSP_ERROR_UNKNOWN,                    // 92
  FSP_ERROR_UNKNOWN,                    // 93
  FSP_ERROR_UNKNOWN,                    // 94
  FSP_ERROR_UNKNOWN,                    // 95
  FSP_ERROR_OTHER,                      // 96
  FSP_ERROR_OTHER,                      // 97
  FSP_ERROR_NOT_AUTHENTICATED,          // 98
  FSP_ERROR_NOT_ENOUGH_PERMISSIONS,     // 99
  FSP_ERROR_INVALID_API_KEY,            // 100
  FSP_ERROR_UNKNOWN,                    // 101
  FSP_ERROR_UNKNOWN,                    // 102
  FSP_ERROR_UNKNOWN,                    // 103
  FSP_ERROR_UNKNOWN,                    // 104
  FSP_ERROR_SERVICE_UNAVAILABLE,        // 105
  FSP_ERROR_UNKNOWN,                    // 106
  FSP_ERROR_UNKNOWN,                    // 107
  FSP_ERROR_AUTHENTICATION_FAILED,      // 108
  FSP_ERROR_UNKNOWN,                    // 109
  FSP_ERROR_UNKNOWN,                    // 110
  FSP_ERROR_OTHER,                      // 111
  FSP_ERROR_OTHER,                      // 112
  FSP_ERROR_UNKNOWN,                    // 113
  FSP_ERROR_OTHER,                      // 114
  FSP_ERROR_OTHER,                      // 115
  FSP_ERROR_OTHER,                      // 116
  FSP_ERROR_UNKNOWN,                    // 117
  FSP_ERROR_UNKNOWN,                    // 118
  FSP_ERROR_UNKNOWN,                    // 119
  FSP_ERROR_UNKNOWN                     // 120
};

FspError
fsp_error_get_from_response_code        (FspErrorMethod method, gint code)
{
  FspError retval = FSP_ERROR_UNKNOWN;

  /* Method specific errors */
  if (code > 0 && code <= N_SPECIFIC_ERRORS)
    {
      switch (method)
        {
        case FSP_ERROR_METHOD_GET_FROB:
        case FSP_ERROR_METHOD_GET_AUTH_TOKEN:
          /* We shouldn't get errors in this range for these two
             methods, return FSP_ERROR_UNKNOWN in case it happened. */
          retval = FSP_ERROR_UNKNOWN;
          break;

        case FSP_ERROR_METHOD_PHOTO_UPLOAD:
          retval = photo_upload_translations[code - 1];
          break;

        case FSP_ERROR_METHOD_PHOTO_GET_INFO:
          retval = photo_get_info_translations[code - 1];
          break;

        case FSP_ERROR_METHOD_PHOTOSET_CREATE:
          retval = photoset_create_translations[code - 1];
          break;

        case FSP_ERROR_METHOD_PHOTOSET_GET_LIST:
          retval = photoset_get_list_translations[code - 1];
          break;

        case FSP_ERROR_METHOD_PHOTOSET_ADD_PHOTO:
          retval = photoset_add_photo_translations[code - 1];
          break;

        default:
          retval = FSP_ERROR_UNKNOWN;
        }
    }

  /* Generic errors */
  if (code > N_SPECIFIC_ERRORS && code <= N_ERRORS)
    retval = general_translations[code - N_SPECIFIC_ERRORS - 1];

  return retval;
}
