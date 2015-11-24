/*
 * fsp-parser.h
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

#ifndef _FSP_PARSER_H
#define _FSP_PARSER_H

#include <glib.h>
#include <glib-object.h>

#include <flicksoup/fsp-data.h>

G_BEGIN_DECLS

#define FSP_TYPE_PARSER (fsp_parser_get_type())

G_DECLARE_FINAL_TYPE (FspParser, fsp_parser, FSP, PARSER, GObject)

/* All the parsers should be defined like this type */
typedef
gpointer (* FspParserFunc)        (FspParser  *self,
                                   const gchar      *buffer,
                                   gulong            buf_size,
                                   GError          **error);

FspParser *
fsp_parser_get_instance          (void);

FspDataAuthToken *
fsp_parser_get_request_token            (FspParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error);

FspDataAuthToken *
fsp_parser_get_access_token             (FspParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error);

FspDataAuthToken *
fsp_parser_check_token                  (FspParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error);

FspDataAuthToken *
fsp_parser_exchange_token               (FspParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error);

FspDataUploadStatus *
fsp_parser_get_upload_status            (FspParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error);

gchar *
fsp_parser_get_upload_result            (FspParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error);

FspDataPhotoInfo *
fsp_parser_get_photo_info               (FspParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error);

GSList *
fsp_parser_get_photosets_list           (FspParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error);

gpointer
fsp_parser_added_to_photoset            (FspParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error);

gchar *
fsp_parser_photoset_created             (FspParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error);

GSList *
fsp_parser_get_groups_list              (FspParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error);

gpointer
fsp_parser_added_to_group               (FspParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error);

GSList *
fsp_parser_get_tags_list                (FspParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error);

gpointer
fsp_parser_set_license                  (FspParser  *self,
                                         const gchar      *buffer,
                                         gulong            buf_size,
                                         GError          **error);

gpointer
fsp_parser_set_location                  (FspParser  *self,
                                          const gchar      *buffer,
                                          gulong            buf_size,
                                          GError          **error);

FspDataLocation *
fsp_parser_get_location                  (FspParser   *self,
                                          const gchar *buffer,
                                          gulong       buf_size,
                                          GError     **error);

gpointer
fsp_parser_set_dates                     (FspParser   *self,
                                          const gchar *buffer,
                                          gulong       buf_size,
                                          GError     **error);

G_END_DECLS

#endif
