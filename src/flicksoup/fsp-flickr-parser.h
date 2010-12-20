/*
 * fsp-flickr-parser.h
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

#ifndef _FSP_FLICKR_PARSER_H
#define _FSP_FLICKR_PARSER_H

#include <glib.h>
#include <glib-object.h>

#include <flicksoup/fsp-data.h>

G_BEGIN_DECLS

#define FSP_TYPE_FLICKR_PARSER                  \
  (fsp_flickr_parser_get_type())
#define FSP_FLICKR_PARSER(obj)                  \
  (G_TYPE_CHECK_INSTANCE_CAST (obj, FSP_TYPE_FLICKR_PARSER, FspFlickrParser))
#define FSP_FLICKR_PARSER_CLASS(klass)          \
  (G_TYPE_CHECK_CLASS_CAST(klass, FSP_TYPE_FLICKR_PARSER, FspFlickrParserClass))
#define FSP_IS_FLICKR_PARSER(obj)               \
  (G_TYPE_CHECK_INSTANCE_TYPE(obj, FSP_TYPE_FLICKR_PARSER))
#define FSP_IS_FLICKR_PARSER_CLASS(klass)       \
  (G_TYPE_CHECK_CLASS_TYPE((klass), FSP_TYPE_FLICKR_PARSER))
#define FSP_FLICKR_PARSER_GET_CLASS(obj)        \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), FSP_TYPE_FLICKR_PARSER, FspFlickrParserClass))

typedef struct _FspFlickrParser FspFlickrParser;
typedef struct _FspFlickrParserClass FspFlickrParserClass;

struct _FspFlickrParser
{
  GObject parent_instance;
};

struct _FspFlickrParserClass
{
  GObjectClass parent_class;
};

/* All the parsers should be defined like this type */
typedef
gpointer (* FspFlickrParserFunc)        (FspFlickrParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error);

GType
fsp_flickr_parser_get_type              (void) G_GNUC_CONST;

FspFlickrParser *
fsp_flickr_parser_get_instance          (void);

gchar *
fsp_flickr_parser_get_frob              (FspFlickrParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error);

FspDataAuthToken *
fsp_flickr_parser_get_auth_token        (FspFlickrParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error);

gchar *
fsp_flickr_parser_get_upload_result     (FspFlickrParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error);

FspDataPhotoInfo *
fsp_flickr_parser_get_photo_info        (FspFlickrParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error);

GSList *
fsp_flickr_parser_get_photosets_list    (FspFlickrParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error);

gpointer
fsp_flickr_parser_added_to_photoset     (FspFlickrParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error);

gchar *
fsp_flickr_parser_photoset_created      (FspFlickrParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error);

G_END_DECLS

#endif
