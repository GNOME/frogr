/*
 * fsp-util.h
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

#ifndef _FSP_UTIL_H
#define _FSP_UTIL_H

#include <fsp-flickr-parser.h>

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

#include <libsoup/soup.h>

G_BEGIN_DECLS

typedef struct
{
  GObject             *object;
  SoupSession         *soup_session;
  SoupMessage         *soup_message;
  GCancellable        *cancellable;
  gulong               cancellable_id;
  GAsyncReadyCallback  callback;
  gpointer             source_tag;
  gpointer             data;
} AsyncRequestData;

gchar *
get_api_signature                       (const gchar *shared_secret,
                                         const gchar *first_param,
                                         ... );
gchar *
get_api_signature_from_hash_table       (const gchar *shared_secret,
                                         GHashTable  *params_table);
gchar *
get_signed_query                        (const gchar *shared_secret,
                                         const gchar *first_param,
                                         ... );

gchar *
get_signed_query_from_hash_table        (const gchar *shared_secret,
                                         GHashTable  *params_table);

void
perform_async_request                   (SoupSession         *soup_session,
                                         const gchar         *url,
                                         SoupSessionCallback  request_cb,
                                         GObject             *source_object,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             source_tag,
                                         gpointer             data);

void
soup_session_cancelled_cb               (GCancellable *cancellable,
                                         gpointer      data);

void
handle_soup_response                    (SoupMessage         *msg,
                                         FspFlickrParserFunc  parserFunc,
                                         gpointer             data);

void
build_async_result_and_complete         (AsyncRequestData *clos,
                                         gpointer    result,
                                         GError     *error);

gpointer
finish_async_request                    (GObject       *object,
                                         GAsyncResult  *res,
                                         gpointer       source_tag,
                                         GError       **error);

G_END_DECLS

#endif
