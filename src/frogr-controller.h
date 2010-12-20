/*
 * frogr-controller.h -- Controller of the whole application
 *
 * Copyright (C) 2009, 2010 Mario Sanchez Prada
 * Authors: Mario Sanchez Prada <msanchez@igalia.com>
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
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifndef FROGR_CONTROLLER_H
#define FROGR_CONTROLLER_H

#include "frogr-main-view.h"
#include "frogr-picture.h"
#include "frogr-picture-uploader.h"

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define FROGR_TYPE_CONTROLLER           (frogr_controller_get_type())
#define FROGR_CONTROLLER(obj)           (G_TYPE_CHECK_INSTANCE_CAST(obj, FROGR_TYPE_CONTROLLER, FrogrController))
#define FROGR_CONTROLLER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST(klass, FROGR_TYPE_CONTROLLER, FrogrControllerClass))
#define FROGR_IS_CONTROLLER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE(obj, FROGR_TYPE_CONTROLLER))
#define FROGR_IS_CONTROLLER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE((klass), FROGR_TYPE_CONTROLLER))
#define FROGR_CONTROLLER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), FROGR_TYPE_CONTROLLER, FrogrControllerClass))

typedef struct _FrogrController      FrogrController;
typedef struct _FrogrControllerClass FrogrControllerClass;

struct _FrogrControllerClass
{
  GObjectClass parent_class;
};

struct _FrogrController
{
  GObject parent;
};

/* Callback to be executed after every single upload */
typedef void (*FCPictureUploadedCallback) (FrogrPictureUploader *self,
                                           FrogrPicture *picture,
                                           GError *error);

/* Callback to be executed after receiving the list of albums */
typedef void (*FCAlbumsFetchedCallback) (GObject *object,
                                         GSList *albums_list,
                                         GError *error);

GType frogr_controller_get_type (void) G_GNUC_CONST;

FrogrController *frogr_controller_get_instance (void);
FrogrMainView *frogr_controller_get_main_view (FrogrController *self);

gboolean frogr_controller_run_app (FrogrController *self);
gboolean frogr_controller_quit_app (FrogrController *self);

void frogr_controller_show_about_dialog (FrogrController *self);
void frogr_controller_show_auth_dialog (FrogrController *self);
void frogr_controller_show_details_dialog (FrogrController *self,
                                           GSList *fpictures);
void frogr_controller_show_add_tags_dialog (FrogrController *self,
                                            GSList *fpictures);

void frogr_controller_show_add_to_album_dialog (FrogrController *self,
                                                GSList *fpictures);

void frogr_controller_open_auth_url (FrogrController *self);

void frogr_controller_complete_auth (FrogrController *self);

gboolean frogr_controller_is_authorized (FrogrController *self);

void frogr_controller_revoke_authorization (FrogrController *self);

void frogr_controller_upload_picture (FrogrController *self,
                                      FrogrPicture *fpicture,
                                      FCPictureUploadedCallback picture_uploaded_cb,
                                      GObject *object);

void frogr_controller_fetch_albums (FrogrController *self,
                                    FCAlbumsFetchedCallback albums_fetched_cb,
                                    GObject *object);

void frogr_controller_cancel_ongoing_request (FrogrController *self);

G_END_DECLS

#endif
